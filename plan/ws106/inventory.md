# WS106 移動対象の台帳（2026-10-01）

調査 source: `01c754a0`。移動前の案。base 21 + desktop 9 = **30 package**。
ユーザー回答「mview と gpudemo も移動候補に含める」を反映。

| 移動元 | 移動先 | 理由 / 既存 Linux Makefile |
| --- | --- | --- |
| `userland/base/tests/acquire-fence/` | `userland/tests/acquire-fence/` | 既存 test/probe/injector/benchmark package / 無し |
| `userland/base/tests/browser-probe/` | `userland/tests/browser-probe/` | 既存 test/probe/injector/benchmark package / 無し |
| `userland/base/tests/data-probe/` | `userland/tests/data-probe/` | 既存 test/probe/injector/benchmark package / 無し |
| `userland/base/tests/extras-probe/` | `userland/tests/extras-probe/` | 既存 test/probe/injector/benchmark package / 無し |
| `userland/base/tests/gpu-admission/` | `userland/tests/gpu-admission/` | 既存 test/probe/injector/benchmark package / 無し |
| `userland/base/tests/gpu-fence/` | `userland/tests/gpu-fence/` | 既存 test/probe/injector/benchmark package / 無し |
| `userland/base/tests/gpu-forge/` | `userland/tests/gpu-forge/` | 既存 test/probe/injector/benchmark package / 無し |
| `userland/base/tests/gpu-i915/` | `userland/tests/gpu-i915/` | 既存 test/probe/injector/benchmark package / 無し |
| `userland/base/tests/gpu-recovery/` | `userland/tests/gpu-recovery/` | 既存 test/probe/injector/benchmark package / 無し |
| `userland/base/tests/gpu-share/` | `userland/tests/gpu-share/` | 既存 test/probe/injector/benchmark package / 無し |
| `userland/base/tests/greeter-probe/` | `userland/tests/greeter-probe/` | 既存 test/probe/injector/benchmark package / 無し |
| `userland/base/tests/menu-probe/` | `userland/tests/menu-probe/` | 既存 test/probe/injector/benchmark package / 無し |
| `userland/base/tests/network-probe/` | `userland/tests/network-probe/` | 既存 test/probe/injector/benchmark package / 無し |
| `userland/base/tests/peninject/` | `userland/tests/peninject/` | 既存 test/probe/injector/benchmark package / 無し |
| `userland/base/tests/popup-probe/` | `userland/tests/popup-probe/` | 既存 test/probe/injector/benchmark package / 無し |
| `userland/base/tests/seat-probe/` | `userland/tests/seat-probe/` | 既存 test/probe/injector/benchmark package / 無し |
| `userland/base/tests/subsurface-probe/` | `userland/tests/subsurface-probe/` | 既存 test/probe/injector/benchmark package / 無し |
| `userland/base/tests/tablet-probe/` | `userland/tests/tablet-probe/` | 既存 test/probe/injector/benchmark package / 無し |
| `userland/base/tests/titlebar-probe/` | `userland/tests/titlebar-probe/` | 既存 test/probe/injector/benchmark package / 無し |
| `userland/base/tests/touchinject/` | `userland/tests/touchinject/` | 既存 test/probe/injector/benchmark package / 無し |
| `userland/base/tests/wakebench/` | `userland/tests/wakebench/` | 既存 test/probe/injector/benchmark package / 無し |
| `userland/desktop/egltest/` | `userland/tests/egltest/` | EGL/GLES の描画・format 等の試験 / 無し |
| `userland/desktop/glescompute/` | `userland/tests/glescompute/` | GLES compute の試験 / 無し |
| `userland/desktop/ime-probe/` | `userland/tests/ime-probe/` | IME/text-input の probe（WS095 は人間作業中） / 無し |
| `userland/desktop/kuidemo/` | `userland/tests/kuidemo/` | UI widget の sampler / 有り |
| `userland/desktop/vkdemo/` | `userland/tests/vkdemo/` | Vulkan の試験/表示の見本 / 有り |
| `userland/desktop/wlshm/` | `userland/tests/wlshm/` | wl_shm の試験 client / 有り |
| `userland/desktop/wltest/` | `userland/tests/wltest/` | Wayland/Vulkan の試験 client / 有り |
| `userland/desktop/mview/` | `userland/tests/mview/` | model viewer / demo（ユーザー追加指定） / 有り |
| `userland/desktop/gpudemo/` | `userland/tests/gpudemo/` | Noct CPU/GPU デモデータ（ユーザー追加指定、S13） / 無し |

## 残すもの

- `userland/base/test/`: POSIX の test utility。test app ではない。
- production app/library/compositor、外部 package、plan/tools の host fixture は現位置。
- 既存 Phase/Queue history の旧経路は過去の証拠。現役 runner を更新し、必要なら履歴へ dated locator follow-up をリンク。

## p001 で全数確認する参照

1. top Makefile の package wildcard（`userland/*/*/Makefile` で移動先を発見可能）と旧 base/tests grouping。
2. 各 package の source/asset/relative include、desktop/package.mk→base/package.mk の再利用方針。
3. package ID・path/category・dependencies・ZEDBSD config の選択名。grouping tests と新 root を二重登録しない。
4. `userland/desktop/keiland-linux.mk` の includes、各 Makefile.linux、App Home と試験の実行/install の path。
5. `plan/tools/keiland-linux/makefile-sync.sh` 等の locator 仮定、WS101 S13 と mview model/texture、vkdemo/egltest shader。
6. `ime-probe` の所有調整。選定前に実際の人間作業を確認し、同一ファイルへ並行 write しない。

WS106 は既存 Linux 対応5本を維持する。残りを Linux/FreeBSD に新規移植する仕事はこの移動に加えない。
全ファイルの tracked inventory・hash と runner の参照一覧は p001 の成果として保存する。
