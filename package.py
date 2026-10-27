#!/usr/bin/env python3
import io,tarfile,pathlib
root=pathlib.Path(__file__).resolve().parent
exe=root/'f2_recover_sqlite_arm'
if not exe.is_file():raise SystemExit('Missing ARM executable')
staticexe=root/'f2_recover_sqlite_static_arm'
if not staticexe.is_file():raise SystemExit('Missing static ARM executable')
head=exe.read_bytes()[:20]
if head[:4]!=b'\x7fELF' or head[4]!=1 or int.from_bytes(head[18:20],'little')!=40:raise SystemExit('Not ARM32 ELF')
control=b'Package: f2_service_diagnostic\nVersion: 1.0.5\nArchitecture: armv7l\nMaintainer: service\nDescription: F2 read-only diagnostic SQLite WAL\n'
postinst=b'#!/bin/sh\nUSB=/mnt/nand_disk\nLOG="$USB/F2_diagnostic_status.txt"\nif [ ! -d "$USB" ]; then exit 1; fi\nrm -f "$USB/F2_passwords.txt"\n/usr/local/bin/f2_recover_sqlite_arm /root/machine.db "$USB" > "$LOG" 2>&1\nRC=$?\nsync\nexit "$RC"\n'
def archive(entries):
 b=io.BytesIO()
 with tarfile.open(fileobj=b,mode='w:gz',format=tarfile.GNU_FORMAT) as tar:
  for name,data,mode in entries:
   t=tarfile.TarInfo(name);t.size=len(data);t.mode=mode;t.uid=0;t.gid=0;tar.addfile(t,io.BytesIO(data))
 return b.getvalue()
ct=archive([('./control',control,0o644),('./postinst',postinst,0o755)])
dt=archive([('./usr/local/bin/f2_recover_sqlite_arm',exe.read_bytes(),0o755),('./usr/local/bin/f2_recover_sqlite_static_arm',staticexe.read_bytes(),0o755)])
out=root/'f2_service_diagnostic_1.0.5.drcoffee.ipk'
with tarfile.open(str(out),'w',format=tarfile.GNU_FORMAT) as tar:
 for name,data in [('./debian-binary',b'2.0\n'),('./control.tar.gz',ct),('./data.tar.gz',dt)]:
  t=tarfile.TarInfo(name);t.size=len(data);t.mode=0o644;tar.addfile(t,io.BytesIO(data))
print(out)
