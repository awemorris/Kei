# q578 main fresh image preflight

Source: main5ac9b753dfa75420ff6cf4bb6545f338ad8ffd31（browser2 source取込前）。
Command: sh plan/ws075/demo/build-demo-image.sh build/main-n3-demo-pt passthrough -o userland/base/noct/noct/build-zedbsd-amd64/.zedbsd-built-2.0.1-zedbsd12-t1-accel -o build/NoctLang/build-static/noct -o build/host-noct-state/built-2.0.1-zedbsd12-process

- 前のmake -n 3495linesにNoct/LLVM cmake/build/patch invocation無しを確認。Noct canonical accel artifactを再利用、toolchain4trees lock維持。全demo選択を保持、アプリ除外なし。
- Build exit0、ours warnings0（全1929lines）、check-amd64-native-image OK。
- image72003343313d85b5e0950f5659ca6af5a6183ef3c0776c68a1e7ecdfba0b3e7e
- wayland2cf785aea72e344ba1f957ba481328d87afcdb62d01f130ddf65fa7bc47a5732
- libbrowser8fd7ca50f0d7e48369eb4a4ca3cf4ba1e46a61c7bda39e2fe03acb7ee2bbd51e
- noct1f56cfb5b28fe8018cee8cce771f31b8f18545f25535e1d4212af56db247abe7

Framebuffer boot-test1: exit1 QMP screendump recv TimeoutError、PNG無し。kernel/serial logは判定に使わず、boot合格は主張しない。有限1回再試行の結果を追記する。
Hardware short/60minはP9が独立所有fixtureで実施し、このbuild/旧shortのみでwhole-Phase clearにしない。

Framebuffer boot-test2: 同じQMPscreendump TimeoutErrorでexit1、PNG無し。無限再試行せずこの補助確認を未達として保持。passthrough configのhardware受け入れはP9の実PNG/ログで別途判定する。
