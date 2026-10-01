/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The asynchronous loader (plan/ws074/design.md §9): http and https GETs
 * that never block the caller.  The caller's main loop polls the
 * descriptors the loader lists and gives it the ones that became ready;
 * each request goes through its states as far as its socket allows and
 * calls its callback when it ends.
 *
 * A request's states: its host's addresses looked up on the resolver's
 * thread (getaddrinfo blocks, so it is the only work done off the main
 * thread; the result comes back through a pipe), a non-blocking connect
 * to each address in turn, the TLS handshake for https, the request sent,
 * and the response read until its framing (its length, its last chunk, or
 * the end of the connection) says it is complete.  The response is parsed
 * as the synchronous fetch parses it (http.c); a redirect starts the
 * request again at its Location.  A request that makes no progress for
 * LOADER_TIMEOUT fails with ETIMEDOUT.
 *
 * Connections are kept (ws074-p058): a response that leaves its
 * connection open puts it in the loader's pool of idle connections, by
 * scheme, host and port, and the next request to the same place takes it
 * instead of connecting again (a kept connection the server has closed
 * meanwhile is replaced once by a new one).  At most LOADER_PER_HOST
 * connections to one place are open at a time; the requests past that wait
 * for one to end.  An idle connection is closed after LOADER_IDLE.
 *
 * The memory cache keeps the 200 responses that say they may be kept
 * (Cache-Control max-age, or an ETag to revalidate them; not no-store), by
 * URL, up to LOADER_CACHE_MAX bytes of bodies (the least recently used go
 * first).  A fresh entry answers a request without the network; a stale
 * one with an ETag is revalidated with If-None-Match, and a 304 answers
 * with the kept body.
 *
 * Everything but the resolver's lookups runs on the caller's thread.  A
 * callback may start other requests and cancel any request, itself
 * included; a request ended or cancelled is freed once the loader is no
 * longer walking its list.
 */

#include "net/net.h"

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

/* How long a request may make no progress, in milliseconds. */
#define LOADER_TIMEOUT		30000U

/* The most redirects a request follows. */
#define LOADER_REDIRECTS	20

/* The largest response a request reads, in bytes. */
#define LOADER_MAX_RESPONSE	((size_t)256U * 1024U * 1024U)

/* The bytes read from a connection at a time. */
#define LOADER_CHUNK		16384U

/* The most connections open to one scheme, host and port at a time. */
#define LOADER_PER_HOST		6

/* How long an idle connection is kept, in milliseconds. */
#define LOADER_IDLE		30000U

/* The most bytes of bodies the memory cache keeps. */
#define LOADER_CACHE_MAX	((size_t)64U * 1024U * 1024U)

/* The longest place (scheme://host:port) a connection is kept for. */
#define LOADER_KEY_MAX		320U

/* The deadline of a request that waits for a connection (it does not time out while it waits). */
#define LOADER_NEVER		((uint64_t)-1)

/* A request's states. */
enum loader_state {
	LOADER_READY,
	LOADER_WAITING,
	LOADER_RESOLVING,
	LOADER_CONNECTING,
	LOADER_HANDSHAKE,
	LOADER_SENDING,
	LOADER_RECEIVING,
	LOADER_ENDED
};

/*
 * A lookup for the resolver's thread: the request it is for (by number,
 * since the request may be gone when the answer comes), the host and the
 * port; then the answer.  Jobs wait in the loader's queue and answers in
 * its list of results, both under its lock.
 */
struct loader_job {
	struct loader_job *next;
	uint64_t number;
	char host[256];
	char port[16];
	struct addrinfo *addresses;
	int status;
};

/*
 * An idle connection of the pool: the place it goes to (its key), its
 * socket and TLS, and since when it has been idle.
 */
struct loader_connection {
	struct loader_connection *next;
	char key[LOADER_KEY_MAX];
	int descriptor;
	struct net_tls *tls;
	uint64_t idle_since;
};

/*
 * A response of the memory cache: its URL (the key), what it gives back
 * (the Content-Type and the body), its ETag, when it was stored or last
 * revalidated, and how long it stays fresh (max-age in seconds, -1 for
 * none; no_cache revalidates it every time).
 */
struct loader_entry {
	struct loader_entry *next;
	char *url;
	struct wb_buffer content_type;
	struct wb_buffer body;
	struct wb_buffer etag;
	uint64_t stored_at;
	long max_age;
	int no_cache;
};

/*
 * One request: its number, the URL it asks for now (after the redirects so
 * far), its state and connection (the addresses left to try, the socket,
 * TLS, and what the socket must become ready for; the place it goes to,
 * whether the connection came from the pool and whether a kept connection
 * was replaced already), the request's text and how much of it went, the
 * response's bytes so far and how far they frame it, the cache entry it
 * revalidates (or NULL), when it times out (on the loader's monotonic
 * clock, in milliseconds), and its callback.  error and response are the
 * outcome the callback reads.
 */
struct net_request {
	struct net_loader *loader;
	struct net_request *next;
	uint64_t number;
	int state;
	int cancelled;
	struct net_url url;
	int redirects;
	struct addrinfo *addresses;
	struct addrinfo *address;
	int descriptor;
	struct net_tls *tls;
	short wants;
	char key[LOADER_KEY_MAX];
	int reused;
	int retried;
	struct wb_buffer request;
	size_t sent;
	struct wb_buffer raw;
	struct net_http_framing framing;
	struct loader_entry *validating;
	uint64_t deadline;
	net_request_done done;
	void *context;
	int error;
	struct net_response response;
};

/*
 * The loader: its requests, the resolver's thread with its queue of jobs
 * and list of answers (under lock; wake tells the thread a job is there),
 * the pipe the thread writes a byte to for each answer, whether the list
 * is being walked (so that requests are freed only after the walk), the
 * pool of idle connections, and the memory cache (most recently used
 * first) with the bytes of its bodies.
 */
