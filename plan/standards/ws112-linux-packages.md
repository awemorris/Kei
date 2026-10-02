# Linux Keiland パッケージ・リリース方針

Authority: 2026-10-02 current user、このchatの「WSを作ってあとで実装できるようにします」および Raspberry Pi OS「64 bit（arm64、推奨）」回答。
適用: [WS112](../ws112/ws.md) の将来の実装。計画作成の指示であり、実装・push・release公開の許可ではない。

## 必須契約

| make target | 対象OS | 成果物 |
| --- | --- | --- |
| `make keiland-linux-debian` | Debian 13 | `.deb` |
| `make keiland-linux-ubuntu2604` | Ubuntu 26.04 | `.deb` |
| `make keiland-linux-rpi` | Raspberry Pi OS / arm64 | `.deb` |
| `make keiland-linux-fedora44` | Fedora 44 | `.rpm` |
| `make keiland-linux-arch` | Arch Linux | Arch binary package |

全5種類を既存CIで作成し、既存release filesへ添付する。CIでの導入・起動・GUI等の動作確認は不要。
Raspberry Pi OSはuserの追加決定「ビルドが通ればOK」により、arm64 build/deb生成で受け入れる。理由はuser申告のQEMU GPUエミュレーション制約。GPU/GUI/表示・実機試験はclear条件にしない。package形式の確認は配布物の整合確認であり、runtime代替の動作試験を追加しない。
FreeBSDはソースからbuild/installする前提であり、パッケージを作らない。

正式なdistro devtoolや配布用source packageの作成は必須でない。必要なpayloadとmetadataをad hocに生成・圧縮してよい。
ただし各OSのpackage managerが解釈できる実際のbinary package形式にする。拡張子だけを変更したarchiveを成果物にしない。
軽量な既存encoderやquery/extraction toolsの利用は禁止しない。署名付き公式入力の検証、ライセンスと依存情報、root ownership/modeを保つ。

## 継承と変更

[WS108](../ws108/ws.md)の完了時結果は保持する。今回の新方針は今後のCIのruntime gateを置換し、過去のGUI/導入証拠を未実施へ書き換えない。
Debian/UbuntuのQEMU内native buildという先の指示は撤回されていないため維持する。新OSのbuild環境はWS112 p001で具体化する。
QEMU boot/SSH/consoleの扱いはGuardrailに従う。新しいOSへの既存例外の適用可否は環境を実行する前に照合する。

既存production payload、`/opt/keiland`、system提供libc/Vulkan等を利用する境界を引き継ぎ、共通`keiland-desktop`を収録する。
LinuxのGDM entryはwaylandを直接起動する。本人password/将来PINの`--login`や`--testing`実装はこのWSに含めない。
Cの全文規約の例外は追加しない。非Cの既存形式と全文/manual review、絞ったpackage形式・metadata・CI失敗検証を適用する。

2026-10-02 / ws112-rpi-build-only-20261002: current userのRPi build-only受け入れを反映。影響するp001/p003/p007・WS/design・release方針を更新、Queue/実装許可は追加しない。GitHub event deliveryは保留。
