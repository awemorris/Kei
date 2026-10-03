/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Implements the zedBSD init userland command.
 */

#include "userland/base/service/service-config.h"
#include "userland/base/service/rcconf.h"
#include "userland/base/service/zsv1-server.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <uapi/system.h>

#define SERVICE_MAX 32
#define ARGUMENT_MAX 16

/* Requests held while a oneshot service runs, and how often init looks at the socket and the service then. */
#define DEFERRED_MAX 8
#define ONESHOT_POLL_MS 100

enum service_type { SERVICE_DAEMON, SERVICE_ONESHOT, SERVICE_RESPAWN };

enum service_state {
	SERVICE_STOPPED,
	SERVICE_STARTING,
	SERVICE_RUNNING,
	SERVICE_COMPLETED,
	SERVICE_FAILED,
	SERVICE_SKIPPED
};

enum init_action {
	INIT_ACTION_NONE,
	INIT_ACTION_HALT,
	INIT_ACTION_POWEROFF,
	INIT_ACTION_REBOOT
};

struct service {
	char name[64];
	char command[256];
	char arguments[512];
	char after[256];
	char
		requires[
		    256];
	enum service_type type;
	enum service_state state;
	pid_t pid;
	int enabled;
	int required;
	int restart_always;
	int restart_failure;
	int notify_fd3;
	unsigned notify_timeout;
	unsigned failures;
	/*
	 * The service this one stands in for (replaces=, ws035-p098): while
	 * this one is enabled the other is not started; when this one ends and
	 * is not started again, the other is.  The graphical login's greeter
	 * replaces the console's getty this way.
	 */
	char replaces[64];
};

static struct service services[SERVICE_MAX];
static size_t service_count;
static volatile sig_atomic_t reload_requested;
static volatile sig_atomic_t action_requested;

/* Set when the system is being stopped: a replaced service is not started then. */
static int services_stopping;

/*
 * The control socket's listener (-1 without one).  It is served while a
 * oneshot service runs too (BUG-095): a oneshot that asks for the power to be
 * cut, through /sbin/poweroff, waits for init's answer while init waits for it.
 */
static int control_listener = -1;

/*
 * A request that starts or stops services, received while a oneshot service
 * ran: it is carried out when init is back in its loop, as it was when such a
 * request waited in the listen queue.
 */
struct deferred_request {
	int client;
	struct zsv1_request request;
};

static struct deferred_request deferred_requests[DEFERRED_MAX];
static size_t deferred_count;

enum dependency_result {
	DEPENDENCIES_WAIT,
	DEPENDENCIES_READY,
	DEPENDENCIES_SKIP
};

static void make_runtime_directories(void);
static struct rcconf_model *load_rcconf_snapshot(void);
static void set_configured_hostname(const struct rcconf_model *snapshot);
static void run_startup_command(const char *path, char *name);
static int load_services(const struct rcconf_model *snapshot);
static int load_one_service(const char *name, const struct rcconf_model *snapshot);
static void report_unknown_services(const struct rcconf_model *snapshot);
static int yes(const char *value);
static int on_off(const char *value, int *result);
static int parse_seconds(const char *value, unsigned minimum, unsigned maximum, unsigned *result);
static int open_control_socket(void);
static void start_enabled_services(void);
static enum dependency_result dependencies_state(const struct service *service);
static struct service *find_service(const char *name);
static struct service *replacer_of(const struct service *service);
static void release_replaced(const struct service *service);
static int terminal_state(enum service_state state);
static int spawn_service(struct service *service);
static int wait_for_oneshot(pid_t child, int *status);
static void serve_while_oneshot(void);
static void handle_deferred_requests(void);
static int wait_for_notification(struct service *service, int descriptor);
static uint64_t monotonic_milliseconds(void);
static int valid_failure_record(const char *record);
static void reap_children(void);
static void shutdown_system(enum init_action action);
static int stop_service(struct service *service);
static int reload_policy(void);
static void handle_request(int client);
static void refuse_request(int client, int error);
static void dispatch_request(int client, const struct zsv1_request *request);
static int receive_request(int client, struct zsv1_request *request);
static int send_service(int client, const struct service *service);
static enum zsv1_service_state zsv1_state(enum service_state state);
static int send_dependencies(int client, const char *list, enum zsv1_record_type type);
static void signal_handler(int number);

/*
 * Runs the init command.
 */
int
main(
	void)
{
	int client;
	struct rcconf_model *snapshot;

	/* Handles a failed getpid operation. */
	if (getpid() != 1) {
		fprintf(stderr, "init: must run as process 1\n");

		/* Reports operation failure. */
		return 1;
	}

	(void)signal(SIGHUP, signal_handler);
	(void)signal(SIGINT, signal_handler);
	(void)signal(SIGTERM, signal_handler);
	(void)signal(SIGCHLD, SIG_DFL);

	make_runtime_directories();
	snapshot = load_rcconf_snapshot();

	/* Handles the snapshot availability. */
	if (snapshot == NULL) {
		fprintf(stderr, "init: cannot load %s: %s\n", RCCONF_PATH,
			strerror(errno));
	}
	set_configured_hostname(snapshot);
	run_startup_command("/sbin/mount", "mount");
	run_startup_command("/sbin/swapon", "swapon");

	/* Handles a failed load services operation. */
	if (load_services(snapshot) != 0) {
		fprintf(stderr,
			"init: continuing without service definitions\n");
	}
	report_unknown_services(snapshot);
	free(snapshot);

	/* Opens the control socket. */
	control_listener = open_control_socket();

	/* Handles the listener condition. */
	if (control_listener < 0)
		fprintf(stderr, "init: control socket: %s\n", strerror(errno));

	start_enabled_services();

	printf("init: system running\n");

	/* Continue until the operation reaches a terminal state. */
	for (;;) {
		reap_children();

		/* Handles the action requested condition. */
		if (action_requested != 0)
			shutdown_system(action_requested);

		/* Handles the reload requested condition. */
		if (reload_requested) {
			reload_requested = 0;

			/* Handles a failed reload policy operation. */
			if (reload_policy() == 0) {
				/*
 * Runtime instances are deliberately preserved.
				 */
				printf("init: configuration reloaded\n");
			}
		}

		/* Carries out the requests held while a oneshot service ran. */
		handle_deferred_requests();

		/* Handles the listener condition. */
		if (control_listener < 0) {
			sleep(1);
			continue;
		}

		/* Answers the next request. */
		client = accept4(control_listener, NULL, NULL, SOCK_CLOEXEC);
		if (client >= 0) {
			handle_request(client);
			close(client);
			continue;
		}

		sleep(1);
	}
}

