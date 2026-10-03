# q597 checkpoint76 / HTML parser full source review

Fully reviewed original2550 and current2576 lines of `html/parser.c` against full C/component standards and Guardrail. Reviewed195/209,14 production C/header pending; all209 inventory hashes match. C string literal instances and public ABI are unchanged. The review covered parser ownership/tracing and fragment contexts, streaming and script writes, open/formatting/template stacks, scope and foster parenting, adoption agency, text and token dispatch, node creation and failure paths.

Detached elements now have an explicit temporary GC root while attribute values and template contents are allocated. All three attribute failure exits remove it through one cleanup path; the implied-template factory releases its root after content creation. The existing parser failure state and return contract are retained. A collector forced exactly inside this allocation window was not run.

GCC14.2 whole host build and scoped compile exit0/warning0. Style checker lists only three permitted forward cleanup jumps; clang-format19.1.7 ran on a copy. Native parser8/8, document stream9/9, table row75/75, DOM23/23, and focused attribute/template/table tree creation3/3 pass.

Resume from [remaining14](remaining.json) and original whole p172 gates. p100 remains blocked/unselected. GitHub Issue/Project publication remains deferred.
