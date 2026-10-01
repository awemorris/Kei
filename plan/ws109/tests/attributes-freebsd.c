/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Compares the native attribute adapter with independent UFS namespace and no-follow inquiries. */
#include <sys/types.h>
#include <sys/extattr.h>
#include <sys/xattr.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int probe_attributes(const char *source, const char *destination, const char *link);
static int probe_permission(const char *source);

/* Runs only on the controller's exclusively owned source, destination and symbolic link. */
int
main(
	int argc,
	char **argv)
{
	int error;
	int same;

	/* Refuses incomplete owned-file fixture arguments. */
	if (argc != 5)
		return 1;

	/* Permission checks use actual dropped credentials while ordinary checks compare native values. */
	same = strcmp(argv[1], "permission");
	if (same == 0) {
		error = probe_permission(argv[2]);
	} else {
		error = probe_attributes(argv[2], argv[3], argv[4]);
	}

	/* Reports a contract failure without touching any caller-owned filesystem fixture. */
	if (error != 0) {
		(void)fprintf(stderr, "native attributes failed errno=%d\n", error);
		return 1;
	}

	/* Succeeded: real namespace, output bounds and object identity were verified. */
	(void)printf("PASS native attributes %s\n", argv[1]);
	return 0;
}

/* Checks native value/list translations, fd metadata copies and link-object identity. */
static int
probe_attributes(
	const char *source,
	const char *destination,
	const char *link)
{
	char value[128];
	char names[256];
	char tiny[2];
	ssize_t size;
	ssize_t required;
	size_t at;
	int descriptor;
	int output;
	int status;
	int error;
	int saved;

	/* Writes the application's actual tag representation and reads it through the independent native API. */
	status = setxattr(source, "user.keiland.tags", "Work\nIdeas\n", 11, 0);
	if (status != 0)
		return errno;

	/* Native storage strips only the namespace prefix, retaining the exact user attribute leaf. */
	size = extattr_get_file(source, EXTATTR_NAMESPACE_USER, "keiland.tags", value, sizeof(value));
	if (size != 11)
		return EPROTO;

	/* Stored tag bytes must survive namespace translation without encoding changes. */
	status = memcmp(value, "Work\nIdeas\n", 11);
	if (status != 0)
		return EPROTO;

	/* Inserts another native namespace directly so enumeration cannot rely only on adapter writes. */
	size = extattr_set_file(source, EXTATTR_NAMESPACE_SYSTEM, "ws109.system", "native", 6);
	if (size != 6)
		return EPROTO;

	/* A native binary value must preserve embedded NUL bytes during the application's fd copy. */
	size = extattr_set_file(source, EXTATTR_NAMESPACE_USER, "native.binary", "a\0b", 3);
	if (size != 3)
		return EPROTO;

	/* Queries the translated NUL-separated list length rather than its native length-prefixed size. */
	required = llistxattr(source, NULL, 0);
	if (required != sizeof("user.keiland.tags") + sizeof("system.ws109.system") + sizeof("user.native.binary"))
		return EPROTO;

	/* A short list cannot publish even its first partial name. */
	memset(tiny, 0x5a, sizeof(tiny));
	size = llistxattr(source, tiny, sizeof(tiny));
	if (size != -1 ||
	    errno != ERANGE ||
	    tiny[0] != 0x5a ||
	    tiny[1] != 0x5a)
		return EPROTO;

	/* A short native value buffer also must remain untouched instead of receiving truncated bytes. */
	size = getxattr(source, "user.keiland.tags", tiny, sizeof(tiny));
	if (size != -1 ||
	    errno != ERANGE ||
	    tiny[0] != 0x5a ||
	    tiny[1] != 0x5a)
		return EPROTO;

	/* A size-only value inquiry must return the entire actual native attribute size. */
	size = getxattr(source, "user.keiland.tags", NULL, 0);
	if (size != 11)
		return EPROTO;

	/* Unsupported atomic flags cannot replace an existing attribute through a racy approximation. */
	status = setxattr(source, "user.keiland.tags", "changed", 7, 1);
	if (status != -1 || errno != ENOTSUP)
		return EPROTO;

	/* Unsupported Linux namespaces cannot change native privileged namespace policy. */
	status = setxattr(source, "security.ws109", "changed", 7, 0);
	if (status != -1 || errno != ENOTSUP)
		return EPROTO;

	/* Invalid borrowed descriptors remain ordinary native kernel errors. */
	size = fgetxattr(-1, "user.keiland.tags", value, sizeof(value));
	if (size != -1 || errno != EBADF)
		return EPROTO;

	/* Enumeration cannot turn an invalid descriptor into a successful empty list. */
	size = flistxattr(-1, names, sizeof(names));
	if (size != -1 || errno != EBADF)
		return EPROTO;

	/* Opens both exclusively owned files and retains their descriptor owners outside the adapter. */
	descriptor = open(source, O_RDONLY | O_CLOEXEC);
	if (descriptor < 0)
		return errno;

	/* Destination failure cannot abandon the already opened source. */
	output = open(destination, O_RDWR | O_CLOEXEC);
	if (output < 0) {
		saved = errno;
		(void)close(descriptor);
		return saved;
	}

	/* Copies the same complete namespace-qualified metadata stream used by the real file manager. */
	error = 0;
	size = flistxattr(descriptor, names, sizeof(names));
	if (size != required) {
		error = EPROTO;
		goto cleanup;
	}

	/* Every returned name must preserve its entire binary value through fd-based copy. */
	at = 0;
	while (at < (size_t)required) {
		size = fgetxattr(descriptor, names + at, value, sizeof(value));
		if (size < 0) {
			error = errno;
			goto cleanup;
		}

		/* The destination write must acknowledge all bytes of this real native attribute. */
		status = fsetxattr(output, names + at, value, (size_t)size, 0);
		if (status != 0) {
			error = errno;
			goto cleanup;
		}

		/* The translated terminator keeps enumeration independent of native prefix-length records. */
		at += strlen(names + at) + 1;
	}

	/* Independent native inquiries verify both copied namespaces and the binary payload. */
	size = extattr_get_fd(output, EXTATTR_NAMESPACE_USER, "native.binary", value, sizeof(value));
	if (size != 3) {
		error = EPROTO;
		goto cleanup;
	}

	/* A copied NUL byte must not become an attribute-value terminator. */
	status = memcmp(value, "a\0b", 3);
	if (status != 0) {
		error = EPROTO;
		goto cleanup;
	}

	/* Native system metadata must remain in its original privileged namespace. */
	size = extattr_get_fd(output, EXTATTR_NAMESPACE_SYSTEM, "ws109.system", value, sizeof(value));
	if (size != 6) {
		error = EPROTO;
		goto cleanup;
	}

	/* The source tag value remains unchanged despite the refused unsupported updates. */
	size = extattr_get_fd(output, EXTATTR_NAMESPACE_USER, "keiland.tags", value, sizeof(value));
	if (size != 11) {
		error = EPROTO;
		goto cleanup;
	}

	/* A present empty value must remain distinct from a missing native attribute. */
	status = fsetxattr(output, "user.empty", "", 0, 0);
	if (status != 0) {
		error = errno;
		goto cleanup;
	}

	/* Native storage independently confirms the zero-byte value still exists. */
	size = extattr_get_fd(output, EXTATTR_NAMESPACE_USER, "empty", NULL, 0);
	if (size != 0) {
		error = EPROTO;
		goto cleanup;
	}

	/* A fitting read of an existing empty value succeeds without inventing a payload. */
	size = fgetxattr(output, "user.empty", value, sizeof(value));
	if (size != 0) {
		error = EPROTO;
		goto cleanup;
	}

	/* Gives the link its own native attribute distinct from the target's tags. */
	size = extattr_set_link(link, EXTATTR_NAMESPACE_USER, "link.marker", "link", 4);
	if (size != 4) {
		error = EPROTO;
		goto cleanup;
	}

	/* No-follow inquiries must read the link's own value. */
	size = lgetxattr(link, "user.link.marker", value, sizeof(value));
	if (size != 4) {
		error = EPROTO;
		goto cleanup;
	}

	/* Followed inquiries must refuse a link-only attribute absent on the target. */
	size = getxattr(link, "user.link.marker", value, sizeof(value));
	if (size != -1 || errno != ENOATTR) {
		error = EPROTO;
		goto cleanup;
	}

	/* Link enumeration must omit every target-only attribute. */
	size = llistxattr(link, names, sizeof(names));
	if (size != sizeof("user.link.marker")) {
		error = EPROTO;
		goto cleanup;
	}

	/* The sole translated link name must identify its independent inode attribute. */
	status = strcmp(names, "user.link.marker");
	if (status != 0) {
		error = EPROTO;
		goto cleanup;
	}

	/* Removing the tag affects only the source inode and reports native absence on the next query. */
	status = removexattr(source, "user.keiland.tags");
	if (status != 0) {
		error = errno;
		goto cleanup;
	}

	/* Missing native values must be failures rather than empty successful values. */
	size = getxattr(source, "user.keiland.tags", NULL, 0);
	if (size != -1 || errno != ENOATTR) {
		error = EPROTO;
		goto cleanup;
	}

cleanup:
	/* Only the probe closes its borrowed source and destination files. */
	(void)close(descriptor);
	(void)close(output);
	if (error != 0)
		return error;

	/* Succeeded: native storage, translated names and no-follow metadata ownership agree. */
	return 0;
}

/* Checks accessible user enumeration despite the kernel's actual denied system namespace. */
static int
probe_permission(
	const char *source)
{
	char names[256];
	ssize_t size;
	uid_t user;
	int groups;

	/* Refuses a controller that accidentally retained root or supplementary permissions. */
	user = geteuid();
	if (user != 65534)
		return EINVAL;

	/* The independent namespace inquiry requires actual unprivileged credentials. */
	groups = getgroups(0, NULL);
	if (groups != 0)
		return EINVAL;

	/* Native kernel policy must deny enumeration of this inode's system attributes. */
	size = extattr_list_link(source, EXTATTR_NAMESPACE_SYSTEM, names, sizeof(names));
	if (size != -1 || errno != EPERM)
		return EPROTO;

	/* The adapter must retain accessible user metadata instead of discarding the whole list. */
	size = llistxattr(source, names, sizeof(names));
	if (size != sizeof("user.native.binary"))
		return EPROTO;

	/* Succeeded: native permissions and accessible user metadata remain independently observable. */
	return 0;
}