/* Supports the make runtime directories operation. */
static void
make_runtime_directories(
	void)
{
	(void)mkdir("/var", 0755);
	(void)mkdir("/run", 0755);
	(void)mkdir("/var/log", 0755);
}

/* Supports the load rcconf snapshot operation. */
static struct rcconf_model *
load_rcconf_snapshot(
	void)
{
	struct rcconf_model *snapshot;
	int error;

	snapshot = malloc(sizeof(*snapshot));

	/* Handles the snapshot availability. */
	if (snapshot == NULL)
		return NULL;

	/* Handles a failed rcconf load operation. */
	if (rcconf_load(RCCONF_PATH, snapshot) == 0)
		return snapshot;
	error = errno;
	free(snapshot);
	errno = error;

	/* Reports that no result is available. */
	return NULL;
}

/* Supports the set configured hostname operation. */
static void
set_configured_hostname(
	const struct rcconf_model *snapshot)
{
	/* Handles a failed sethostname operation. */
	if (snapshot != NULL && snapshot->hostname[0] != '\0' &&
	    sethostname(snapshot->hostname, strlen(snapshot->hostname)) != 0)
		fprintf(stderr, "init: sethostname: %s\n", strerror(errno));
}

/* Runs filesystem setup in order while retaining console recovery on failure. */
static void
run_startup_command(
	const char *path,
	char *name)
{
	pid_t child;
	pid_t waited;
	int status;
	char *arguments[] = {name, "-a", NULL};

	child = fork();
	if (child < 0) {
		fprintf(stderr, "init: %s -a: fork: %s\n", name, strerror(errno));
		return;
	}
	if (child == 0) {
		execv(path, arguments);
		fprintf(stderr, "init: exec %s: %s\n", path,
			strerror(errno));
		_exit(127);
	}

	/* Signals must not let service startup race an unfinished setup command. */
	do {
		waited = waitpid(child, &status, 0);
	} while (waited < 0 && errno == EINTR);
	if (waited != child) {
		fprintf(stderr, "init: %s -a: wait: %s\n", name, strerror(errno));
		return;
	}
	/* Says how it failed: an exit status, or the signal that killed it. */
	if (WIFSIGNALED(status))
		fprintf(stderr, "init: %s -a killed by signal %d\n", name,
			WTERMSIG(status));
	else if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
		fprintf(stderr, "init: %s -a failed (status %d)\n", name,
			WIFEXITED(status) ? WEXITSTATUS(status) : -1);
}

/* Supports the load services operation. */
static int
load_services(
	const struct rcconf_model *snapshot)
{
	DIR *directory;
	struct dirent *entry;

	service_count = 0;

	directory = opendir("/etc/service.d");

	/* Handles the directory availability. */
	if (directory == NULL) {
		fprintf(stderr, "init: /etc/service.d: %s\n", strerror(errno));

		/* Reports operation failure. */
		return -1;
	}

	while ((entry = readdir(directory)) != NULL) {
		/* Handles the entry condition. */
		if (entry->d_name[0] == '.')
			continue;

		/* Handles a failed load one service operation. */
		if (load_one_service(entry->d_name, snapshot) != 0) {
			fprintf(stderr,
				"init: invalid service definition: %s\n",
				entry->d_name);
		}
	}

	closedir(directory);

	/* Reports successful completion. */
	return 0;
}

/*
 * Supports the report unknown services operation.
 *
 * A configuration that names a service with no definition has usually been
 * written wrongly, and saying so is more use than silence.  One written
 * down as optional is expected to be absent whenever the package that
 * carries it was left out, so that one is passed over without a word.
 */
static void
report_unknown_services(
	const struct rcconf_model *snapshot)
{
	const struct rcconf_service *configured;
	size_t index;
	size_t loaded;
	int found;

	/* Handles the snapshot availability. */
	if (snapshot == NULL)
		return;

	/* Process each element required by the operation. */
	for (index = 0; index < snapshot->service_count; index++) {
		configured = &snapshot->services[index];
		found = 0;

		/* Process each remaining element. */
		for (loaded = 0; loaded < service_count; loaded++) {
			if (strcmp(services[loaded].name,
			    configured->name) == 0) {
				found = 1;
				break;
			}
		}

		/* Says nothing about one that is there, or one that may not be. */
		if (found || configured->optional)
			continue;
		fprintf(stderr,
			"init: %s: no definition in /etc/service.d\n",
			configured->name);
	}
}