struct net_loader {
	struct net_request *requests;
	struct loader_connection *idle;
	struct loader_entry *cache;
	size_t cache_bytes;
	uint64_t next_number;
	int walking;
	pthread_t thread;
	int thread_started;
	pthread_mutex_t lock;
	pthread_cond_t wake;
	struct loader_job *jobs;
	struct loader_job *answers;
	int stopping;
	int pipe_read;
	int pipe_write;
};

static uint64_t loader_clock(void);
static void *loader_resolver(void *argument);
static int loader_begin(struct net_request *request);
static void loader_answers(struct net_loader *loader);
static void loader_step(struct net_request *request);
static int loader_connect(struct net_request *request);
static int loader_connected(struct net_request *request);
static int loader_handshake(struct net_request *request);
static int loader_send(struct net_request *request);
static int loader_receive(struct net_request *request);
static int loader_finish(struct net_request *request);
static void loader_close(struct net_request *request);
static void loader_end(struct net_request *request, int error);
static void loader_reap(struct net_loader *loader);
static void loader_free(struct net_request *request);
static int loader_prepare(struct net_request *request);
static int loader_retry(struct net_request *request);
static int loader_active(const struct net_loader *loader, const char *key);
static void loader_wake(struct net_loader *loader, const char *key);
static void loader_keep(struct net_request *request);
static void loader_expire_idle(struct net_loader *loader, uint64_t now);
static struct loader_entry *loader_cache_find(struct net_loader *loader, const char *url);
static int loader_cache_fresh(const struct loader_entry *entry, uint64_t now);
static int loader_cache_answer(struct net_request *request, const struct loader_entry *entry);
static void loader_cache_store(struct net_request *request);
static void loader_cache_free(struct net_loader *loader, struct loader_entry *entry);

/*
 * Makes a loader and starts its resolver's thread.
 */
int
net_loader_create(
	struct net_loader **loader)
{
	struct net_loader *made;
	int descriptors[2];
	int status;

	/* The loader, its lock and its condition. */
	*loader = NULL;
	made = calloc(1, sizeof(*made));
	if (made == NULL)
		return ENOMEM;
	made->next_number = 1;
	pthread_mutex_init(&made->lock, NULL);
	pthread_cond_init(&made->wake, NULL);

	/* The pipe the answers are signalled through; the main side never blocks on it. */
	status = pipe2(descriptors, O_CLOEXEC);
	if (status != 0) {
		status = errno;
		free(made);
		return status;
	}

	/* The pipe's two ends; the main side's reads never block. */
	made->pipe_read = descriptors[0];
	made->pipe_write = descriptors[1];
	(void)fcntl(made->pipe_read, F_SETFL, O_NONBLOCK);

	/* The resolver's thread. */
	status = pthread_create(&made->thread, NULL, loader_resolver, made);
	if (status != 0) {
		close(made->pipe_read);
		close(made->pipe_write);
		free(made);
		return status;
	}

	/* The thread runs. */
	made->thread_started = 1;

	/* Succeeded: the loader takes requests. */
	*loader = made;
	return 0;
}

/*
 * Ends a loader: its requests are dropped without their callbacks, and
 * its thread is stopped.
 */
void
net_loader_destroy(
	struct net_loader *loader)
{
	struct net_request *request;
	struct loader_job *job;

	/* Nothing to end. */
	if (loader == NULL)
		return;

	/* Every request. */
	while (loader->requests != NULL) {
		request = loader->requests;
		loader->requests = request->next;
		loader_free(request);
	}

	/* The thread, told to stop. */
	pthread_mutex_lock(&loader->lock);

	loader->stopping = 1;
	pthread_cond_signal(&loader->wake);

	pthread_mutex_unlock(&loader->lock);

	/* It ends after its current lookup. */
	if (loader->thread_started)
		pthread_join(loader->thread, NULL);

	/* The jobs and answers left. */
	while (loader->jobs != NULL) {
		job = loader->jobs;
		loader->jobs = job->next;
		free(job);
	}
	while (loader->answers != NULL) {
		job = loader->answers;
		loader->answers = job->next;
		if (job->addresses != NULL)
			freeaddrinfo(job->addresses);
		free(job);
	}

	/* The idle connections. */
	loader_expire_idle(loader, LOADER_NEVER);

	/* The cache. */
	while (loader->cache != NULL)
		loader_cache_free(loader, loader->cache);

	/* The pipe, then the loader. */
	close(loader->pipe_read);
	close(loader->pipe_write);
	pthread_mutex_destroy(&loader->lock);
	pthread_cond_destroy(&loader->wake);
	free(loader);
}

/*
 * Starts a GET of an http or https URL; done is called with context when
 * it ends (well or not), from net_loader_process.  *request (which may be
 * NULL) is the request, for cancelling it before then.
 */
int
net_loader_fetch(
	struct net_loader *loader,
	const char *url,
	net_request_done done,
	void *context,
	struct net_request **request)
{
	struct net_request *made;
	int is_web;
	int error;

	/* The request. */
	if (request != NULL)
		*request = NULL;
	made = calloc(1, sizeof(*made));
	if (made == NULL)
		return ENOMEM;
	made->loader = loader;
	made->number = loader->next_number;
	loader->next_number++;
	made->descriptor = -1;
	made->done = done;
	made->context = context;
	wb_buffer_init(&made->request);
	wb_buffer_init(&made->raw);
	net_response_init(&made->response);

	/* Its URL, which must be http or https. */
	error = net_url_parse(url, strlen(url), NULL, &made->url);
	if (error != 0) {
		free(made);
		return error;
	}

	/* Only http and https are fetched. */
	is_web = net_http_is_web(made->url.scheme);
	if (!is_web) {
		net_url_release(&made->url);
		free(made);
		return EPROTONOSUPPORT;
	}

