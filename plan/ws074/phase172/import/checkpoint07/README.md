# A1 / q584-i01 checkpoint07

Full current C/component review completed for `bind/svg-length.c`, `bind/xml.c`,
`xml/dom.c`, and `html/input.c`; [inventory](review-inventory.json) fixes final
hashes. Total reviewed: 16/209; remaining133 C/header and60 other entries.
The whole p172 criteria remain unmet and downstream browser work stays blocked.

The complete review covers declarations and file/public/static order; actual
owner/tracer relationships; allocation and immediate guarded failure handling;
ANSI C locals; exact short-circuit order and coercion-before-readonly refusal;
paragraphs, loops/switches and braces; explicit success/failure conventions;
protocol flags/counters; immutable table lifetime; licenses and default behavior.
Exactly ten SVG and one PI callback declarations precede their constant initializer
tables because those tables use them. Unrelated helpers retain the normal block.
No public API/ABI, Wayland dependency, runtime callback or resource lifetime changed.

SVG native brands, SameObject graph/root cleanup, creator-prototype snapshot,
scalar grammar/conversion and bounded serialization preserve their original
operations. XML projection retains the Document and pending names/node graph,
copies model storage independently, follows iterative source preorder, and
unwinds only registered root slots. The input stream preserves CR/LF folding,
inserted-text CR sentinel and tokenizer EOF state. PI getters still use genuine
native identity and immutable traced targets.

Repairs explain individual XML projection stages, table lifetimes and HTML input
protocol state. XML error-code locals are named `error`; three void input functions
now have explicit success returns. SVG requires only its file explanation layout
after final review of the existing checkpoint05 fixes. [Formatter](format.json)
records19.1.7; reviewed differences are definition tabs, readable immutable rows
and compound-clause indentation restored under the full standard. Macro spacing
follows formatter output. style-check0 and git diff-check supplement manual review.

[Object-code comparison](object-code.json): all four GCC14.2.0 objects compile
with -Wall/-Wextra/-Werror and no diagnostics; `.text` equals base4da5eb2e5.
Implementation tokens equal after the documented local-name and equivalent
trailing-void-return normalizations. The [serialized plain build](plain-build.log)
passes warning0. This is host evidence; no repeated target/runtime/boot claim.
Checkpoint02/03/04 runtime and main target/boot/p014 receipts remain retained.
No toolchain build, install, push, aggregate make-check or serial-log judgment.