/* Supports the load one service operation. */
static int
load_one_service(
	const char *name,
	const struct rcconf_model *snapshot)
{
	struct service *service;
	char path[320], value[64];
	size_t after_count, requires_count;
	int enabled;
	int replaces_error;

	/* Handles a failed service name valid operation. */
	if (!service_name_valid(name) || service_count == SERVICE_MAX ||
	    snprintf(path, sizeof(path), "/etc/service.d/%s", name) >=
		(int)sizeof(path))

		/* Reports operation failure. */
		return -1;

	service = &services[service_count];
	memset(service, 0, sizeof(*service));
	strcpy(service->name, name);

	/* Handles a failed assignment get operation. */
	if (assignment_get(path, "command", service->command,
			   sizeof(service->command)) != 0 ||
	    service->command[0] != '/')

		/* Reports operation failure. */
		return -1;

	(void)assignment_get(path, "arguments", service->arguments,
			     sizeof(service->arguments));

	/* The service this one stands in for, when it names one. */
	replaces_error = assignment_get(path, "replaces", service->replaces,
					sizeof(service->replaces));
	if (replaces_error != 0)
		service->replaces[0] = '\0';

	/* Handles the reported system error. */
	if ((assignment_get(path, "after", service->after,
			    sizeof(service->after)) != 0 &&
	     errno != ENOENT) ||
	    (assignment_get(path, "requires", service->requires,
			    sizeof(service->requires)) != 0 &&
	     errno != ENOENT) ||
	    zsv1_server_dependency_lists_validate(
		service->after, service->requires, &after_count,
		&requires_count) != 0)

		/* Reports operation failure. */
		return -1;
	service->notify_timeout = 10;

	service->type = SERVICE_DAEMON;

	/* Handles a failed assignment get operation. */
	if (assignment_get(path, "type", value, sizeof(value)) == 0) {
		/* Selects the matching value. */
		if (strcmp(value, "oneshot") == 0)
			service->type = SERVICE_ONESHOT;
		else if (strcmp(value, "respawn") == 0)
			service->type = SERVICE_RESPAWN;
		else if (strcmp(value, "daemon") != 0)

			/* Reports operation failure. */
			return -1;
	}

	/* Handles a failed assignment get operation. */
	if (assignment_get(path, "required", value, sizeof(value)) == 0)
		service->required = yes(value);

	/* Handles a failed assignment get operation. */
	if (assignment_get(path, "restart", value, sizeof(value)) == 0) {
		service->restart_always = strcmp(value, "always") == 0;
		service->restart_failure = strcmp(value, "on-failure") == 0;
	}

	/* Handles a failed assignment get operation. */
	if (assignment_get(path, "notify-fd3", value, sizeof(value)) == 0 &&
	    on_off(value, &service->notify_fd3) != 0)

		/* Reports operation failure. */
		return -1;

	/* Handles a failed assignment get operation. */
	if (assignment_get(path, "notify-timeout", value, sizeof(value)) == 0 &&
	    parse_seconds(value, 1, 300, &service->notify_timeout) != 0)

		/* Reports operation failure. */
		return -1;

	/* Handles the service condition. */
	if (service->notify_fd3 && service->type == SERVICE_ONESHOT)
		return -1;

	service->enabled =
	    snapshot != NULL &&
	    rcconf_service_enabled(snapshot, name, &enabled) == 0 && enabled;
	service->state = SERVICE_STOPPED;
	service_count++;

	/* Reports successful completion. */
	return 0;
}

/* Supports the yes operation. */
static int
yes(
	const char *value)
{
	int function_result;

	/* Computes the function result. */
	function_result = value != NULL &&
	       (strcmp(value, "YES") == 0 || strcmp(value, "yes") == 0 ||
		strcmp(value, "1") == 0 || strcmp(value, "true") == 0);

	/* Returns the computed result. */
	return function_result;
}

/* Supports the on off operation. */
static int
on_off(
	const char *value,
	int *result)
{
	/* Selects the matching value. */
	if (strcmp(value, "on") == 0) {
		*result = 1;
		/* Reports successful completion. */
		return 0;
	}

	/* Selects the matching value. */
	if (strcmp(value, "off") == 0) {
		*result = 0;
		/* Reports successful completion. */
		return 0;
	}

	/* Reports operation failure. */
	return -1;
}

/* Supports the parse seconds operation. */
static int
parse_seconds(
	const char *value,
	unsigned minimum,
	unsigned maximum,
	unsigned *result)
{
	char *end;
	unsigned long number;

	/* Handles the value availability. */
	if (value == NULL || *value == '\0')
		return -1;
	number = strtoul(value, &end, 10);

	/* Checks the current endpoint. */
	if (*end != '\0' || number < minimum || number > maximum)
		return -1;
	*result = (unsigned)number;
	/* Reports successful completion. */
	return 0;
}

/* Supports the open control socket operation. */
static int
open_control_socket(
	void)
{
	int error;
	struct sockaddr_un address;
	int descriptor;

	descriptor = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);

	/* Checks the file descriptor. */
	if (descriptor < 0)
		return -1;

	memset(&address, 0, sizeof(address));
	address.sun_family = AF_UNIX;
	strcpy(address.sun_path, KERN_INIT_SOCKET);

	(void)unlink(address.sun_path);

	/* Handles a failed bind operation. */
	if (bind(descriptor, (struct sockaddr *)&address, sizeof(address)) !=
		0 ||
	    chmod(address.sun_path, 0600) != 0 || listen(descriptor, 8) != 0) {
		error = errno;

		close(descriptor);
		errno = error;

		/* Reports operation failure. */
		return -1;
	}

	(void)fcntl(descriptor, F_SETFL, O_NONBLOCK);

	/* Returns the computed result. */
	return descriptor;
}

/* Supports the start enabled services operation. */
static void
start_enabled_services(
	void)
{
	struct service *service;
	struct service *replacer;
	enum dependency_result dependencies;
	size_t pass, index;

	/* Process each remaining element. */
	for (pass = 0; pass < service_count; pass++) {
		/* Process each remaining element. */
		for (index = 0; index < service_count; index++) {
			service = &services[index];

			/* A halt, power-off or reboot asked for meanwhile starts nothing more; the loop carries it out. */
			if (action_requested != 0)
				return;

			/* Handles the service condition. */
			if (!service->enabled ||
			    service->state != SERVICE_STOPPED)
				continue;

			/* A service another enabled one stands in for waits until that one gives up (replaces=). */
			replacer = replacer_of(service);
			if (replacer != NULL) {
				service->state = SERVICE_SKIPPED;
				fprintf(stderr, "init: %s replaced by %s\n",
					service->name, replacer->name);
				continue;
			}

			/* The dependencies decide whether it starts now. */
			dependencies = dependencies_state(service);

			/* Handles the dependencies condition. */
			if (dependencies == DEPENDENCIES_READY)
				(void)spawn_service(service);
			else if (dependencies == DEPENDENCIES_SKIP) {
				service->state = SERVICE_SKIPPED;
				fprintf(stderr,
					"init: skipped %s: required dependency "
					"failed\n",
					service->name);
			}
		}
	}

	/* Process each remaining element. */
	for (index = 0; index < service_count; index++) {
		/* Handles the services condition. */
		if (services[index].enabled &&
		    services[index].state == SERVICE_STOPPED) {
			services[index].state = SERVICE_FAILED;
			fprintf(stderr, "init: dependency cycle: %s\n",
				services[index].name);
		}
	}
}