	/* Into the loader's list, then its lookup. */
	made->next = loader->requests;
	loader->requests = made;
	error = loader_begin(made);
	if (error != 0) {
		made->cancelled = 1;
		loader_reap(loader);
		return error;
	}

	/* Succeeded: the request runs. */
	if (request != NULL)
		*request = made;
	return 0;
}

/*
 * Cancels a request: its callback is not called, and it is freed (at
 * once, or after the callback that cancels it returns).
 */
void
net_request_cancel(
	struct net_request *request)
{
	/* Nothing to cancel. */
	if (request == NULL)
		return;

	/* Marked, and freed when nothing walks the list; a request waiting for its place may go. */
	request->cancelled = 1;
	loader_close(request);
	loader_wake(request->loader, request->key);
	loader_reap(request->loader);
}

/* Tells how a request ended: 0, or an errno value (EPROTO for TLS, whose reason net_tls_error gives). */
int
net_request_error(
	const struct net_request *request)
{
	/* The request's outcome. */
	return request->error;
}

/* Gives a request's response (its final URL, status, Content-Type and body), valid in its callback. */
const struct net_response *
net_request_response(
	const struct net_request *request)
{
	/* The response the request read. */
	return &request->response;
}

/*
 * Lists the descriptors the caller polls for the loader: the resolver's
 * pipe, then each connection for what it waits for.  Returns how many were
 * written (at most capacity).
 */
size_t
net_loader_poll_fds(
	const struct net_loader *loader,
	struct pollfd *fds,
	size_t capacity)
{
	const struct net_request *request;
	size_t count;

	/* The pipe of the resolver's answers. */
	count = 0;
	if (capacity == 0)
		return 0;
	fds[count].fd = loader->pipe_read;
	fds[count].events = POLLIN;
	fds[count].revents = 0;
	count++;

	/* Each connection that waits for its socket. */
	for (request = loader->requests; request != NULL && count < capacity; request = request->next) {
		if (request->cancelled || request->descriptor < 0 || request->wants == 0)
			continue;
		fds[count].fd = request->descriptor;
		fds[count].events = request->wants;
		fds[count].revents = 0;
		count++;
	}

	/* Succeeded: the descriptors to poll. */
	return count;
}

/*
 * Tells how long the caller may wait for the descriptors before the
 * loader must run anyway (the earliest time out), in milliseconds, or -1
 * when there is no request to time out.
 */
int
net_loader_timeout(
	const struct net_loader *loader)
{
	const struct net_request *request;
	uint64_t earliest;
	uint64_t now;
	int found;

	/* The earliest deadline of a request that runs. */
	found = 0;
	earliest = 0;
	for (request = loader->requests; request != NULL; request = request->next) {
		if (request->cancelled || request->state == LOADER_ENDED || request->state == LOADER_WAITING)
			continue;
		if (request->state == LOADER_READY)
			return 0;
		if (!found || request->deadline < earliest)
			earliest = request->deadline;
		found = 1;
	}

	/* No request, no time out. */
	if (!found)
		return -1;

	/* The time left, none when it has passed. */
	now = loader_clock();
	if (earliest <= now)
		return 0;
	if (earliest - now > 60000U)
		return 60000;
	return (int)(earliest - now);
}

/*
 * Runs the loader: the resolver's answers, the connections whose
 * descriptors are ready (as poll left them in fds), and the time outs.
 * Callbacks run from here.
 */
void
net_loader_process(
	struct net_loader *loader,
	const struct pollfd *fds,
	size_t count)
{
	struct net_request *request;
	uint64_t now;
	size_t index;

	/* The time, and a walk that frees nothing until it ends. */
	now = loader_clock();
	loader->walking++;

	/* The resolver's answers. */
	loader_answers(loader);

	/* The requests the cache answered. */
	for (request = loader->requests; request != NULL; request = request->next) {
		if (!request->cancelled && request->state == LOADER_READY)
			loader_end(request, 0);
	}

	/* The idle connections kept too long. */
	loader_expire_idle(loader, now);

	/* Each ready connection takes its next steps. */
	for (index = 0; index < count; index++) {
		if (fds[index].revents == 0 || fds[index].fd == loader->pipe_read)
			continue;
		for (request = loader->requests; request != NULL; request = request->next) {
			if (request->cancelled || request->descriptor != fds[index].fd)
				continue;
			loader_step(request);
			break;
		}
	}

	/* The requests that made no progress in time. */
	for (request = loader->requests; request != NULL; request = request->next) {
		if (request->cancelled || request->state == LOADER_ENDED || request->state == LOADER_WAITING)
			continue;
		if (now >= request->deadline)
			loader_end(request, ETIMEDOUT);
	}

	/* The walk is over: the ended and cancelled requests go. */
	loader->walking--;
	loader_reap(loader);
}

/*
 * Tells whether a location is one the loader fetches (an http or https
 * URL).
 */
int
net_loader_takes(
	const char *location)
{
	int differs;

	/* http: and https: URLs. */
	differs = strncmp(location, "http://", 7);
	if (differs == 0)
		return 1;
	differs = strncmp(location, "https://", 8);
	if (differs == 0)
		return 1;

	/* Paths and other schemes are read at once. */
	return 0;
}

/* Reads the loader's monotonic clock, in milliseconds. */
static uint64_t
loader_clock(void)
{
	struct timespec now;
	uint64_t milliseconds;

	/* The monotonic clock. */
	clock_gettime(CLOCK_MONOTONIC, &now);
	milliseconds = (uint64_t)now.tv_sec * 1000U + (uint64_t)now.tv_nsec / 1000000U;
	return milliseconds;
}

