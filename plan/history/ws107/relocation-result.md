# q542 移動結果

engine165files（133C）のmove/hash/mode確認、うち生成器4filesだけlocator更新、表/shader binary未再生成。libbrowser/appのsource変数とprivate includeを分離、registry/exports/API v2/install保存。target libbrowser133fresh source＋browser7forced source/probe build exit0/warning0。host clean build exit0/warning0、public-only browser-probeのDT_NEEDEDはlibbrowser.so/libcのみ、libraryはhost標準Vulkan/libm/libcのみ（targetもWayland無し）。list-sources140C、runner syntax/diff-check PASS。

Commands: make -j16 -W <browser main/shell 7source> build/amd64/dynamic/libbrowser.so build/amd64/bin/browser build/amd64/bin/browser-probe; BROWSER_HOST_BUILD=build/ws107-host sh plan/ws074/tests/host-build.sh plain; readelf -d各ELF。logs: build/ws107-p002/{target,host}-build.log。

q543は有限quality修正とcallback契約、q544は全文規約と最終bootの確認。14既存style候補と移動部分のstyle判断はp003/p004へ。
