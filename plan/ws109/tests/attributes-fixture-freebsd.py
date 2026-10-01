# zedBSD; Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
"""Prepare only owned UFS files and exercise actual root/unprivileged attribute permissions."""
from pathlib import Path
import os,subprocess,sys,tempfile
program=sys.argv[1]
with tempfile.TemporaryDirectory(prefix='ws109-attributes-',dir='/tmp') as directory:
 os.chmod(directory,0o755)
 source=Path(directory,'source');source.write_bytes(b'owned source');source.chmod(0o666)
 destination=Path(directory,'destination');destination.write_bytes(b'owned destination');destination.chmod(0o666)
 link=Path(directory,'link');link.symlink_to(source)
 args=[program,'attributes',str(source),str(destination),str(link)]
 subprocess.run(args,check=True,timeout=10)
 def unprivileged():
  os.setgroups([]);os.setgid(65534);os.setuid(65534)
 args[1]='permission'
 subprocess.run(args,preexec_fn=unprivileged,cwd='/tmp',check=True,timeout=10)
print('owned native attribute source/destination/link removed',flush=True)