/* The resolver's thread: each job's lookup, answered through the list and the pipe, until the loader stops. */
static void *
loader_resolver(
	void *argument)
{
	struct net_loader *loader;
	struct loader_job *job;
	struct addrinfo hints;
	unsigned char signal;

	/* Each job, as they come. */
	loader = argument;
	for (;;) {
		/* The next job, waited for. */
		pthread_mutex_lock(&loader->lock);

		while (loader->jobs == NULL && !loader->stopping)
			pthread_cond_wait(&loader->wake, &loader->lock);
		job = loader->jobs;
		if (job != NULL)
			loader->jobs = job->next;

		pthread_mutex_unlock(&loader->lock);

		/* A loader that stops ends the thread. */
		if (job == NULL)
			break;

		/* The lookup, which may take long. */
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		job->addresses = NULL;
		job->status = getaddrinfo(job->host, job->port, &hints, &job->addresses);

		/* The answer, then a byte through the pipe to wake the main loop. */
		pthread_mutex_lock(&loader->lock);

		job->next = loader->answers;
		loader->answers = job;

		pthread_mutex_unlock(&loader->lock);

		/* The byte. */
		signal = 1;
		(void)write(loader->pipe_write, &signal, 1);
	}

	/* The loader stopped. */
	return NULL;
}

/*
 * Starts a request at its URL: answered by the cache when it holds the URL
 * fresh; else over a kept connection to the place, or, when fewer than
 * LOADER_PER_HOST are open there, over a new one (the host's lookup given
 * to the resolver's thread); else it waits for one to end.
 */
static int
loader_begin(
	struct net_request *request)
{
	struct net_loader *loader;
	struct loader_connection **link;
	struct loader_connection *kept;
	struct loader_entry *entry;
	struct loader_job *job;
	struct wb_buffer url;
	size_t length;
	uint64_t now;
	int fresh;
	int active;
	int error;
	int port;

	/* The place the request goes to. */
	loader = request->loader;
	now = loader_clock();
	port = net_url_default_port(request->url.scheme);
	if (request->url.port >= 0)
		port = request->url.port;
	snprintf(request->key, sizeof(request->key), "%s://%s:%d", request->url.scheme, request->url.host, port);

	/* A fresh answer of the cache ends the request at the next run of the loader. */
	wb_buffer_init(&url);
	error = net_url_serialize(&request->url, 1, &url);
	if (error != 0) {
		wb_buffer_release(&url);
		return error;
	}

	/* The URL's entry, answering when it is fresh. */
	entry = loader_cache_find(loader, wb_buffer_string(&url));
	wb_buffer_release(&url);
	fresh = loader_cache_fresh(entry, now);
	if (fresh) {
		error = loader_cache_answer(request, entry);
		if (error != 0)
			return error;
		request->state = LOADER_READY;
		request->wants = 0;
		return 0;
	}

	/* A stale entry with an ETag is revalidated. */
	request->validating = NULL;
	if (entry != NULL && entry->etag.length != 0)
		request->validating = entry;

	/* A kept connection to the place, when there is one (not after a kept one failed). */
	for (link = &loader->idle; *link != NULL && !request->retried; link = &(*link)->next) {
		error = strcmp((*link)->key, request->key);
		if (error == 0)
			break;
	}

	/* One is there: the request takes it. */
	if (!request->retried && *link != NULL) {
		/* Taken out of the pool. */
		kept = *link;
		*link = kept->next;
		request->descriptor = kept->descriptor;
		request->tls = kept->tls;
		request->reused = 1;
		free(kept);

		/* The request goes out over it. */
		error = loader_prepare(request);
		if (error != 0)
			return error;
		request->state = LOADER_SENDING;
		request->wants = POLLOUT;
		return 0;
	}

	/* A place with all its connections open: the request waits for one to end. */
	request->reused = 0;
	active = loader_active(loader, request->key);
	if (active >= LOADER_PER_HOST) {
		request->state = LOADER_WAITING;
		request->wants = 0;
		request->deadline = LOADER_NEVER;
		return 0;
	}

	/* The job: the host without an IPv6 address's brackets, and the port or the scheme's. */
	job = calloc(1, sizeof(*job));
	if (job == NULL)
		return ENOMEM;
	job->number = request->number;
	length = strlen(request->url.host);
	if (length >= sizeof(job->host)) {
		free(job);
		return ENAMETOOLONG;
	}

	/* The host, without an IPv6 address's brackets. */
	memcpy(job->host, request->url.host, length + 1U);
	if (length >= 2U && job->host[0] == '[') {
		memmove(job->host, job->host + 1, length - 2U);
		job->host[length - 2U] = '\0';
	}

	/* The port, or the scheme's. */
	snprintf(job->port, sizeof(job->port), "%d", port);

	/* The request waits for the answer, for as long as a request may be silent. */
	request->state = LOADER_RESOLVING;
	request->wants = 0;
	request->deadline = loader_clock() + LOADER_TIMEOUT;

	/* The job to the thread. */
	pthread_mutex_lock(&loader->lock);

	job->next = loader->jobs;
	loader->jobs = job;
	pthread_cond_signal(&loader->wake);

	pthread_mutex_unlock(&loader->lock);

	/* Succeeded: the lookup is under way. */
	return 0;
}

/* Takes the resolver's answers: each request that waits for one starts to connect (or fails). */
static void
loader_answers(
	struct net_loader *loader)
{
	struct loader_job *answers;
	struct loader_job *job;
	struct net_request *request;
	unsigned char drained[64];
	ssize_t got;
	int error;

	/* The pipe's bytes, all of them. */
	do {
		got = read(loader->pipe_read, drained, sizeof(drained));
	} while (got > 0);

	/* The answers, taken at once. */
	pthread_mutex_lock(&loader->lock);

	answers = loader->answers;
	loader->answers = NULL;

	pthread_mutex_unlock(&loader->lock);

