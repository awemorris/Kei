#!/bin/sh
# ws095-p003: converts the 100 sentences of ja-sentences.tsv with the given
# dictionaries and counts how many come out right as a whole on the first
# candidates.  Needs build/ws095/host-engine (sh plan/ws095/tests/host-engine.sh).
#   sh plan/ws095/tests/measure.sh SYSTEM [SUPPLEMENT]
# ws095-p012: ENGINE names another engine binary, SENTENCES another sentence list
# (the held-out set is plan/ws095/tests/ja-heldout.tsv).
# Prints one line per sentence (ok or NG, the split, the expected text) and a total.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -e
cd "$(dirname "$0")/../../.."
engine=${ENGINE:-build/ws095/host-engine}
test -x "$engine" || { echo "measure: build $engine first" >&2; exit 1; }
system=$1
supplement=${2:-}
sentences=${SENTENCES:-plan/ws095/tests/ja-sentences.tsv}
readings=$(grep -v '^#' "$sentences" | cut -f1)
# shellcheck disable=SC2086
if test -n "$supplement"; then
	"$engine" convert "$system" "$supplement" -- $readings
else
	"$engine" convert "$system" -- $readings
fi | awk -F '\t' -v list="$sentences" '
	BEGIN {
		while ((getline line < list) > 0) {
			if (line ~ /^#/) continue
			split(line, field, "\t")
			expected[field[1]] = field[2]
		}
	}
	{
		split_text = $2
		joined = split_text
		gsub(/\|/, "", joined)
		total++
		if (joined == expected[$1]) { good++; mark = "ok" } else { mark = "NG" }
		printf "%s\t%s\t%s\t%s\n", mark, $1, split_text, expected[$1]
	}
	END { printf "measure: %d of %d sentences right (%.0f%%)\n", good, total, 100 * good / total }'
