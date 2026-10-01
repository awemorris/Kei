#!/bin/sh
# gen-forward.sh TSV > forward.inc : F forwarders + table fill
set -eu
awk -F'\t' '
{ n[NR]=$2; r[NR]=$3; p[NR]=$4; a[NR]=$5 }
END {
	for (i = 1; i <= NR; i++) print "static PFN_" n[i] " next_" n[i] ";"
	for (i = 1; i <= NR; i++) {
		print "VKAPI_ATTR " r[i] " VKAPI_CALL " n[i] "(" p[i] ")"
		print "{"
		print "\tif (!next_" n[i] ") missing(\"" n[i] "\");"
		print "\t" (r[i] == "void" ? "" : "return ") "next_" n[i] "(" a[i] ");"
		print "}"
	}
	print "static void fill_table(void *h)"
	print "{"
	for (i = 1; i <= NR; i++) print "\tnext_" n[i] " = (PFN_" n[i] ")dlsym(h, \"" n[i] "\");"
	print "}"
}' "$1"
