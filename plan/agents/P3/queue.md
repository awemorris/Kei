# P3 Queue lane

| Queue / attempt | Phase | Scope | Approval | Timebox | State |
| --- | --- | --- | --- | --- | --- |
| q592 / q592-i01 | [ws114-p007](../../ws114/phase007/phase.md) | CSD/SSD の残り5点（新 attempt） | 2026-10-02 user「作業を開始しましょう。」 | 3h | finished / cleared |

| q597 / q597-i01 | [ws115-p001](../../ws115/phase001/phase.md) | 素の GTK4 移植の契約 | user 2026-10-02「まずは素のGTK4を移植してください」 | 4h | finished / cleared |
| q600 / q600-i01 | [ws115-p004](../../ws115/phase004/phase.md) | meson の cross 契約と host 道具 | 継続 dispatch | 4h | finished / cleared |
| q602 / q602-i01 | [ws115-p005](../../ws115/phase005/phase.md) | libffi・pcre2・glib | 継続 dispatch | 4h | in-progress |

Next（予約）: ws115-p006（libpng・freetype・harfbuzz・fontconfig）〜

## Merge requests

| P3-001 | q592 | 8856edf21（base 901037f9f） | plan/ws114 evidence/tests のみ | integrated debba7e9e |
| P3-002 | q592 | b2ebbfee6（前回 8856edf21） | plan/ws114 evidence/結果 | integrated d10ad2cf3 |
| P3-003 | q592 | 597841d2a（前回 b2ebbfee6） | plan/ws114 | integrated 3b985ae4c |
| P3-004 | q597 | 33e061c3b..be587f8ae（前回 597841d2a） | plan/ws115 | integrated 408a31586 |
| P3-005 | q600 | 503b94c80（前回 be587f8ae） | external.mk・packages/tools/gen-meson-cross.sh・devel/gperf・plan/ws115 | integrated 81f77738d |
| P3-006 | q602 途中 | 24c66c58e（前回 503b94c80） | packages/libs/pcre2・libffi・plan/ws115（proposed の libc 差分 2 つはユーザーの承認待ち） | integrated dea890041 |
| P3-007 | q602 wrap | c4e303ffa（前回 24c66c58e） | packages/libs/glib・plan/ws115 | integrated 698759738 |
| P3-008 | q602 | 150c2f51f（base 798ad97bc） | include/libc/libintl.h・sys/socket.h（ユーザー許可の libc 差分） | integrated 871777b34 |
| P3-009 | q602 | 31155f84d..958b6c060（前回 150c2f51f） | packages/libs/glib・plan/ws115 | integrated（cleared） |
| q605 / q605-i01 | ws115-p006 | libpng・freetype・harfbuzz・fontconfig | 継続 dispatch | 4h | finished / cleared |
| P3-010 | q605 | 19de99f8e..cbbc1607f（前回 958b6c060） | packages/libs/{libpng,freetype,harfbuzz,fontconfig}・plan/ws115 | integrated 3c33fb259 |
| q608 / q608-i01 | ws115-p007 | pixman・cairo・fribidi・pango | 継続 dispatch | 4h | finished / cleared |
| P3-011 | q608 | 53b5dbe6b..03f00921d（前回 cbbc1607f） | packages/libs/{pixman,fribidi,cairo,pango}・plan/ws115 | integrated db8f21553 |
| q610 / q610-i01 | ws115-p008 | gdk-pixbuf・jpeg・tiff・graphene・libepoxy・libxkbcommon | 継続 dispatch | 4h | in-progress |