	/* Each answer to its request, when it is still there and waiting. */
	while (answers != NULL) {
		job = answers;
		answers = job->next;
		for (request = loader->requests; request != NULL; request = request->next) {
			if (request->number == job->number)
				break;
		}

		/* An answer nobody waits for any more. */
		if (request == NULL || request->cancelled || request->state != LOADER_RESOLVING) {
			if (job->addresses != NULL)
				freeaddrinfo(job->addresses);
			free(job);
			continue;
		}

		/* A host that has no address fails the request. */
		if (job->status != 0 || job->addresses == NULL) {
			if (job->addresses != NULL)
				freeaddrinfo(job->addresses);
			free(job);
			loader_end(request, EHOSTUNREACH);
			continue;
		}

		/* The addresses, tried in turn from the first. */
		request->addresses = job->addresses;
		request->address = job->addresses;
		free(job);
		request->state = LOADER_CONNECTING;
		error = loader_connect(request);
		if (error != 0)
			loader_end(request, error);

		/* The connection goes on when its socket becomes writable. */
	}
}

/* Takes a request as far as its socket allows, ending it when it is done or fails. */
static void
loader_step(
	struct net_request *request)
{
	int error;

	/* The step of each state, until one must wait (EAGAIN) or the request ends. */
	error = 0;
	while (error == 0 && !request->cancelled && request->state != LOADER_ENDED) {
		/* Decide by the state. */
		switch (request->state) {
		case LOADER_CONNECTING:
			error = loader_connected(request);
			break;
		case LOADER_HANDSHAKE:
			error = loader_handshake(request);
			break;
		case LOADER_SENDING:
			error = loader_send(request);
			break;
		case LOADER_RECEIVING:
			error = loader_receive(request);
			break;
		default:
			error = EAGAIN;
			break;
		}
	}

	/* Waiting is not a failure; anything else ends the request. */
	if (error != 0 && error != EAGAIN)
		loader_end(request, error);
}

/*
 * Starts a non-blocking connection to the request's current address,
 * moving past the addresses that cannot even be tried.
 */
static int
loader_connect(
	struct net_request *request)
{
	struct addrinfo *address;
	int status;
	int error;

	/* The addresses from the current one. */
	error = ECONNREFUSED;
	for (address = request->address; address != NULL; address = address->ai_next) {
		request->address = address;
		request->descriptor = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
		if (request->descriptor < 0) {
			error = errno;
			continue;
		}

		/* The socket does not block, and its connection starts. */
		(void)fcntl(request->descriptor, F_SETFL, O_NONBLOCK);
		status = connect(request->descriptor, address->ai_addr, address->ai_addrlen);
		if (status == 0 || errno == EINPROGRESS) {
			request->wants = POLLOUT;
			request->deadline = loader_clock() + LOADER_TIMEOUT;
			return 0;
		}

		/* An address that refuses at once gives way to the next. */
		error = errno;
		close(request->descriptor);
		request->descriptor = -1;
	}

	/* No address could be tried. */
	return error;
}

/*
 * Checks a connection that was under way: connected, it goes on to TLS or
 * the request; refused, the next address is tried.
 */
static int
loader_connected(
	struct net_request *request)
{
	struct sockaddr_storage peer;
	socklen_t size;
	int failure;
	int status;
	int is_https;
	int error;

	/* The connection's outcome. */
	failure = 0;
	size = sizeof(failure);
	status = getsockopt(request->descriptor, SOL_SOCKET, SO_ERROR, &failure, &size);
	if (status != 0)
		failure = errno;
	if (failure == EINPROGRESS || failure == EALREADY)
		return EAGAIN;

	/* No error yet is not a connection yet: a connection has a peer. */
	if (failure == 0) {
		size = sizeof(peer);
		status = getpeername(request->descriptor, (struct sockaddr *)&peer, &size);
		if (status != 0 && errno == ENOTCONN)
			return EAGAIN;
	}

	/* A refused or failed connection moves to the next address. */
	if (failure != 0) {
		close(request->descriptor);
		request->descriptor = -1;
		request->address = request->address->ai_next;
		if (request->address == NULL)
			return failure;
		error = loader_connect(request);
		if (error != 0)
			return error;
		return EAGAIN;
	}

	/* The addresses are no longer needed. */
	freeaddrinfo(request->addresses);
	request->addresses = NULL;
	request->address = NULL;

	/* The request's text. */
	error = loader_prepare(request);
	if (error != 0)
		return error;

	/* https starts TLS over the connection; http sends at once. */
	is_https = !strcmp(request->url.scheme, "https");
	if (is_https) {
		net_tls_clear_error();
		error = net_tls_start(request->descriptor, request->url.host, &request->tls);
		if (error != 0)
			return error;
		request->state = LOADER_HANDSHAKE;
		return 0;
	}

	/* Succeeded: the request can be sent. */
	request->state = LOADER_SENDING;
	return 0;
}

/* Takes the TLS handshake a step. */
static int
loader_handshake(
	struct net_request *request)
{
	int error;

	/* The step; waiting says what for. */
	error = net_tls_handshake(request->tls, &request->wants);
	if (error != 0)
		return error;

	/* Succeeded: the connection is secure, and the request can be sent. */
	request->state = LOADER_SENDING;
	request->deadline = loader_clock() + LOADER_TIMEOUT;
	return 0;
}

/* Sends as much of the request as the connection takes now. */
static int
loader_send(
	struct net_request *request)
{
	const unsigned char *bytes;
	size_t length;
	size_t sent;
	ssize_t count;
	int error;

	/* What is left to send. */
	bytes = request->request.data + request->sent;
	length = request->request.length - request->sent;
	sent = 0;

