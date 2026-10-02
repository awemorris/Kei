# A1 / q584-i01 checkpoint09

Complete C/component review of html/html.h, html/parser.h, bind/bind.h and
page/page.h covers every type, private interface and ownership/protocol comment.
[Inventory](review-inventory.json) fixes their final hashes. Reviewed80/209;
remaining129 C/header. p172/q584 stay in-progress; downstream gate stays closed.

File-wide macros now precede enums and types; HTML input and binding storage/
location enums retain their values. page's opaque helper declarations precede
its API functions and the page purpose comment directly precedes its definition.
Network CR/LF state, outer script/child task reentry guards and copied-versus-
borrowed host/storage lifetimes are explained from actual implementation use.
The four headers remain engine-private and expose no project window/Wayland/UAPI
API. The public browser ABI/callback contract is unchanged.

[Declaration comparison](declarations.json) verifies all struct/enum bodies and
field order, preprocessor directives and other declarations; only independent
section ordering differs. No types, fields, values or function prototypes change.
Every allocation/operation described remains owned by its original implementation;
header review does not claim full review of those pending implementation files.

[Formatter](format.json): clang-format19.1.7 with checked-in configuration; all
output retained. [Style checker](style-summary.txt) has zero candidates and git
diff-check passes. [Serialized own plain build](plain-build.log) succeeds warning0.
[Object comparison](object-code.json) verifies all168 host production objects
(167 engine and main) have exactly unchanged emitted .text before/after rebuild.
This compiler-specific evidence supports equivalent declaration/comment changes;
it is not a target compiler proof. Existing native host/ASan/target/boot/p014
receipts are retained, with no repeated guest/runtime claim. No toolchain build,
install, push, aggregate make-check or serial-log judgment.
