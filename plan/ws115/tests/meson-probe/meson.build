# ws115-p004: the smallest Meson project that exercises the zedBSD cross
# contract -- a shared library with a SONAME, an executable linked to it, a
# dependency found through the zedBSD pkg-config, a code generator built for
# the build machine, and a generated .pc file.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
project('zedbsd-meson-probe', 'c', version: '1.0.0',
        default_options: ['warning_level=3', 'werror=true'])

# The cross file names the target; a project that branches on the system
# sees zedbsd and nothing else.
assert(meson.is_cross_build(), 'the probe is a cross build')
assert(host_machine.system() == 'zedbsd', 'host_machine.system() is zedbsd')

# The zedBSD pkg-config sees only the declared stages: zlib is declared,
# the build machine's glib is installed but must not be found.
zlib_dep = dependency('zlib')
host_glib = dependency('glib-2.0', required: false)
assert(not host_glib.found(), 'the build machine\'s .pc files are not visible')

# A facility of the zedBSD C library, checked by compiling and linking
# against the sysroot through the wrapper.
cc = meson.get_compiler('c')
assert(cc.has_function('shm_open', prefix: '#include <sys/mman.h>'),
       'shm_open links against the zedBSD libc')

# A program for the build machine, run during the build to generate a header.
generator = executable('probe-generate', 'generate.c', native: true)
seed_h = custom_target('probe-seed',
                       output: 'probe-seed.h',
                       command: [generator, '@OUTPUT@'])

probe_lib = shared_library('probe', 'probe.c', seed_h,
                           dependencies: zlib_dep,
                           version: '1.0.0', soversion: '1',
                           install: true)
install_headers('probe.h')
executable('probe-check', 'check.c',
           link_with: probe_lib, install: true)

pkg = import('pkgconfig')
pkg.generate(probe_lib, description: 'zedBSD Meson cross probe')
