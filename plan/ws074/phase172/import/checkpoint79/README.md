# q597 checkpoint79 / Node binding full source review

Fully reviewed original1543 and current1565 lines of `bind/node.c` against full C/component standards and Guardrail. Reviewed198/209,11 production C/header pending; all209 inventory hashes match. C string literal instances and public ABI are unchanged. The review covered wrappers and ownership, Node getters/text mutations, pre-insertion and adoption, child arrays, cloning, DOM traversal, exact interface prototype selection and callback/error cleanup.

The shared node-array append helper now roots the result array in the caller heap and the wrapped node in its owning document heap until the array owns the wrapper. Callers in Node, mixin, query and event paths receive the same protection; earlier caller roots remain valid. A collector forced exactly inside this helper after wrapping was not run.

GCC14.2 whole host build and scoped compile exit0/warning0; style checker exit0/findings0 and clang-format19.1.7 on a copy. DOM23/23, native insertion29/29, cloning26 checks/0 failures, collection GC37/37 and XML binding34/34 pass.

Resume from [remaining11](remaining.json) and original whole p172 gates. p100 remains blocked/unselected. GitHub Issue/Project publication remains deferred.
