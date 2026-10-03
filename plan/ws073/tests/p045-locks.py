# ws073-p045 (BUG-135): a gdb (Python) script for the guest's gdbstub with a kernel built with DWARF and frame
# pointers.  It stops the guest once and, for each mounted UFS volume, prints every mutex of its mount state that
# is held: the owner thread, its process and its kernel stack (as p045-stacks.py unwinds it), then lets it go.
#   gdb -q -batch -ex 'set $port=PORT' -x plan/ws073/tests/p045-locks.py build/<x>/vmunix
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


def stack(thread):
	state = str(thread["state"])
	task = thread["task"].cast(gdb.lookup_type("struct amd64_task").pointer())
	rsp = int(task["resume_rsp"]) & 0xffffffffffffffff
	process = thread["proc"]
	command = process["command"].string() if int(process) != 0 else "?"
	print(f"    owner tid={int(thread['tid'])} process={command!r} state={state}")
	if "RUNNING" in state or rsp < KERNEL_LOW:
		return
	rip = q(rsp + 56)
	rbp = q(rsp + 32)
	print(f"      #0 {symbol(rip)} {line(rip)}")
	for depth in range(1, 24):
		if rbp < KERNEL_LOW:
			break
		rip = q(rbp + 8)
		if rip < KERNEL_LOW:
			break
		print(f"      #{depth} {symbol(rip)} {line(rip - 1)}")
		rbp = q(rbp)


port = int(gdb.parse_and_eval("$port"))
run("set pagination off")
run(f"target remote 127.0.0.1:{port}")
mounts = gdb.parse_and_eval("mounts")
count = mounts.type.range()[1] + 1
state_type = gdb.lookup_type("struct ufs_mount_state").pointer()
for index in range(count):
	mount = mounts[index]
	data = int(mount["m_data"])
	path = mount["m_path"].string()
	if data == 0 or not path.startswith("/"):
		continue
	kind = mount["m_type"]
	if int(kind) == 0 or kind["fs_name"].string() != "ufs":
		continue
	try:
		state = mount["m_data"].cast(state_type).dereference()
		fields = state.type.fields()
	except gdb.error:
		continue
	for field in fields:
		if str(field.type) != "struct mutex":
			continue
		lock = state[field.name]
		owner = int(lock["owner"])
		if int(lock["locked"]) == 0 and owner == 0:
			continue
		print(f"MOUNT {path} {field.name} locked={int(lock['locked'])}")
		if owner != 0:
			stack(lock["owner"].dereference())
gdb.execute("detach")
