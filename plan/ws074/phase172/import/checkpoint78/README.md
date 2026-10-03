# q597 checkpoint78 / Element binding full source review

Fully reviewed original1420 and current1468 lines of `bind/element.c` against full C/component standards and Guardrail. Reviewed197/209,12 production C/header pending; all209 inventory hashes match. C string literal instances and public ABI are unchanged. The review covered Element/HTMLElement/image/script IDL tables, attribute names and reflection, class scanning, synthetic click, native rendered image dimensions and callback ownership, async script ordering and error cleanup.

The temporary name and empty string used by `hidden` and `script.async` setters now have explicit roots through the next string allocation and attribute dispatch. Both setters release only registered roots on success and failure. A collector forced exactly inside these allocation windows was not run.

GCC14.2 whole host build and scoped compile exit0/warning0. Style checker lists only four permitted forward cleanup jumps; clang-format19.1.7 ran on a copy. DOM23/23 including elements and dynamic scripts, form29 checks/0 failures, and native image geometry10/10 pass.

Resume from [remaining12](remaining.json) and original whole p172 gates. p100 remains blocked/unselected. GitHub Issue/Project publication remains deferred.