	/* Over TLS, or the socket. */
	if (request->tls != NULL) {
		error = net_tls_write_some(request->tls, bytes, length, &sent, &request->wants);
		if (error != 0)
			return error;
	} else {
		count = send(request->descriptor, bytes, length, MSG_NOSIGNAL);
		if (count < 0 && (errno == EAGAIN || errno == EINTR)) {
			request->wants = POLLOUT;
			return EAGAIN;
		}

		/* A kept connection the server closed is replaced once. */
		if (count < 0 && request->reused) {
			error = loader_retry(request);
			return error;
		}

		/* Any other failure ends the request. */
		if (count < 0)
			return errno;
		sent = (size_t)count;
	}

	/* The bytes went; all of them means the response comes next. */
	request->sent += sent;
	request->deadline = loader_clock() + LOADER_TIMEOUT;
	if (request->sent < request->request.length)
		return 0;

	/* Succeeded: the request is sent, and the response is read. */
	request->state = LOADER_RECEIVING;
	request->wants = POLLIN;
	return 0;
}

/* Reads what the connection has now; its end finishes the response. */
static int
loader_receive(
	struct net_request *request)
{
	unsigned char chunk[LOADER_CHUNK];
	size_t received;
	ssize_t count;
	int error;

	/* Over TLS, or the socket. */
	received = 0;
	if (request->tls != NULL) {
		error = net_tls_read_some(request->tls, chunk, sizeof(chunk), &received, &request->wants);
		if (error != 0)
			return error;
	} else {
		count = recv(request->descriptor, chunk, sizeof(chunk), 0);
		if (count < 0 && (errno == EAGAIN || errno == EINTR)) {
			request->wants = POLLIN;
			return EAGAIN;
		}

		/* Any other failure ends the request. */
		if (count < 0)
			return errno;
		received = (size_t)count;
	}

	/* A kept connection closed before anything of the response came is replaced once. */
	if (received == 0 && request->reused && request->raw.length == 0) {
		error = loader_retry(request);
		return error;
	}

	/* The end of the connection is the end of the response. */
	if (received == 0) {
		error = loader_finish(request);
		return error;
	}

	/* The bytes, within the largest response. */
	if (request->raw.length + received > LOADER_MAX_RESPONSE)
		return EFBIG;
	error = wb_buffer_append(&request->raw, chunk, received);
	if (error != 0)
		return error;

	/* A response its framing says is complete ends here, its connection kept when it may be. */
	request->deadline = loader_clock() + LOADER_TIMEOUT;
	net_http_framing_update(&request->framing, request->raw.data, request->raw.length);
	if (request->framing.done) {
		error = loader_finish(request);
		return error;
	}

	/* Succeeded: more may come. */
	return 0;
}

/*
 * Finishes a response read to its end: parsed, it either ends the request
 * or, as a redirect with a Location, starts it again there.
 */
static int
loader_finish(
	struct net_request *request)
{
	struct wb_buffer location;
	struct net_url next;
	int redirected;
	int is_web;
	int error;

	/* The connection goes back to the pool when the response left it open, else it is closed. */
	if (request->framing.done && request->framing.keep_alive)
		loader_keep(request);
	loader_close(request);

	/* The response, with nothing of the last one's caching left. */
	wb_buffer_clear(&request->response.content_type);
	wb_buffer_clear(&request->response.body);
	wb_buffer_clear(&request->response.etag);
	request->response.max_age = -1;
	request->response.no_store = 0;
	request->response.no_cache = 0;
	wb_buffer_init(&location);
	error = net_http_parse_response(&request->url, &request->raw, &request->response, &location);
	wb_buffer_clear(&request->raw);
	if (error != 0) {
		wb_buffer_release(&location);
		return error;
	}

	/* A 304 to a revalidation is the kept body, fresh again. */
	if (request->response.status == 304 && request->validating != NULL) {
		request->validating->stored_at = loader_clock();
		if (request->response.max_age >= 0)
			request->validating->max_age = request->response.max_age;
		error = loader_cache_answer(request, request->validating);
		if (error != 0) {
			wb_buffer_release(&location);
			return error;
		}
	}

	/* A response that is not a redirect with a place to go ends the request at this URL. */
	redirected = net_http_is_redirect(request->response.status);
	if (!redirected || location.length == 0) {
		wb_buffer_release(&location);
		wb_buffer_clear(&request->response.url);
		error = net_url_serialize(&request->url, 0, &request->response.url);
		if (error != 0)
			return error;
		loader_cache_store(request);
		loader_end(request, 0);
		return 0;
	}

	/* Too many redirects. */
	if (request->redirects == LOADER_REDIRECTS) {
		wb_buffer_release(&location);
		return ELOOP;
	}

	/* The next URL, resolved against this one, which must be http or https. */
	error = net_url_parse(wb_buffer_string(&location), location.length, &request->url, &next);
	wb_buffer_release(&location);
	if (error != 0)
		return error;
	is_web = net_http_is_web(next.scheme);
	if (!is_web) {
		net_url_release(&next);
		return EPROTONOSUPPORT;
	}

	/* The request starts again there. */
	net_url_release(&request->url);
	request->url = next;
	request->redirects++;
	error = loader_begin(request);
	if (error != 0)
		return error;

	/* Succeeded: the lookup of the next host waits (the step loop stops here). */
	return EAGAIN;
}

/* Closes a request's connection and frees its addresses (it may start another). */
static void
loader_close(
	struct net_request *request)
{
	/* TLS, then the socket. */
	if (request->tls != NULL)
		net_tls_close(request->tls);
	request->tls = NULL;
	if (request->descriptor >= 0)
		close(request->descriptor);
	request->descriptor = -1;
	request->wants = 0;

	/* The addresses of the lookup. */
	if (request->addresses != NULL)
		freeaddrinfo(request->addresses);
	request->addresses = NULL;
	request->address = NULL;
}

