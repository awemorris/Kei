# q597 checkpoint85 / Event binding full source review

Fully reviewed original2496 and current2603 lines of `bind/event.c` against full C/component standards and Guardrail. Reviewed204/209,5 production C/header pending; all209 inventory hashes match. C string literal contents and public ABI are unchanged.

Fresh Event and EventTarget state stays live until wrapper publication. Event constructors, preparation and dispatch retain wrappers and detached listener snapshots across reentrant getters and callbacks. Root removal covers success and failure. Event getter return paths now retain distinct success and failure outcomes. The adjacent `bind/input.c` KeyboardEvent/WheelEvent caller root gap was found, but its source is outside q597's exact39-file scope; repair requires selection in a later Queue.

GCC14.2 whole host build and event scoped compile exit0/warning0. Style checker reports only15 permitted forward cleanup jumps; clang-format19.1.7 ran on a copy. Click lifetime10/10, JS14/14 and DOM23/23 pass. Forced GC in every constructor getter/dispatch callback window remains untested.

Resume from [remaining5](remaining.json) and original whole p172 gates. p100 remains blocked/unselected. GitHub Issue/Project publication remains deferred.
