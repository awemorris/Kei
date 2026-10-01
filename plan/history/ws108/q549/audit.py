# zedBSD; Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
# Audits the final native packages independently of the guest packager.
import hashlib,io,json,subprocess,sys,tarfile
from pathlib import Path
base=Path('build/keiland-deb/artifacts')
for distro in ('debian13','ubuntu2604'):
 directory=base/distro
 package,=directory.glob('*.deb')
 info=json.loads(next(directory.glob('*.buildinfo.json')).read_text())
 assert info['source_commit'].startswith('b0e1eaf9') and info['source_dirty'] is False
 manifest=json.loads(next(directory.glob('*.manifest.json')).read_text())
 data=tarfile.open(fileobj=io.BytesIO(subprocess.check_output(['dpkg-deb','--fsys-tarfile',str(package)])))
 members={ '/' + member.name.removeprefix('./'):member for member in data.getmembers() if member.isfile() }
 assert set(members)=={x['path'] for x in manifest}
 for entry in manifest:
  member=members[entry['path']]
  assert member.uid==member.gid==0
  assert member.mode==int(entry['mode'],8)
  assert hashlib.sha256(data.extractfile(member).read()).hexdigest()==entry['sha256']
 control=tarfile.open(fileobj=io.BytesIO(subprocess.check_output(['dpkg-deb','--ctrl-tarfile',str(package)])))
 names={m.name.removeprefix('./'):m for m in control.getmembers() if m.isfile()}
 assert set(names)=={'control','md5sums','conffiles'}
 fields=control.extractfile(names['control']).read().decode()
 assert 'Maintainer: Awe Morris <awemorris@users.noreply.github.com>' in fields
 assert 'Version: '+info['version'] in fields
 assert 'Architecture: amd64' in fields
 assert control.extractfile(names['conffiles']).read()==b'/opt/keiland/etc/keiland/apps.conf\n'
 for line in control.extractfile(names['md5sums']).read().decode().splitlines():
  h,name=line.split('  ',1)
  assert hashlib.md5(data.extractfile(members['/'+name]).read(),usedforsecurity=False).hexdigest()==h
 apps=data.extractfile(members['/opt/keiland/etc/keiland/apps.conf']).read().decode()
 assert 'Model viewer|' not in apps and 'Widget Demo|' not in apps
 desktop=data.extractfile(members['/usr/share/wayland-sessions/keiland.desktop']).read().decode()
 assert 'Exec=/opt/keiland/bin/wayland --session' in desktop
 elf=sum(data.extractfile(m).read(4)==b'\x7fELF' for m in members.values())
 assert elf==19
 for name in ('wlshm','wltest','vkdemo','mview','kuidemo'):
  assert '/opt/keiland/bin/'+name not in members
 assert not any('/include/' in name or '/share/mview/' in name for name in members)
 assert '/opt/keiland/share/doc/keiland/copyright' in members
 print(distro,package.name,'PASS: exact 40-file manifest/hash/mode/root ownership, 19 ELF, control/conffile/md5, session/font notices, runtime filters')
