# q590 internal MIME parser correction / 2026-10-02

Full review of net/data.c found two actual MIME parsing errors: serialized quoted
value text such as a=";b=x" was mistaken for a preexisting b parameter, dropping
later b=y; and the subtype incorrectly stripped leading whitespace, accepting
text/ html instead of MIME-only fallback. The current primary
[MIME Sniffing Standard parsing/serialization algorithms](https://mimesniff.spec.whatwg.org/#parsing-a-mime-type)
(accessed2026-10-02, page dated17 July2026) validate independent expectations:
quoted values are collected as values, ordered parameter keys are deduplicated,
and only trailing subtype whitespace is removed.

Four ordinary inputs added to the existing host-data-url corpus reproduce four
MIME failures against actual baseline39091b2b9 engine objects:76/80, exit1.
They cover quoted delimiters, case-folded real duplicates, escaped quote and
invalid leading subtype whitespace; full patch/commands/raw evidence belong to
checkpoint25. The supporting fixture already has a checkpoint11 full review inside the209
import inventory. Revalidate the changed corpus and update its receipt/hash
without counting it as an additional newly reviewed path.

Correction: recognize quoted/escaped serialized value regions while searching
for actual parameter names, retain existing first-key/case-fold behavior, and
trim only trailing subtype whitespace. Related conformance adds immediate
fallible guards and complete C buffer cleanup without erasing body errors via
MIME fallback. No public interface, normal valid MIME output, loading profile,
foreign Phase, dependency, acceptance or product/architecture decision changes.
This narrow correction remains within the finite p172 full review/repair scope;
final outcome is recorded only after checkpoint25 verification.