/*
 * Ends a request with an outcome: its connection closed and its callback
 * called (the request is freed after the walk that ended it).
 */
static void
loader_end(
	struct net_request *request,
	int error)
{
	/* Ended once. */
	if (request->state == LOADER_ENDED || request->cancelled)
		return;
	loader_close(request);
	request->state = LOADER_ENDED;
	request->error = error;

	/* A request waiting for its place may go. */
	loader_wake(request->loader, request->key);

	/* The callback, while nothing is freed. */
	request->loader->walking++;
	if (request->done != NULL)
		request->done(request->context, request);
	request->loader->walking--;
}

/* Frees the ended and cancelled requests, unless the list is being walked. */
static void
loader_reap(
	struct net_loader *loader)
{
	struct net_request **link;
	struct net_request *request;

	/* Nothing is freed during a walk. */
	if (loader->walking != 0)
		return;

	/* Each request that is over leaves the list. */
	link = &loader->requests;
	while (*link != NULL) {
		request = *link;
		if (request->state != LOADER_ENDED && !request->cancelled) {
			link = &request->next;
			continue;
		}

		/* Unlinked and freed. */
		*link = request->next;
		loader_free(request);
	}
}

/* Frees a request and what it holds. */
static void
loader_free(
	struct net_request *request)
{
	/* The connection, the URL and the buffers. */
	loader_close(request);
	net_url_release(&request->url);
	wb_buffer_release(&request->request);
	wb_buffer_release(&request->raw);
	net_response_release(&request->response);
	free(request);
}

/*
 * Prepares a request to go out over its connection: its text (keeping the
 * connection, and If-None-Match for a revalidation), nothing sent or read
 * yet, and a new time out.
 */
static int
loader_prepare(
	struct net_request *request)
{
	struct wb_buffer extra;
	const char *lines;
	int error;

	/* If-None-Match with the kept response's ETag, for a revalidation. */
	wb_buffer_init(&extra);
	lines = NULL;
	if (request->validating != NULL) {
		error = wb_buffer_printf(&extra, "If-None-Match: %s\r\n", wb_buffer_string(&request->validating->etag));
		if (error != 0) {
			wb_buffer_release(&extra);
			return error;
		}

		/* The line goes after the fixed headers. */
		lines = wb_buffer_string(&extra);
	}

	/* The text, keeping the connection. */
	wb_buffer_clear(&request->request);
	error = net_http_request_text(&request->url, &request->request, 1, lines);
	wb_buffer_release(&extra);
	if (error != 0)
		return error;

	/* Nothing sent or read, and a new time out. */
	request->sent = 0;
	wb_buffer_clear(&request->raw);
	net_http_framing_init(&request->framing);
	request->deadline = loader_clock() + LOADER_TIMEOUT;

	/* Succeeded: the request can go out. */
	return 0;
}

/*
 * Replaces a kept connection that turned out closed by a new connection
 * (once): the request starts again from its lookup.
 */
static int
loader_retry(
	struct net_request *request)
{
	int error;

	/* A request whose kept connection failed twice fails. */
	if (request->retried)
		return ECONNRESET;

	/* The old connection goes, and the request starts again without the pool. */
	loader_close(request);
	request->retried = 1;
	request->reused = 0;
	error = loader_begin(request);
	if (error != 0)
		return error;

	/* Succeeded: the request waits for its new connection. */
	return EAGAIN;
}

/* Counts the requests that hold or are opening a connection to a place. */
static int
loader_active(
	const struct net_loader *loader,
	const char *key)
{
	const struct net_request *request;
	int count;
	int differs;

	/* Each request that is past waiting and not over. */
	count = 0;
	for (request = loader->requests; request != NULL; request = request->next) {
		if (request->cancelled || request->state == LOADER_ENDED)
			continue;
		if (request->state == LOADER_READY || request->state == LOADER_WAITING)
			continue;
		differs = strcmp(request->key, key);
		if (differs == 0)
			count++;
	}

	/* The count. */
	return count;
}

/* Starts the first request that waits for a place, when the place has room again. */
static void
loader_wake(
	struct net_loader *loader,
	const char *key)
{
	struct net_request *request;
	int differs;
	int error;

	/* A place without a key (a request that never started) frees nothing. */
	if (key[0] == '\0')
		return;

	/* The first request waiting for the place starts; one that cannot fails. */
	for (request = loader->requests; request != NULL; request = request->next) {
		if (request->cancelled || request->state != LOADER_WAITING)
			continue;
		differs = strcmp(request->key, key);
		if (differs != 0)
			continue;
		error = loader_begin(request);
		if (error != 0)
			loader_end(request, error);
		break;
	}
}

/* Puts a request's connection in the pool of idle connections (the request no longer holds it). */
static void
loader_keep(
	struct net_request *request)
{
	struct net_loader *loader;
	struct loader_connection *kept;

	/* The pool's entry; without memory the connection is simply closed. */
	loader = request->loader;
	kept = calloc(1, sizeof(*kept));
	if (kept == NULL)
		return;
	memcpy(kept->key, request->key, sizeof(kept->key));
	kept->descriptor = request->descriptor;
	kept->tls = request->tls;
	kept->idle_since = loader_clock();

	/* The connection is the pool's now. */
	request->descriptor = -1;
	request->tls = NULL;
	request->wants = 0;
	kept->next = loader->idle;
	loader->idle = kept;
}

/* Closes the idle connections that have been idle since before a time (LOADER_NEVER: all of them). */
static void
loader_expire_idle(
	struct net_loader *loader,
	uint64_t now)
{
	struct loader_connection **link;
	struct loader_connection *kept;

	/* Each idle connection. */
	link = &loader->idle;
	while (*link != NULL) {
		kept = *link;
		if (now != LOADER_NEVER && kept->idle_since + LOADER_IDLE > now) {
			link = &kept->next;
			continue;
		}

		/* Unlinked and closed. */
		*link = kept->next;
		if (kept->tls != NULL)
			net_tls_close(kept->tls);
		close(kept->descriptor);
		free(kept);
	}
}

