# libvulkan-compat

Linux の Keiland に使う Vulkan library。zedBSD の `userland/desktop/libvulkan/` とは別の実装である。
こちらは描画 driver を実装せず、system の Vulkan に chain し、Keiland の WSI を持つ。

```text
Keiland app (DT_NEEDED libvulkan.so.1)
  → /opt/keiland/lib/libvulkan.so.1 (this library)
      → our Wayland / KMS WSI
      → absolute-path dlopen(system libvulkan.so.1)
          → system loader / vendor Vulkan → driver
```

## 関数と handle

`functions.tsv` が関数の責務と export の正本。Vulkan 1.4.309 の prototype から作り、手動で分類を保つ。
`gen-forward.sh` は build directory の `forward.inc` と `exports.map` を生成する。F は core の素通し、I は横取り、O は自前の WSI、N は拒む名前。
現段階（ws105-p005）は F 222 / I 12 / O 26 / N 25。Wayland / KMS の surface / swapchain と single-device group を自前で実装する。Layer / version の enumeration も、後段の無いときの規定の fallback を持つため I とする。

Dispatchable handle は後段の値をそのまま使い、包まない。F は後段の export の `dlsym` pointer を呼ぶ。
I の後段の版も必ず `dlsym` から得る。後段の GetProcAddr の答えを I の「次」にしない。
instance / device / queue の小さな ownership record は mutex で守り、Vulkan の規定どおり caller が handle の lifetime を守る。
後段の WSI 関数に、こちらの surface / swapchain handle を渡してはならない。

## 後段の選択

- `KEILAND_VULKAN_BACKEND`（絶対 path）
- `/opt/keiland/etc/vulkan-backend` の1行目（絶対 path）
- 上記の指定が無ければ build 時の multiarch の絶対 path 一覧

明示した環境変数・file の選択は authoritative。壊れた指定を system の既定に隠さず、診断して失敗する。
自分自身の realpath / entry address は拒む。後段は `pthread_once` で1回だけ開き、process が終わるまで保持する。
既定は `RTLD_DEEPBIND`。`KEILAND_VULKAN_NO_DEEPBIND=1` は、preload した allocator などの symbol binding を必要とする利用者のための設定。
library は `-Bsymbolic` で link する。

F / I の入口は pthread の thread-local record で再入を検出し、同じ関数への再入に診断と abort を返す。
thread ごとの record は pthread key の destructor が解放する。初期化は process ごとに1回、stack は最大64呼び出し。
後段が無ければ instance creation は INCOMPATIBLE_DRIVER、instance extension / layer enumeration は空、version は1.0。
後段が持たない core の直呼びは診断して abort する。

## Build と試験

`make keiland-linux`（gcc / clang）、`make keiland-linux-install DESTDIR=...`。install の SONAME は `libvulkan.so.1`、RUNPATH は `/opt/keiland/lib`。
我々の app はこの SONAME に link し、system の Wayland を使う外部 app は対象に含めない。
[試験の道具](../../../plan/tools/keiland-linux/README.md)を参照する。host では DRM device を `none` にし、Wayland / X の環境変数を外す。
Wayland は version-three linux-dmabuf と surface 専用 event queue、backend から export した単一 plane image を使う。
implicit sync は DMA_BUF_IOCTL_IMPORT_SYNC_FILE、能力不足や ENOTTY/EINVAL/EPERM では CPU fence 待ち。試験専用の production 環境変数は無い。
KMS は問い合わせで master を保持せず、seat fd の duplicate または直接 acquire を使う。OPTIMAL image → coherent readback → double dumb buffer のコピーで、最初は modeset、続きは 100 ms 上限の page flip を待つ。破棄で acquire 時の CRTC を戻す。

`timeout 120 bash plan/tools/keiland-linux/wsi-check.sh` は FIFO / fallback / resize / MAILBOX を各90 frameで検証する。
試験 observer は kernel import の成否と private fence wait を記録する。raw SYNC_IOC_FILE_INFO の fence 数・driver/timeline はそのまま残す。
kernel の stub は空の reservation にも現れるため、raw fence 数だけを payload の受け渡しの証明にしない。
後段の RTLD_DEEPBIND は observer の run でも維持する。実機 GPU の非同期待ちは未検証。
