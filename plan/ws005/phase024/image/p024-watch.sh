#!/bin/sh
# ws005-p024: a test-image service (root) that records the Wi-Fi state and stages the system's store.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
#
# Every change networkd announces ("net watch") is appended to /var/log/p024-wifi.log as
# "EPOCH wifi state=... interface=... ssid=HEX radios=N", synced at once, so the log survives a hard stop of
# the passthrough QEMU and can be read from the image afterwards (plan/ws031/tests/ufs-cat.py).
# When kei creates ~/p024-system, kei's saved networks are copied to the system's store (/etc/wifi.conf,
# root's, mode 600): the test stages "an AP saved in the system's store" without a key on any command line.
log=/var/log/p024-wifi.log
echo "$(date +%s) p024-watch start" >> "$log"
sync
(
	while :; do
		if [ -f /home/kei/p024-system ]; then
			rm -f /home/kei/p024-system
			cp /home/kei/.wifi.conf /etc/wifi.conf.new && chown root:root /etc/wifi.conf.new &&
				chmod 600 /etc/wifi.conf.new && mv /etc/wifi.conf.new /etc/wifi.conf &&
				echo "$(date +%s) p024-watch system store staged" >> "$log"
			sync
		fi
		sleep 2
	done
) &
/sbin/net watch 2>&1 | while IFS= read -r line; do
	case "$line" in
	wifi*)
		echo "$(date +%s) $line" >> "$log"
		sync
		;;
	esac
done