/* Supports the dependencies state operation. */
static enum dependency_result
dependencies_state(
	const struct service *service)
{
	struct service *dependency_local;
	struct service *dependency_local1;
	char copy[256], *name;

	/* Handles the service condition. */
	if (service->after[0] != '\0') {
		strcpy(copy, service->after);

		/* Process each element required by the operation. */
		for (name = strtok(copy, ","); name != NULL;
		     name = strtok(NULL, ",")) {
			dependency_local = find_service(name);

			/* Handles the dependency local availability. */
			if (dependency_local == NULL)
				return DEPENDENCIES_SKIP;

			/* Handles a failed terminal state operation. */
			if (dependency_local->enabled &&
			    !terminal_state(dependency_local->state))

				/* Returns the computed result. */
				return DEPENDENCIES_WAIT;
		}
	}

	/* Handles the service condition. */
	if (service->requires[0] != '\0') {
		strcpy(copy, service->requires);

		/* Process each element required by the operation. */
		for (name = strtok(copy, ","); name != NULL;
		     name = strtok(NULL, ",")) {
			dependency_local1 = find_service(name);

			/* Handles an operation failure. */
			if (dependency_local1 == NULL || !dependency_local1->enabled ||
			    dependency_local1->state == SERVICE_FAILED ||
			    dependency_local1->state == SERVICE_SKIPPED)

				/* Returns the computed result. */
				return DEPENDENCIES_SKIP;

			/* Handles the dependency local1 condition. */
			if (dependency_local1->state != SERVICE_RUNNING &&
			    dependency_local1->state != SERVICE_COMPLETED)

				/* Returns the computed result. */
				return DEPENDENCIES_WAIT;
		}
	}

	/* Returns the computed result. */
	return DEPENDENCIES_READY;
}

/* Supports the find service operation. */
static struct service *
find_service(
	const char *name)
{
	size_t index;

	/* Process each remaining element. */
	for (index = 0; index < service_count; index++) {
		/* Selects the matching value. */
		if (strcmp(services[index].name, name) == 0)
			return &services[index];
	}

	/* Reports that no result is available. */
	return NULL;
}

/* Finds the enabled service that stands in for a service (replaces=), or NULL. */
static struct service *
replacer_of(
	const struct service *service)
{
	size_t index;
	int same;

	/* Each other enabled service that names this one. */
	for (index = 0; index < service_count; index++) {
		if (&services[index] == service || !services[index].enabled)
			continue;
		if (services[index].replaces[0] == '\0')
			continue;
		same = strcmp(services[index].replaces, service->name);
		if (same == 0)
			return &services[index];
	}

	/* No service stands in for it. */
	return NULL;
}

/* Starts the service an ended one stood in for, unless the system is being stopped. */
static void
release_replaced(
	const struct service *service)
{
	struct service *replaced;

	/* Nothing named, or the system is stopping. */
	if (service->replaces[0] == '\0' || services_stopping)
		return;

	/* The service it stood in for, when that one is enabled and was held back. */
	replaced = find_service(service->replaces);
	if (replaced == NULL || !replaced->enabled || replaced->state != SERVICE_SKIPPED)
		return;

	/* Starts it. */
	fprintf(stderr, "init: %s ended; starting %s\n", service->name, replaced->name);
	replaced->state = SERVICE_STOPPED;
	(void)spawn_service(replaced);
}

/* Supports the terminal state operation. */
static int
terminal_state(
	enum service_state state)
{
	/* Returns the computed result. */
	return state == SERVICE_RUNNING || state == SERVICE_COMPLETED ||
	       state == SERVICE_FAILED || state == SERVICE_SKIPPED;
}

/* Supports the spawn service operation. */
static int
spawn_service(
	struct service *service)
{
	int error_local;
	int error_local1;
	int error_local2;
	char argument_copy[512], *argv[ARGUMENT_MAX];
	char *argument;
	int count = 1, status, notify_pipe[2] = {-1, -1};
	int waited;
	pid_t child;

	/* Handles the service condition. */
	if (service->state == SERVICE_RUNNING ||
	    service->state == SERVICE_STARTING) {
		errno = EBUSY;

		/* Reports operation failure. */
		return -1;
	}

	/*
	 * The program is named to itself by the path it was run from, not by
	 * the name of the service.  A program that re-executes itself has
	 * only this to go on, and one that is handed a bare name cannot find
	 * itself again; it is also what a program prints when it reports its
	 * own name.
	 */
	argv[0] = service->command;

	strcpy(argument_copy, service->arguments);

	/* Process each element required by the operation. */
	for (argument = strtok(argument_copy, " \t"); argument != NULL;
	     argument = strtok(NULL, " \t")) {
		/* Checks the remaining item count. */
		if (count + 1 >= ARGUMENT_MAX) {
			fprintf(stderr, "init: too many arguments for %s\n",
				service->name);
			errno = E2BIG;

			/* Reports operation failure. */
			return -1;
		}
		argv[count++] = argument;
	}

	argv[count] = NULL;

	service->state = SERVICE_STARTING;

	/* Handles a failed pipe2 operation. */
	if (service->notify_fd3 &&
	    pipe2(notify_pipe, O_CLOEXEC | O_NONBLOCK) != 0) {
		service->state = SERVICE_FAILED;

		/* Reports operation failure. */
		return -1;
	}

