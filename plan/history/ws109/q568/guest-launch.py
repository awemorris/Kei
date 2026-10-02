# zedBSD; Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
"""Start only the prepared WS109 native FreeBSD PCI passthrough fixture."""
from pathlib import Path
import subprocess,json,os,socket
D=Path('/home/awe/ws109-freebsd-fixture')
assert os.geteuid()==0
assert (D/'native.qcow2').is_file()
with socket.socket() as s:s.bind(('127.0.0.1',47969))
assert not (D/'qmp.sock').exists()
assert Path('/sys/bus/pci/devices/0000:00:02.0/driver').resolve().name=='vfio-pci'
args=['qemu-system-x86_64','-name','ws109-freebsd-passthrough','-machine','q35','-accel','kvm','-cpu','host,host-phys-bits-limit=39','-m','4096','-smp','4','-drive','file='+str(D/'native.qcow2')+',format=qcow2,if=none,id=ws109disk','-device','virtio-blk-pci,drive=ws109disk,bus=pcie.0,addr=0x4','-vga','none','-device','VGA,id=ws109console,bus=pcie.0,addr=0x1','-device','vfio-pci,host=0000:00:02.0,id=ws109igpu,bus=pcie.0,addr=0x2,rombar=0,x-igd-opregion=on','-device','qemu-xhci,id=xhci,bus=pcie.0,addr=0x5','-device','usb-tablet,bus=xhci.0','-device','usb-kbd,bus=xhci.0','-netdev','user,id=net0,hostfwd=tcp:127.0.0.1:47969-:22','-device','virtio-net-pci,netdev=net0,bus=pcie.0,addr=0x6','-device','intel-hda,bus=pcie.0,addr=0x7','-device','hda-duplex,audiodev=none0','-audiodev','none,id=none0','-display','none','-serial','null','-monitor','none','-qmp','unix:'+str(D/'qmp.sock')+',server=on,wait=off']
p=subprocess.Popen(args,stdin=subprocess.DEVNULL,stdout=subprocess.DEVNULL,stderr=(D/'qemu-stderr.txt').open('w'),start_new_session=True)
(D/'process.json').write_text(json.dumps({'pid':p.pid,'args':args,'owner':'WS109 dedicated fixture'},indent=2)+'\n')
print('WS109 own remote guest launched',p.pid)
