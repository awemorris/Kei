# q597 checkpoint75 / HTML insertion modes full source review

Fully reviewed unchanged2119 lines of `html/modes.c` against full C/component standards and Guardrail. Reviewed194/209,15 production C/header pending; all209 inventory hashes match. The review covered initial/doctype quirks selection, before/in/after head and body, script text, table and foster parenting, caption/column/row/cell, template, frameset, EOF handling, native table text ownership and rooted DOM doctype identifiers. No source change was needed; public ABI and C literals are unchanged.

GCC14.2 scoped compile exit0/warning0; style checker reports only five permitted forward cleanup jumps in rooted DOCTYPE construction; clang-format19.1.7 ran on a copy. The unchanged production source had whole host build exit0/warning0 in [checkpoint73](../checkpoint73/README.md). Focused tree cases 5/5 pass: doctype/head/body, implicit tbody and cell closure, foster parenting outside a table, template content and legacy DOCTYPE. html5lib tree corpus is absent locally and was not counted.

Resume from [remaining15](remaining.json) and original whole p172 gates. p100 remains blocked/unselected. GitHub Issue/Project publication remains deferred.