	child = fork();

	/* Checks the child process state. */
	if (child == 0) {
		/* Handles the service condition. */
		if (service->notify_fd3) {
			close(notify_pipe[0]);

			/* Handles a failed dup2 operation. */
			if (dup2(notify_pipe[1], 3) < 0 ||
			    fcntl(3, F_SETFD, 0) != 0 ||
			    setenv("KERN_NOTIFY_FD", "3", 1) != 0)
				_exit(126);

			/* Handles the notify pipe condition. */
			if (notify_pipe[1] != 3)
				close(notify_pipe[1]);
		}
		execv(service->command, argv);
		fprintf(stderr, "init: exec %s: %s\n", service->command,
			strerror(errno));
		_exit(127);
	}

	/* Checks the child process state. */
	if (child < 0) {
		error_local = errno;

		/* Handles the notify pipe condition. */
		if (notify_pipe[0] >= 0)
			close(notify_pipe[0]);

		/* Handles the notify pipe condition. */
		if (notify_pipe[1] >= 0)
			close(notify_pipe[1]);
		service->state = SERVICE_FAILED;
		errno = error_local;

		/* Reports operation failure. */
		return -1;
	}

	service->pid = child;

	/* Handles the service condition. */
	if (service->notify_fd3) {
		close(notify_pipe[1]);

		/* Handles a failed wait for notification operation. */
		if (wait_for_notification(service, notify_pipe[0]) != 0) {
			error_local1 = errno;

			close(notify_pipe[0]);
			(void)kill(child, SIGTERM);
			(void)waitpid(child, &status, 0);
			service->pid = 0;
			service->state = SERVICE_FAILED;
			errno = error_local1;

			/* Reports operation failure. */
			return -1;
		}
		close(notify_pipe[0]);
	}

	/* Handles the service condition. */
	if (service->type != SERVICE_ONESHOT) {
		service->state = SERVICE_RUNNING;
		printf("init: started %s pid %ld\n", service->name,
		       (long)child);

		/* Reports successful completion. */
		return 0;
	}

	/* Waits for the oneshot, serving the control socket meanwhile. */
	waited = wait_for_oneshot(child, &status);
	if (waited > 0) {
		/* A halt, power-off or reboot was asked for: the service is stopped with the others. */
		printf("init: oneshot %s still running at the system action\n",
		       service->name);

		/* Reports successful completion. */
		return 0;
	}

	/* Handles a failed wait. */
	if (waited < 0) {
		error_local2 = errno;

		service->state = SERVICE_FAILED;
		service->pid = 0;
		fprintf(stderr, "init: oneshot %s wait failed\n",
			service->name);
		errno = error_local2;

		/* Reports operation failure. */
		return -1;
	}

	/* Handles a failed WIFEXITED operation. */
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
		service->state = SERVICE_FAILED;
		service->pid = 0;
		fprintf(stderr, "init: oneshot %s failed\n", service->name);
		errno = EIO;

		/* Reports operation failure. */
		return -1;
	}

	service->state = SERVICE_COMPLETED;
	service->pid = 0;

	/* Reports successful completion. */
	return 0;
}

/*
 * Waits for a oneshot service to end while serving the control socket.
 * Returns 0 when it ended (its status in *status), 1 when a halt, power-off or
 * reboot was asked for first, and -1 when the wait failed (errno).
 */
static int
wait_for_oneshot(
	pid_t child,
	int *status)
{
	struct pollfd listener;
	pid_t reaped;

	/* Looks at the service and the socket until one of them ends the wait. */
	for (;;) {
		/* Collects the service when it has ended. */
		reaped = waitpid(child, status, WNOHANG);
		if (reaped == child)
			return 0;

		/* Handles a failed wait. */
		if (reaped < 0 && errno != EINTR)
			return -1;

		/* A system action asked for meanwhile ends the wait: the action stops the service. */
		if (action_requested != 0)
			return 1;

		/* Answers the requests that came. */
		serve_while_oneshot();

		/* Waits a little for a connection, or only sleeps without a socket. */
		if (control_listener >= 0) {
			listener.fd = control_listener;
			listener.events = POLLIN;
			listener.revents = 0;
			(void)poll(&listener, 1, ONESHOT_POLL_MS);
		} else {
			(void)usleep(ONESHOT_POLL_MS * 1000);
		}
	}
}

/*
 * Serves the control socket while a oneshot service runs.  A system action
 * and the requests that only read are answered at once; one that starts or
 * stops services is held for init's loop (deferred_requests), or refused
 * when too many are held.
 */
static void
serve_while_oneshot(
	void)
{
	struct zsv1_request request;
	int client;
	int received;
	int error;

	/* Takes each connection waiting now. */
	for (;;) {
		client = accept4(control_listener, NULL, NULL, SOCK_CLOEXEC);
		if (client < 0)
			return;

		/* Reads the request. */
		received = receive_request(client, &request);
		if (received != 0) {
			error = errno;
			refuse_request(client, error);
			close(client);
			continue;
		}

		/* Decides whether the request is answered now. */
		switch (request.command) {
		case ZSV1_COMMAND_HALT:
		case ZSV1_COMMAND_POWEROFF:
		case ZSV1_COMMAND_REBOOT:
		case ZSV1_COMMAND_LIST:
		case ZSV1_COMMAND_SHOW:
		case ZSV1_COMMAND_RELOAD:
			dispatch_request(client, &request);
			close(client);
			break;
		default:
			/* Starting or stopping services waits for init's loop. */
			if (deferred_count < DEFERRED_MAX) {
				deferred_requests[deferred_count].client = client;
				deferred_requests[deferred_count].request = request;
				deferred_count++;
			} else {
				(void)zsv1_server_send_error_end_fd(client, EBUSY, "oneshot-running");
				close(client);
			}

			break;
		}
	}
}

