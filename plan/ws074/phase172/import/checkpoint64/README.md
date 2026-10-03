# q597 checkpoint64 / CSS tokenizer full source review

Fully reviewed the current 1104 lines of `css/tokenizer.c` against full C/component standards and Guardrail. All209 inventory hashes match source; reviewed183/209,26 production C/header pending. All original C string literals and public ABI are unchanged. The review traced input preprocessing, comments, punctuation, names/escapes, numbers, strings, URLs, token spans, arena ownership, vector growth and failure paths.

A bad string ending at CRLF previously rewound only one UTF16 unit after `lex_next` consumed both. The direct tokenizer fixture showed BAD_STRING ending at offset3 and whitespace starting at3 for input `"a\r\nx`; after restoring the saved position, the spans are2 and2. `lex_peek` now checks the remaining input before adding lookahead, avoiding position-plus-ahead overflow. The offset fixture uses previous and repaired host objects with otherwise identical source; [exact evidence](offset-probe.json).

GCC14.2 scoped compile and whole host build exit0/warning0. Style checker exit0/findings0; `clang-format-19` v19.1.7 ran on a copy. Native color media39/39, CSS rule model44/44, CSSOM12/12, native style source21/21 and DOM23/23 pass. The values style dump exactly matches its checked-in golden. No huge-buffer injection is claimed for the defensive arithmetic change.

Resume from [remaining26](remaining.json) and original whole p172 gates. p100 is blocked/unselected until whole p172 clearance and actual output. GitHub Issue/Project publication remains deferred.
