#!/bin/sh
# ws073-p045: the guest half of the namespace's shared-lookup stress (run in the guest): for SECONDS, four lookers
# stat names that come and go in DIR while two changers create, rename and remove them (and fsync now and then),
# and the journal commits meanwhile.  Prints CHURN done and the names left, or nothing when a step hangs (the host's
# timeout then tells).
#   sh p045-churn.sh DIR SECONDS
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
dir=$1
seconds=$2
rm -rf "$dir"
mkdir -p "$dir"
end=$(( $(date +%s) + seconds ))
looker() {
	while [ "$(date +%s)" -lt "$end" ]; do
		for n in 0 1 2 3 4 5 6 7 8 9; do
			[ -e "$dir/f$n" ] || [ -e "$dir/g$n" ] || true
			ls "$dir" > /dev/null 2>&1
		done
	done
}
changer() {
	i=0
	while [ "$(date +%s)" -lt "$end" ]; do
		n=$(( (i + $1) % 10 ))
		echo "$i" > "$dir/f$n"
		mv "$dir/f$n" "$dir/g$n" 2>/dev/null
		mkdir "$dir/d$n" 2>/dev/null && rmdir "$dir/d$n"
		rm -f "$dir/g$n"
		[ $((i % 20)) -eq 0 ] && sync
		i=$((i + 1))
	done
	echo "CHANGER $1 steps=$i"
}
looker & looker & looker & looker &
changer 0 & changer 5 &
wait
echo "CHURN done left=$(ls "$dir" | wc -l)"