/* Carries out the requests held while a oneshot service ran, oldest first. */
static void
handle_deferred_requests(
	void)
{
	struct deferred_request deferred;

	/* Takes the oldest each time: carrying one out can hold new ones behind it. */
	while (deferred_count > 0) {
		deferred = deferred_requests[0];
		deferred_count--;
		memmove(&deferred_requests[0], &deferred_requests[1],
			deferred_count * sizeof(deferred_requests[0]));
		dispatch_request(deferred.client, &deferred.request);
		close(deferred.client);
	}
}

/* Supports the wait for notification operation. */
static int
wait_for_notification(
	struct service *service,
	int descriptor)
{
	char *newline;
	ssize_t count;
	char record[513];
	size_t used;
	uint64_t deadline;

	used = 0;
	deadline = monotonic_milliseconds() +
			    (uint64_t)service->notify_timeout * 1000U;

	/* Continue until the operation reaches a terminal state. */
	for (;;) {
		/* Continue until the operation reaches a terminal state. */
		for (;;) {
			count = read(descriptor, record + used,
					     sizeof(record) - used - 1U);

			/* Handles the reported system error. */
			if (count < 0 && errno == EINTR)
				continue;

			/* Handles the reported system error. */
			if (count < 0 &&
			    (errno == EAGAIN || errno == EWOULDBLOCK))
				break;

			/* Checks the remaining item count. */
			if (count <= 0) {
				fprintf(stderr,
					"init: %s exited before readiness\n",
					service->name);
				errno = EPIPE;

				/* Reports operation failure. */
				return -1;
			}
			used += (size_t)count;

			/* Checks the current capacity usage. */
			if (used >= sizeof(record) - 1U) {
				fprintf(stderr,
					"init: %s oversized readiness record\n",
					service->name);
				errno = EOVERFLOW;

				/* Reports operation failure. */
				return -1;
			}
			record[used] = '\0';

			newline = strchr(record, '\n');

			/* Handles the newline availability. */
			if (newline == NULL)
				continue;

			/* Handles the newline condition. */
			if (newline[1] != '\0') {
				errno = EINVAL;

				/* Reports operation failure. */
				return -1;
			}
			*newline = '\0';
			/* Selects the matching value. */
			if (strcmp(record, "READY") == 0)
				return 0;

			/* Handles an operation failure. */
			if (valid_failure_record(record)) {
				fprintf(stderr, "init: %s: %s\n",
					service->name, record);
			} else {
				fprintf(stderr,
					"init: %s malformed readiness "
					"record\n",
					service->name);
			}
			errno = EINVAL;

			/* Reports operation failure. */
			return -1;
		}

		/* Handles a failed monotonic milliseconds operation. */
		if (monotonic_milliseconds() >= deadline) {
			errno = ETIMEDOUT;
			fprintf(stderr, "init: %s readiness timeout\n",
				service->name);

			/* Reports operation failure. */
			return -1;
		}
		usleep(10000);
	}
}

/* Supports the monotonic milliseconds operation. */
static uint64_t
monotonic_milliseconds(
	void)
{
	struct timespec now;

	/* Handles a failed clock gettime operation. */
	if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
		return 0;

	/* Returns the computed result. */
	return (uint64_t)(uint32_t)now.tv_sec * 1000U +
	       (uint32_t)now.tv_nsec / 1000000U;
}

/* Supports the valid failure record operation. */
static int
valid_failure_record(
	const char *record)
{
	unsigned char character;
	const char *cursor;
	unsigned long code;

	cursor = record + 5;
	code = 0;

	/* Selects the matching prefix. */
	if (strncmp(record, "FAIL ", 5) != 0 || *cursor < '0' || *cursor > '9')
		return 0;

	/* Continue while the operation condition remains true. */
	while (*cursor >= '0' && *cursor <= '9') {
		code = code * 10U + (unsigned)(*cursor++ - '0');

		/* Handles the code condition. */
		if (code > 2147483647UL)
			return 0;
	}

	/* Handles the code condition. */
	if (code == 0 || *cursor++ != ' ' || *cursor == '\0')
		return 0;

	/* Continue while the operation condition remains true. */
	while (*cursor != '\0') {
		character = (unsigned char)*cursor++;

		/* Classifies the current input character. */
		if (character < 32U || character == 127U)
			return 0;
	}

	/* Reports operation failure. */
	return 1;
}

/* Supports the reap children operation. */
static void
reap_children(
	void)
{
	struct service *service;
	int success;
	size_t index;
	pid_t child;
	int status;

	/* Continue while the operation condition remains true. */
	while ((child = waitpid(-1, &status, WNOHANG)) > 0) {
		/* Process each remaining element. */
		for (index = 0; index < service_count; index++) {
			service = &services[index];

			/* Handles the service condition. */
			if (service->pid != child)
				continue;

			success = WIFEXITED(status) && WEXITSTATUS(status) == 0;
			service->pid = 0;
			service->state =
			    success ? SERVICE_STOPPED : SERVICE_FAILED;

#if 0

			/* Checks the operation status. */
			if (WIFEXITED(status))
				fprintf(stderr, "init: %s exited status=%d\n", service->name, WEXITSTATUS(status));
			else if (WIFSIGNALED(status))
				fprintf(stderr, "init: %s killed signal=%d\n", service->name, WTERMSIG(status));
			else
				fprintf(stderr, "init: %s changed state status=%d\n", service->name, status);
#endif

			/* Handles an operation failure. */
			if (service->enabled && service->failures < 5 &&
			    (service->restart_always ||
			     (service->restart_failure && !success))) {
				service->failures++;
				sleep(1);
				(void)spawn_service(service);
			} else {
				/* Not started again: the service it stands in for starts now (replaces=). */
				release_replaced(service);
			}

			break;
		}
	}
}

/* Supports the shutdown system operation. */
static void
shutdown_system(
	enum init_action action)
{
	size_t index;
	const char *action_name;
	int system_descriptor, system_action;

	index = service_count;
	services_stopping = 1;

