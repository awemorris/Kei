# q590 internal data-resource cleanup correction / 2026-10-02

Full review of page/link.c found that legacy page_fetch releases decoded
net_data fields only when the later caller-buffer append succeeds. An append
failure therefore leaks independently owned MIME/body buffers. The existing
host-resource fixture now exercises the real buffer-overflow ENOMEM path after
genuine data decoding, four times, restoring its synthetic length before caller
teardown. A focused LeakSanitizer link against the actual baseline production
objects exits23 and reports512 bytes in8 allocations. No engine replacement or
production test control is used.

Correction: check decode and append separately, release decoded data on both
append outcomes, then release the resolved URL and propagate the original
error. Related full-file conformance uses immediate guarded operations and
explicit cleanup for owned URL/response/buffer fields. Public interfaces,
successful output, MIME/URL behavior and component boundaries stay unchanged.
No foreign Phase, cross-Phase dependency or acceptance criterion changes;
the existing finite p172 review/repair scope remains authorized. Final proof
belongs to checkpoint18 and no repaired outcome is claimed before execution.