/* Finds a URL's entry in the cache, and makes it the most recently used. */
static struct loader_entry *
loader_cache_find(
	struct net_loader *loader,
	const char *url)
{
	struct loader_entry **link;
	struct loader_entry *entry;
	int differs;

	/* Each entry. */
	for (link = &loader->cache; *link != NULL; link = &(*link)->next) {
		differs = strcmp((*link)->url, url);
		if (differs != 0)
			continue;

		/* Moved to the front. */
		entry = *link;
		*link = entry->next;
		entry->next = loader->cache;
		loader->cache = entry;
		return entry;
	}

	/* The URL is not kept. */
	return NULL;
}

/* Tells whether an entry answers without the network: kept for less than its max-age, and not no-cache. */
static int
loader_cache_fresh(
	const struct loader_entry *entry,
	uint64_t now)
{
	/* No entry, no max-age, or no-cache. */
	if (entry == NULL || entry->max_age <= 0 || entry->no_cache)
		return 0;

	/* Younger than its max-age. */
	if (now < entry->stored_at + (uint64_t)entry->max_age * 1000U)
		return 1;

	/* Stale. */
	return 0;
}

/* Answers a request from a cache entry: a 200 with its Content-Type and body. */
static int
loader_cache_answer(
	struct net_request *request,
	const struct loader_entry *entry)
{
	int error;

	/* The response the entry keeps, at the request's URL. */
	request->response.status = 200;
	wb_buffer_clear(&request->response.content_type);
	wb_buffer_clear(&request->response.body);
	wb_buffer_clear(&request->response.url);
	error = wb_buffer_append(&request->response.content_type, entry->content_type.data, entry->content_type.length);
	if (error == 0)
		error = wb_buffer_append(&request->response.body, entry->body.data, entry->body.length);
	if (error == 0)
		error = net_url_serialize(&request->url, 0, &request->response.url);
	if (error != 0)
		return error;

	/* Succeeded: the request has its response. */
	return 0;
}

/*
 * Keeps a request's final 200 response in the cache when it may be kept
 * (a max-age or an ETag, and not no-store), in place of an older one of
 * the same URL; the least recently used go when the bodies are too many.
 */
static void
loader_cache_store(
	struct net_request *request)
{
	struct net_loader *loader;
	struct loader_entry *entry;
	struct wb_buffer url;
	int error;

	/* Only a 200 that says it may be kept, and not larger than the cache. */
	loader = request->loader;
	if (request->response.status != 200 || request->response.no_store)
		return;
	if (request->response.max_age <= 0 && request->response.etag.length == 0)
		return;
	if (request->response.body.length > LOADER_CACHE_MAX)
		return;

	/* The URL, the key; an older response of it goes. */
	wb_buffer_init(&url);
	error = net_url_serialize(&request->url, 1, &url);
	if (error != 0) {
		wb_buffer_release(&url);
		return;
	}

	/* An older entry of the URL is replaced (a revalidated one stays). */
	entry = loader_cache_find(loader, wb_buffer_string(&url));
	if (entry != NULL && entry != request->validating)
		loader_cache_free(loader, entry);
	if (entry != NULL && entry == request->validating) {
		/* A revalidated entry was refreshed already. */
		wb_buffer_release(&url);
		return;
	}

	/* The entry. */
	entry = calloc(1, sizeof(*entry));
	if (entry == NULL) {
		wb_buffer_release(&url);
		return;
	}

	/* Its URL, type, body, ETag, time and freshness. */
	entry->url = strdup(wb_buffer_string(&url));
	wb_buffer_release(&url);
	wb_buffer_init(&entry->content_type);
	wb_buffer_init(&entry->body);
	wb_buffer_init(&entry->etag);
	error = ENOMEM;
	if (entry->url != NULL)
		error = wb_buffer_append(&entry->content_type, request->response.content_type.data, request->response.content_type.length);
	if (error == 0)
		error = wb_buffer_append(&entry->body, request->response.body.data, request->response.body.length);
	if (error == 0)
		error = wb_buffer_append(&entry->etag, request->response.etag.data, request->response.etag.length);
	entry->stored_at = loader_clock();
	entry->max_age = request->response.max_age;
	entry->no_cache = request->response.no_cache;

	/* At the front; an entry that could not be made is dropped. */
	entry->next = loader->cache;
	loader->cache = entry;
	loader->cache_bytes += entry->body.length;
	if (error != 0) {
		loader_cache_free(loader, entry);
		return;
	}

	/* The least recently used go until the bodies fit. */
	while (loader->cache_bytes > LOADER_CACHE_MAX) {
		entry = loader->cache;
		while (entry->next != NULL)
			entry = entry->next;
		loader_cache_free(loader, entry);
	}
}

/* Takes an entry out of the cache and frees it (no request revalidates it any more). */
static void
loader_cache_free(
	struct net_loader *loader,
	struct loader_entry *entry)
{
	struct loader_entry **link;
	struct net_request *request;

	/* Unlinked. */
	for (link = &loader->cache; *link != NULL; link = &(*link)->next) {
		if (*link == entry) {
			*link = entry->next;
			break;
		}
	}

	/* The requests that revalidate it no longer do. */
	for (request = loader->requests; request != NULL; request = request->next) {
		if (request->validating == entry)
			request->validating = NULL;
	}

	/* Its bytes, then it. */
	loader->cache_bytes -= entry->body.length;
	free(entry->url);
	wb_buffer_release(&entry->content_type);
	wb_buffer_release(&entry->body);
	wb_buffer_release(&entry->etag);
	free(entry);
}
