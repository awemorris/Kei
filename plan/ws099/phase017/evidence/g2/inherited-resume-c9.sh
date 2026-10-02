#!/bin/sh
set -u
cd /home/awe/zedBSD-worktrees/p8 || exit 1
for index in 2 3 4 5; do
 out=build/p8-q577/c9-$index
 sh plan/ws099/tests/criteria.sh build/p8-q577/criteria.img "$out" C9 > "$out.txt" 2>&1
 status=$?
 count=$(grep -c '^C9 .* PASS ' "$out/results.txt" || true)
 failures=$(grep -c '^C9 .* FAIL ' "$out/results.txt" || true)
 printf 'C9 run=%s exit=%s passed=%s failed=%s\n' "$index" "$status" "$count" "$failures" >> build/p8-q577/c9-resumed.txt
 if [ "$status" -ne 0 ] || [ "$count" -ne 10 ] || [ "$failures" -ne 0 ]; then echo 1 > build/p8-q577/c9-resume.status; exit 1; fi
done
echo 0 > build/p8-q577/c9-resume.status
