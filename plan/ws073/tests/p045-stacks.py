# ws073-p045 (BUG-135): a gdb (Python) script for the guest's gdbstub with a kernel built with DWARF and frame
# pointers (ZEDBSD_KERNEL_LTO_CFLAGS="-g -fno-omit-frame-pointer").  It stops the guest once, prints the kernel
# stack of every thread of the processes whose command holds NAME (from the saved context of a thread that is not
# running: asm_task_dispatch's pushes, then the frame pointers), and lets the guest go.
#   gdb -q -batch -ex 'set $port=PORT' -ex 'set $name="fsprobe"' -x plan/ws073/tests/p045-stacks.py build/<x>/vmunix
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
import gdb

KERNEL_LOW = 0xffff800000000000


def run(command):
	return gdb.execute(command, to_string=True)


def q(address):
	return int(gdb.parse_and_eval(f"*(unsigned long long *){address:#x}")) & 0xffffffffffffffff


def symbol(address):
	text = run(f"info symbol {address:#x}").strip()
	if text.startswith("No symbol"):
		return f"{address:#x}"
	return text.split(" in section")[0]


def line(address):
	text = run(f"info line *{address:#x}").strip()
	if text.startswith("Line "):
		parts = text.split()
		return f"{parts[3]}:{parts[1]}"
	return ""


port = int(gdb.parse_and_eval("$port"))
name = gdb.parse_and_eval("$name").string()
run("set pagination off")
run(f"target remote 127.0.0.1:{port}")
process = gdb.parse_and_eval("all_processes")
while int(process) != 0:
	command = process["command"].string()
	if name in command:
		thread = process["threads"]
		while int(thread) != 0:
			state = str(thread["state"])
			task = thread["task"].cast(gdb.lookup_type("struct amd64_task").pointer())
			rsp = int(task["resume_rsp"]) & 0xffffffffffffffff
			print(f"THREAD pid={int(process['pid'])} tid={int(thread['tid'])} state={state} rsp={rsp:#x}")
			if "RUNNING" not in state and rsp >= KERNEL_LOW:
				rip = q(rsp + 56)
				rbp = q(rsp + 32)
				print(f"  #0 {symbol(rip)} {line(rip)}")
				for depth in range(1, 24):
					if rbp < KERNEL_LOW:
						break
					rip = q(rbp + 8)
					if rip < KERNEL_LOW:
						break
					print(f"  #{depth} {symbol(rip)} {line(rip - 1)}")
					rbp = q(rbp)
			thread = thread["proc_next"]
	process = process["all_next"]
gdb.execute("detach")
