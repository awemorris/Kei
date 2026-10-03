# BUG-146: the client numbers of the applications a guest test started, whether or not zdesktop started the input
# method first (sourced by the Venus guest tests).
#
#   . plan/tools/guest/zwl-clients.sh
#   zwl_app_clients [LOG]      sets zc1 zc2 zc3 zc4: the 1st..4th application's client number in LOG
#                              (/tmp/zdesktop.log when omitted)
#   zwl_app_client N [LOG]     prints the Nth one (for helpers that take an application's index)
#
# zdesktop numbers its connections in one sequence.  When keiland-ime is installed it starts the input method on a
# connection of its own before ZWL READY ("ZWL CLIENT client=N fd=D ime=1"), so the first application the test
# starts is client 2, not client 1; an input method started again later takes a number in the middle.  The
# applications are the clients logged without ime=1, in the order they connected; one that has not connected yet
# gets the next numbers after the highest one seen (the 1st..4th of a log without clients are 1..4, as before).
# Waits up to 30 seconds for ZWL READY first, since the input method's line comes before it.
# Runs the guest command with $ZWL_RUN (guest when unset); variables of its own begin with zwl_.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib

zwl_app_clients() {
	zwl_log=${1:-/tmp/zdesktop.log}
	zwl_numbers=$(${ZWL_RUN:-guest} "i=0; while ! grep -q 'ZWL READY' $zwl_log 2>/dev/null && [ \$i -lt 60 ]; do sleep 0.5; i=\$((i+1)); done; grep 'ZWL CLIENT client=' $zwl_log 2>/dev/null" |
	    awk '/ZWL CLIENT client=[0-9]+ / {
			for (i = 1; i <= NF; i++)
				if ($i ~ /^client=[0-9]+$/)
					number = substr($i, 8) + 0
			if (number > highest)
				highest = number
			if ($0 !~ / ime=1/)
				app[++count] = number
		}
		END {
			for (k = 1; k <= 4; k++)
				printf "%d ", (k <= count) ? app[k] : highest + k - count
		}')
	set -- $zwl_numbers
	zc1=${1:-1}; zc2=${2:-2}; zc3=${3:-3}; zc4=${4:-4}
}

# The client number of the Nth application (1..4) in LOG, for helpers that take an application's index:
#   zwl_app_client N [LOG]
zwl_app_client() {
	zwl_app_clients "${2:-}"
	eval "echo \${zc$1}"
}