	printf("init: stopping services\n");

	/* Process each remaining element. */
	while (index > 0)
		(void)stop_service(&services[--index]);

	sync();
	action_name = action == INIT_ACTION_REBOOT     ? "reboot"
		      : action == INIT_ACTION_POWEROFF ? "poweroff"
						       : "halt";
	printf("init: executing system action %s\n", action_name);
	(void)fflush(stdout);

	/* A power-off asks the kernel to cut the power; a halt and a reboot ask for those. */
	if (action == INIT_ACTION_REBOOT) {
		system_action = KERN_SYSTEM_REBOOT;
	} else if (action == INIT_ACTION_POWEROFF) {
		system_action = KERN_SYSTEM_POWEROFF;
	} else {
		system_action = KERN_SYSTEM_HALT;
	}

	/* Keeps the requested action pending while storage can still recover. */
	for (;;) {
		system_descriptor = open("/dev/system", O_RDONLY);
		if (system_descriptor >= 0) {
			if (ioctl(system_descriptor, system_action) == 0) {
				(void)close(system_descriptor);
				break;
			}

			/* A kernel or a machine that cannot cut its power is halted instead. */
			if (system_action == KERN_SYSTEM_POWEROFF && errno == EOPNOTSUPP) {
				fprintf(stderr, "init: the machine cannot turn its power off; halting\n");
				system_action = KERN_SYSTEM_HALT;
				(void)close(system_descriptor);
				continue;
			}
		}
		fprintf(stderr, "init: final system action failed: %s; retrying in 5 seconds\n",
			strerror(errno));
		if (system_descriptor >= 0)
			(void)close(system_descriptor);
		(void)sleep(5);
	}

	/* Continue until the operation reaches a terminal state. */
	for (;;)
		pause();
}

/* Supports the stop service operation. */
static int
stop_service(
	struct service *service)
{
	unsigned attempt;
	int status;

	/* Handles the service condition. */
	if (service->pid <= 0) {
		service->state = SERVICE_STOPPED;

		/* Reports successful completion. */
		return 0;
	}

	/* Handles the reported system error. */
	if (kill(service->pid, SIGTERM) != 0 && errno != ESRCH)
		return -1;

	/* Process each element required by the operation. */
	for (attempt = 0; attempt < 5; attempt++) {
		/* Handles a failed waitpid operation. */
		if (waitpid(service->pid, &status, WNOHANG) == service->pid) {
			service->pid = 0;
			service->state = SERVICE_STOPPED;

			/* Reports successful completion. */
			return 0;
		}
		sleep(1);
	}

	(void)kill(service->pid, SIGKILL);
	(void)waitpid(service->pid, &status, 0);

	service->pid = 0;
	service->state = SERVICE_STOPPED;

	/* Reports successful completion. */
	return 0;
}

/* Supports the reload policy operation. */
static int
reload_policy(
	void)
{
	int error;
	int enabled;
	struct rcconf_model *snapshot;
	size_t index;

	snapshot = load_rcconf_snapshot();

	/* Handles the snapshot availability. */
	if (snapshot == NULL) {
		error = errno;

		fprintf(stderr, "init: cannot reload %s: %s\n", RCCONF_PATH,
			strerror(error));
		errno = error;

		/* Reports operation failure. */
		return -1;
	}

	/* Process each remaining element. */
	for (index = 0; index < service_count; index++) {
		services[index].enabled =
		    rcconf_service_enabled(snapshot, services[index].name,
					   &enabled) == 0 &&
		    enabled;
	}
	free(snapshot);

	/* Reports successful completion. */
	return 0;
}

/* Supports the handle request operation. */
static void
handle_request(
	int client)
{
	struct zsv1_request request;
	int received;
	int error;

	/* Handles a failed receive request operation. */
	received = receive_request(client, &request);
	if (received != 0) {
		error = errno;
		refuse_request(client, error);

		/* Returns the computed result. */
		return;
	}

	/* Carries the request out. */
	dispatch_request(client, &request);
}

/* Answers a request that could not be read. */
static void
refuse_request(
	int client,
	int error)
{
	const char *reason;

	/* Names what was wrong with the request. */
	if (error == EPROTONOSUPPORT) {
		reason = "unknown-version";
	} else if (error == EOVERFLOW || error == EMSGSIZE) {
		reason = "request-too-long";
	} else if (error == ETIMEDOUT) {
		reason = "request-timeout";
	} else {
		reason = "malformed-request";
	}

	/* A failure without its own number is reported as an I/O error. */
	if (error <= 0)
		error = EIO;

	/* Answers the client. */
	(void)zsv1_server_send_error_end_fd(client, error, reason);
}

