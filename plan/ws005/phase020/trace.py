# ws005-p020: gdb (Python) tracer for the guest kernel: logs entry and return of the named functions.
# Every hit stops the guest briefly; the loop logs it and continues.  The run ends by SIGINT (trace.sh), after
# which the caller detaches and resumes the guest with QMP "cont".
#   TRACE_FUNCTIONS: comma separated "name[:fmt]"; fmt "dev" prints the net_device name at $rdi, "skip=NAME"
#   (fmt "dev!ue0") skips calls on that device.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
import os
import time
import gdb

LIMIT = int(os.environ.get("TRACE_LIMIT", "600"))
# TRACE_HW=1: hardware breakpoints (at most four with the returns), which avoid the KVM software-breakpoint
# race that can hand an int3 to a guest with several vCPUs.
KIND = gdb.BP_HARDWARE_BREAKPOINT if os.environ.get("TRACE_HW") == "1" else gdb.BP_BREAKPOINT


def reg(name):
	return int(gdb.parse_and_eval("$" + name)) & 0xffffffffffffffff


def read64(address):
	return int.from_bytes(gdb.selected_inferior().read_memory(address, 8).tobytes(), "little")


def device_name(address):
	try:
		data = gdb.selected_inferior().read_memory(address, 16).tobytes()
		return data.split(b"\0")[0].decode("ascii", "replace")
	except gdb.MemoryError:
		return "?"


def signed32(value):
	value &= 0xffffffff
	return value - (1 << 32) if value >> 31 else value


entries = {}
for item in os.environ["TRACE_FUNCTIONS"].split(","):
	name, _, fmt = item.partition(":")
	bp = gdb.Breakpoint("*" + name, type=KIND)
	entries[bp.number] = (name, fmt)
returns = {}
logged = 0
interrupted = [False]


def on_stop(event):
	if isinstance(event, gdb.SignalEvent):
		interrupted[0] = True


gdb.events.stop.connect(on_stop)
gdb.execute("set scheduler-locking off")
while logged < LIMIT:
	try:
		gdb.execute("continue", to_string=True)
	except (gdb.error, KeyboardInterrupt):
		break
	if interrupted[0]:
		break
	pc = reg("pc")
	rsp = reg("rsp")
	hit = None
	for bp in gdb.breakpoints():
		if bp.number in entries and int(gdb.parse_and_eval("(long)&" + entries[bp.number][0])) & (2**64 - 1) == pc:
			hit = bp
	key = (pc, rsp)
	if key in returns:
		bp, label = returns.pop(key)
		gdb.write("  <- %s = %d (rax %#x)\n" % (label, signed32(reg("rax")), reg("rax")))
		if not any(k[0] == pc for k in returns):
			bp.delete()
		logged += 1
		continue
	if hit is None:
		continue
	name, fmt = entries[hit.number]
	extra = ""
	if fmt.startswith("dev"):
		dev = device_name(reg("rdi"))
		if "!" in fmt and dev == fmt.split("!")[1]:
			continue
		extra = " dev=%s" % dev
	ret = read64(rsp)
	caller = gdb.execute("info symbol %#x" % ret, to_string=True).strip()
	gdb.write("%.3f -> %s rdi=%#x rsi=%#x rdx=%#x rcx=%#x%s from %s\n" % (time.time() % 1000, name, reg("rdi"), reg("rsi"), reg("rdx"), reg("rcx"), extra, caller))
	existing = [b for (p, s), (b, l) in returns.items() if p == ret]
	bp = existing[0] if existing else gdb.Breakpoint("*%#x" % ret, type=KIND, internal=True)
	returns[(ret, rsp + 8)] = (bp, name)
	logged += 1
gdb.write("trace: %d lines\n" % logged)
