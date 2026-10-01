#!/bin/sh
# vkprotos.sh -- list Vulkan core prototypes from vulkan_core.h.
# usage: vkprotos.sh [header] [blocks]
#   blocks: space-separated guard names (default: VK_VERSION_1_0 .. VK_VERSION_1_3)
# output (tab-separated, one function per line):
#   block  name  return-type  parameter-list  parameter-names
set -eu
hdr=${1:-/usr/include/vulkan/vulkan_core.h}
blocks=${2:-"VK_VERSION_1_0 VK_VERSION_1_1 VK_VERSION_1_2 VK_VERSION_1_3"}
LC_ALL=C awk -v blocks="$blocks" '
BEGIN { n = split(blocks, b, " "); for (i = 1; i <= n; i++) want[b[i]] = 1 }
# every block (core version or extension) begins with "// NAME is a preprocessor guard."
/^\/\/ [A-Za-z0-9_]+ is a preprocessor guard\./ { cur = $2; inproto = 0; next }
/^#ifndef VK_NO_PROTOTYPES/ { inproto = 1; next }
/^#endif/ { inproto = 0; next }
!(cur in want) || !inproto { next }
/^VKAPI_ATTR / { acc = ""; collecting = 1 }
collecting {
	line = $0; sub(/^[ \t]+/, "", line); gsub(/[ \t]+/, " ", line)
	acc = acc (acc == "" || acc ~ /\($/ ? "" : " ") line
	if (acc !~ /\);$/) next
	collecting = 0
	# acc: VKAPI_ATTR <ret> VKAPI_CALL <name>(<params>);
	s = acc; sub(/^VKAPI_ATTR /, "", s)
	ret = s; sub(/ VKAPI_CALL .*/, "", ret)
	name = s; sub(/^.* VKAPI_CALL /, "", name); sub(/\(.*/, "", name)
	params = s; sub(/^[^(]*\(/, "", params); sub(/\);$/, "", params)
	names = ""
	np = split(params, p, ", ")
	for (i = 1; i <= np; i++) {
		w = p[i]; sub(/\[[0-9A-Z_]*\]$/, "", w)      # const float blendConstants[4]
		sub(/^.*[ *]/, "", w)
		names = names (i > 1 ? ", " : "") w
	}
	if (params == "void") names = ""
	print cur "\t" name "\t" ret "\t" params "\t" names
}
' "$hdr"
