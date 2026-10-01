# WS108 final conformance / q549 / P1–P5

Final executable/packager/CI source b0e1eaf972e1 (WIP). Full source SHA inventory:
[source-hashes.json](q549/source-hashes.json). All8 changed source/config files
reviewed completely (plus archived independent audit evidence script q549/audit.py): AGENTS.md, Makefile, CI YAML, run.py, build.py,
verify-artifacts.py, pinned inputs JSON and README. No production C/header or
reused vk-chain-test.c/elf-check.sh change. Full AGENTS/Guardrail/C coding standard
review: C syntax/formatter/style-check sections apply to no newly generated C;
non-C uses established Python/Make/YAML conventions (automation registry), plus
copyright/license, ownership, explicit fallible commands, cleanup, source/module
boundaries, no hidden production test switch. No style exception added.

| Acceptance | Actual evidence |
| --- | --- |
| P1 | amd64, Debian13/Ubuntu26.04 pinned official images, native git version, private /opt layout, production manifest40, fonts/project/dictionary license notices, native libc and explicit dlopen Vulkan/Mesa/seat/kbd dependencies. Test apps5/development headers/model data absent. |
| P2 | Both final requested make targets exit0; native QEMU GCC14.2/GCC15.2 build, source_dirty=false; 19 ELF/RUNPATH/SONAME/NEEDED/ldd PASS; manifest bytes/hash/mode/root ownership and md5/control audited independently from deb tar data. |
| P3 | Each final deb in separate fresh KVM and TCG overlays passed install/reinstall/genuine+smoke1 upgrade/edited conffile/remove/purge/user-home preservation; actual public Vulkan client, direct KMS compositor + Files desktop, Terminal mapping/click/keyboard command independently verified by SSH-created file and QMP PNG. |
| P4 | Matrix exact2 targets/TCG, distinct upload + missing inputs failure, checksum/clean source/runtime-package identity gate, release needs build+entire matrix/download merge, deb/checksum/manifest/buildinfo release files. Combined local release verifier PASS; missing package/corrupt metadata/image checksum refusal PASS. Existing build job deep-equal to pre-WS108; original triggers/permissions/image/optional zip retained. |
| P5 | Full final source/manual rules review, byte compile, YAML + bash syntax, diff-check, payload/control audit, combined release gate and own emulator cessation PASS. Source8-file hash recorded; executable code unchanged after b0. |

## Commands / actual environments

- make keiland-linux-debian / make keiland-linux-ubuntu2604, final b0: exit0.
  QEMU10.0.11 / auto KVM, build+test separate8GiB overlays. Native distro/compiler/
  installed packages/source archive SHA/image pins in q548/evidence/kvm buildinfo.
- KEILAND_DEB_ACCEL=tcg + final run.py Guest/check_os/smoke in independent fresh
  overlays on both final b0 debs: exit0, q548/evidence/tcg and *tcg-final.log.
- Actual native TCG builds were performed in q547 on both OS. All40 payload hashes
  and modes equal final KVM build; guest packager's later install ordering fix
  changes no payload. Final complete TCG target invocation was not repeated;
  native TCG build and final TCG runtime are separate verified executions.
- python3 -m py_compile tools/release/keiland-linux-deb/*.py; Python3.13.
- PyYAML6.0.2 BaseLoader, embedded bash -n, existing jobs.build deep-equality.
- verify-artifacts.py --require-clean merged-local-artifacts debian13 ubuntu2604:
  PASS, as the release step will use. Independent dpkg-deb data/control tar audit:
  exact40 files/hash/mode/uid/gid/md5/conffile/session/no-test/no-headers PASS.
- git diff --check; no own build/keiland-deb emulator processes remaining.

Source compiler warnings0 both. dpkg-shlibdeps emits27 existing private
unversioned SONAME inference warnings each, classified explicitly; no missing-info
suppression. Native computed external libc dependency plus explicit dlopen Vulkan;
libdrm headers are build input, direct DRM uses UAPI, driver owns its libdrm deps.

## Limits / residual / synchronization

Remote Actions runner/service/actions, upload/release/Issue/Project publication
not executed; local same target guests/procedure/fail gates are evidence, not a
remote CI result. Physical GPU/WiFi/audio hardware and display-manager session
start not re-certified; registration and direct session are tested. TCG software
composition screenshots may lag keyboard processing; actual command file effect
is independently checked. No bit-reproducibility claim; source/package/input and
installed dependency versions are recorded.

No new zedBSD image/boot run: no production source, image, toolchain or original
build-job change; WS107 latest boot is context, not a new WS108 test. No aggregate
make check, serial/console log inspection, host /opt/input installation or push.
q547 remains uncleared (earlier input/timing failures), q548 retry cleared with
failure histories retained. No production bug repair claimed. WS106 ime-probe
ownership and previous tracked resize bugs remain separate. MG007 whole acceptance
is not supplied by this WS. GitHub publication/comment/close projections remain
local outbox pending; local completion and remote sync are distinct.
