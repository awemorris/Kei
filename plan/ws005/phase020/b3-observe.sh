#!/bin/sh
# ws005-p020: run in the guest (nohup) while plan/ws005/phase020/qmp-cmd.py unplugs and replugs the usb-net ("ecm").
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
# B3 observer: records routes, resolver and an external fetch while the wired link goes away and comes back.
out=/tmp/b3.txt
: > $out
i=0
while [ $i -lt 12 ]; do
	echo "== t=$((i*8))" >> $out
	ifconfig ue0 | head -1 >> $out
	route show | grep default >> $out
	grep nameserver /etc/resolv.conf >> $out
	if fetch -q -o /dev/null http://example.com/ >> $out 2>&1; then echo "fetch ok" >> $out; else echo "fetch fail" >> $out; fi
	sleep 8
	i=$((i+1))
done
echo done >> $out
