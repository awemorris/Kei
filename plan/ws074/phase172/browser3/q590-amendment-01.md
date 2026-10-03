# q590 internal lifetime correction / 2026-10-02

During complete review of bind/style-context.c, the initial active-child check
was found to precede bind_frame_viewport's synchronous host callback. The native
host may remove the connected frame during that callback. bind_frame_viewport
already handles retirement, but its caller still allocated and returned a new
style context for the now-detached Window, contradicting its existing contract.

The real existing host-frame-viewport fixture was extended to enter through
bind_style_context_engine while its existing host callback removes the iframe
and collects with conservative stack scanning disabled. Before the fix it
returns exit1, 10/12 checks, with both new assertions failing: a non-NULL child
cascade is returned and a replacement cache is created after retirement.

Correction: after successful bind_frame_viewport, check window->detached again
and return the already initialized NULL engine. Existing callers either see no
child cascade or ENOENT for unresolved style. The active-child path, public ABI,
callback timing and original failure conventions are preserved. The matching
native fixture also receives its scoped full-standard review.

This is an internal p172 imported-source lifetime correction under the existing
review/repair goal, recorded distinctly from style changes. No foreign Phase,
interface, dependency or WS criterion changes; q590 timebox/snapshot remain.
Evidence is retained in checkpoint15; no outcome is claimed before post-fix
execution. Subsequent complete review counts cover the current corrected file.
