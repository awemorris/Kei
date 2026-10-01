#!/usr/bin/env python3
# ws075-p001: the Vulkan commands each client calls that the i915 executor does not take (static).  A client's calls are
# the vk*( names in its C sources; libvulkan's opcode table (userland/desktop/libvulkan/opcodes.h) numbers them; the
# executor's commands are the case labels of its opcode switches (src/drivers/gpu/i915/render/*.c: switch (opcode)),
# less those whose body refuses (EOPNOTSUPP, ENOTSUP or "unported").  A call with no opcode stays in libvulkan (WSI,
# queries of the loader) and is not counted.  Creation-time parameters (formats, samples, pipeline stages) are not seen.
#
#   plan/ws075/tests/vk-calls.py [ROOT]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
import glob
import os
import re
import sys

# Commands libvulkan answers without sending them (vkWaitForFences polls the fences' status in sync.c; mapping, the
# queue and the extensions are the library's own): the clients that call them run on the executor (ws075-p001 capture).
LOCAL = {'vkWaitForFences', 'vkMapMemory', 'vkUnmapMemory', 'vkFlushMappedMemoryRanges', 'vkInvalidateMappedMemoryRanges',
         'vkGetDeviceQueue', 'vkEnumerateDeviceExtensionProperties'}

CLIENTS = ['userland/desktop/wayland', 'userland/desktop/files', 'userland/desktop/terminal', 'userland/tests/mview',
           'userland/tests/vkdemo', 'userland/tests/wltest', 'userland/desktop/xserver', 'userland/desktop/libegl',
           'userland/desktop/libglesv2']


def opcodes(root):
	"""Returns libvulkan's opcode of each command name."""
	table = {}
	text = open(os.path.join(root, 'userland/desktop/libvulkan/opcodes.h'), encoding='utf-8').read()
	for name, number in re.findall(r'VULKAN_OPCODE_(vk\w+)\s*=\s*(\d+)', text):
		table[name] = int(number)
	return table


def executor(root):
	"""Returns the opcodes the executor's switches take, and those whose case refuses."""
	taken = set()
	refused = set()
	for path in glob.glob(os.path.join(root, 'src/drivers/gpu/i915/render/*.c')):
		text = open(path, encoding='utf-8').read()
		names = {name: number for name, number in re.findall(r'#define\s+(\w+)\s+(\d+)U', text)}
		lines = text.split('\n')
		index = 0
		while index < len(lines):
			if 'switch (opcode) {' not in lines[index]:
				index += 1
				continue

			# The switch's cases up to its closing brace at the switch's indentation.
			indent = lines[index][:len(lines[index]) - len(lines[index].lstrip())]
			index += 1
			labels = []
			body = ''
			while index < len(lines) and lines[index] != indent + '}':
				line = lines[index]
				match = re.match(r'\s*case (\w+):', line)
				if match and match.group(1) in names:
					match = re.match(r'(\s*)case (\w+):', '\tcase %sU:' % names[match.group(1)])
				match = re.match(r'\s*case (\d+)U:', match.group(0)) if match else None
				if match:
					if body and labels:
						if re.search(r'EOPNOTSUPP|ENOTSUP|unported', body):
							refused.update(labels)
						labels = []
						body = ''
					labels.append(int(match.group(1)))
					taken.add(int(match.group(1)))
				elif re.match(r'\s*default:', line):
					if body and labels and re.search(r'EOPNOTSUPP|ENOTSUP|unported', body):
						refused.update(labels)
					labels = []
					body = ''
				else:
					body += line + '\n'
				index += 1
			if body and labels and re.search(r'EOPNOTSUPP|ENOTSUP|unported', body):
				refused.update(labels)
	return taken - refused, refused


def calls(root, client):
	"""Returns the vk* names a client's C sources call."""
	names = set()
	for path in glob.glob(os.path.join(root, client, '**/*.c'), recursive=True):
		names.update(re.findall(r'\b(vk[A-Z]\w*)\s*\(', open(path, encoding='utf-8', errors='replace').read()))
	return names


def main():
	root = sys.argv[1] if len(sys.argv) > 1 else '.'
	table = opcodes(root)
	taken, refused = executor(root)

	# The recording range: command.c records every vkCmd* opcode it knows in i915_record_command.
	recorded = set()
	text = open(os.path.join(root, 'src/drivers/gpu/i915/render/command.c'), encoding='utf-8').read()
	match = re.search(r'\ni915_record_command\((.*?)\n}\n', text, re.S)
	if match:
		recorded = {int(n) for n in re.findall(r'case (\d+)U:', match.group(1))}
	taken |= recorded
	print('executor: %d opcodes taken, %d refused in their case' % (len(taken), len(refused)))
	for client in CLIENTS:
		names = calls(root, client)
		missing = sorted('%s (%d)' % (name, table[name]) for name in names
		                 if name in table and name not in LOCAL and table[name] not in taken)
		print('%s: %d commands; not taken: %s' % (client, len([n for n in names if n in table]), ', '.join(missing) or 'none'))


if __name__ == '__main__':
	main()