/* Carries out one received request and answers it. */
static void
dispatch_request(
	int client,
	const struct zsv1_request *request)
{
	enum init_action action;
	struct service *service;
	size_t index;
	int error;

	/* Handles the request condition. */
	if (request->command == ZSV1_COMMAND_LIST) {
		/* Process each remaining element. */
		for (index = 0; index < service_count; index++) {
			/* Handles a failed send service operation. */
			if (send_service(client, &services[index]) != 0)
				return;
		}
		(void)zsv1_server_send_end_fd(client);

		/* Returns the computed result. */
		return;
	}

	/* Handles the request condition. */
	if (request->command == ZSV1_COMMAND_RELOAD) {
		/* Handles a failed reload policy operation. */
		if (reload_policy() == 0) {
			(void)zsv1_server_send_ok_end_fd(client, "reloaded");
		} else {
			error = errno;
			(void)zsv1_server_send_error_end_fd(
			    client, error > 0 ? error : EIO, "reload-failed");
		}

		/* Returns the computed result. */
		return;
	}

	/* Handles the request condition. */
	if (request->command == ZSV1_COMMAND_HALT ||
	    request->command == ZSV1_COMMAND_POWEROFF ||
	    request->command == ZSV1_COMMAND_REBOOT) {
		action = request->command == ZSV1_COMMAND_REBOOT ? INIT_ACTION_REBOOT
	    : request->command == ZSV1_COMMAND_POWEROFF
	? INIT_ACTION_POWEROFF
	: INIT_ACTION_HALT;

		/* Handles a failed zsv1 server send ok end fd operation. */
		if (zsv1_server_send_ok_end_fd(client, "scheduled") == 0)
			action_requested = action;

		/* Returns the computed result. */
		return;
	}

	/* Finds the service the request names. */
	service = find_service(request->service);

	/* Handles the service availability. */
	if (service == NULL) {
		(void)zsv1_server_send_error_end_fd(client, ENOENT,
						    "unknown-service");

		/* Returns the computed result. */
		return;
	}

	/* Handles the request condition. */
	if (request->command == ZSV1_COMMAND_SHOW) {
		/* Handles a failed send service operation. */
		if (send_service(client, service) != 0 ||
		    send_dependencies(client, service->after,
				      ZSV1_RECORD_AFTER) != 0 ||
		    send_dependencies(client, service->requires,
				      ZSV1_RECORD_REQUIRES) != 0)

			/* Returns the computed result. */
			return;
		(void)zsv1_server_send_end_fd(client);

		/* Returns the computed result. */
		return;
	}

	/* Handles the request condition. */
	if (request->command == ZSV1_COMMAND_START) {
		/* Handles a failed spawn service operation. */
		if (spawn_service(service) == 0) {
			(void)zsv1_server_send_ok_end_fd(client, "started");

			/* Returns the computed result. */
			return;
		}
		error = errno;
		(void)zsv1_server_send_error_end_fd(
		    client, error > 0 ? error : EIO, "start-failed");

		/* Returns the computed result. */
		return;
	}

	/* Handles the request condition. */
	if (request->command == ZSV1_COMMAND_STOP) {
		/* Handles a failed stop service operation. */
		if (stop_service(service) == 0) {
			(void)zsv1_server_send_ok_end_fd(client, "stopped");

			/* Returns the computed result. */
			return;
		}
		error = errno;
		(void)zsv1_server_send_error_end_fd(
		    client, error > 0 ? error : EIO, "stop-failed");

		/* Returns the computed result. */
		return;
	}

	/* Handles the request condition. */
	if (request->command == ZSV1_COMMAND_RESTART) {
		/* Handles a failed stop service operation. */
		if (stop_service(service) == 0 && spawn_service(service) == 0) {
			(void)zsv1_server_send_ok_end_fd(client, "restarted");

			/* Returns the computed result. */
			return;
		}
		error = errno;
		(void)zsv1_server_send_error_end_fd(
		    client, error > 0 ? error : EIO, "restart-failed");

		/* Returns the computed result. */
		return;
	}

	(void)zsv1_server_send_error_end_fd(client, EINVAL, "unknown-command");
}

/* Supports the receive request operation. */
static int
receive_request(
	int client,
	struct zsv1_request *request)
{
	int function_result;
	struct timespec deadline;

	/* Handles a failed clock gettime operation. */
	if (clock_gettime(CLOCK_MONOTONIC, &deadline) != 0)
		return -1;
	deadline.tv_sec += 5;

	/* Obtains the zsv1 server receive fd result. */
	function_result = zsv1_server_receive_fd(client, request, &deadline);

	/* Returns the computed result. */
	return function_result;
}

/* Supports the send service operation. */
static int
send_service(
	int client,
	const struct service *service)
{
	int function_result;
	struct zsv1_record record;

	memset(&record, 0, sizeof(record));
	record.type = ZSV1_RECORD_SERVICE;
	strcpy(record.service.name, service->name);
	record.service.state = zsv1_state(service->state);
	record.service.enabled = service->enabled;
	record.service.pid = service->pid;

	/* Obtains the zsv1 server send record fd result. */
	function_result = zsv1_server_send_record_fd(client, &record);

	/* Returns the computed result. */
	return function_result;
}

/* Supports the zsv1 state operation. */
static enum zsv1_service_state
zsv1_state(
	enum service_state state)
{
	/* Dispatch the current operation state. */
	switch (state) {
	case SERVICE_STOPPED:
		/* Returns the computed result. */
		return ZSV1_STATE_STOPPED;
	case SERVICE_STARTING:
		/* Returns the computed result. */
		return ZSV1_STATE_STARTING;
	case SERVICE_RUNNING:
		/* Returns the computed result. */
		return ZSV1_STATE_RUNNING;
	case SERVICE_COMPLETED:
		/* Returns the computed result. */
		return ZSV1_STATE_COMPLETED;
	case SERVICE_FAILED:
		/* Returns the computed result. */
		return ZSV1_STATE_FAILED;
	case SERVICE_SKIPPED:
		/* Returns the computed result. */
		return ZSV1_STATE_SKIPPED;
	}

	/* Returns the computed result. */
	return ZSV1_STATE_FAILED;
}

/* Supports the send dependencies operation. */
static int
send_dependencies(
	int client,
	const char *list,
	enum zsv1_record_type type)
{
	struct zsv1_record record;
	char copy[256], *name;

	/* Handles the list condition. */
	if (*list == '\0')
		return 0;
	strcpy(copy, list);

	/* Process each element required by the operation. */
	for (name = strtok(copy, ","); name != NULL; name = strtok(NULL, ",")) {
		memset(&record, 0, sizeof(record));
		record.type = type;
		strcpy(record.name, name);

		/* Handles a failed zsv1 server send record fd operation. */
		if (zsv1_server_send_record_fd(client, &record) != 0)
			return -1;
	}

	/* Reports successful completion. */
	return 0;
}

/* Supports the signal handler operation. */
static void
signal_handler(
	int number)
{
	/* Handles the number condition. */
	if (number == SIGHUP)
		reload_requested = 1;
	else if (number == SIGINT)
		action_requested = INIT_ACTION_REBOOT;
	else if (number == SIGTERM)
		action_requested = INIT_ACTION_HALT;
}
