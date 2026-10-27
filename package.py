#!/usr/bin/env python3
import io,tarfile,pathlib
root=pathlib.Path(__file__).resolve().parent
exe=root/'f2_recover_sqlite_arm'
if not exe.is_file():raise SystemExit('Missing ARM executable')
staticexe=root/'f2_recover_sqlite_static_arm'
if not staticexe.is_file():raise SystemExit('Missing static ARM executable')
head=exe.read_bytes()[:20]
if head[:4]!=b'\x7fELF' or head[4]!=1 or int.from_bytes(head[18:20],'little')!=40:raise SystemExit('Not ARM32 ELF')
control=b'Package: f2_service_diagnostic\nVersion: 1.0.6\nArchitecture: armv7l\nMaintainer: service\nDescription: F2 read-only diagnostic SQLite WAL\n'
postinst="#!/bin/sh\nUSB=/mnt/nand_disk\nLOG=\"$USB/F2_diagnostic_status.txt\"\nSYS=\"$USB/F2_machine_info.tsv\"\nif [ ! -d \"$USB\" ]; then exit 1; fi\nrm -f \"$USB/F2_passwords.txt\" \"$USB/F2_settings.tsv\"\nprintf 'Section\\tParameter\\tValue\\tSource\\n' > \"$SYS\"\nrow() {\n  SECTION=\"$1\"; KEY=\"$2\"; VALUE=\"$3\"; SOURCE=\"$4\"\n  VALUE=$(printf '%s' \"$VALUE\" | tr '\\t\\r\\n' '   ' | cut -c1-500)\n  printf '%s\\t%s\\t%s\\t%s\\n' \"$SECTION\" \"$KEY\" \"$VALUE\" \"$SOURCE\" >> \"$SYS\"\n}\nrow system kernel \"$(uname -r 2>/dev/null)\" uname\nrow system architecture \"$(uname -m 2>/dev/null)\" uname\nrow system kernel_full \"$(uname -a 2>/dev/null)\" uname\nrow system hostname \"$(hostname 2>/dev/null)\" hostname\nrow system uptime \"$(cat /proc/uptime 2>/dev/null)\" /proc/uptime\nrow system cpu_model \"$(grep -m1 -E 'Hardware|model name|Processor' /proc/cpuinfo 2>/dev/null)\" /proc/cpuinfo\nrow system memory \"$(grep MemTotal /proc/meminfo 2>/dev/null)\" /proc/meminfo\nfor x in /etc/os-release /etc/issue /etc/version /etc/buildroot-release /etc/firmware_version; do\n if [ -f \"$x\" ]; then row firmware \"$x\" \"$(head -c 500 \"$x\" 2>/dev/null)\" \"$x\"; fi\ndone\nfor x in /root/Hmi /opt/updateAssistant /usr/local/bin/f2_recover_sqlite_arm /usr/local/bin/f2_recover_sqlite_static_arm; do\n if [ -e \"$x\" ]; then\n  row software \"$x\" \"$(ls -l \"$x\" 2>/dev/null)\" ls\n else\n  row software \"$x\" absent filesystem\n fi\ndone\nfor x in /lib/ld-linux-armhf.so.3 /lib/ld-uClibc.so.0 /lib/libc.so.6 /lib/libsqlite3.so.0; do\n if [ -e \"$x\" ]; then row runtime \"$x\" \"$(ls -l \"$x\" 2>/dev/null)\" ls; else row runtime \"$x\" absent filesystem; fi\ndone\nrow storage root \"$(df -k / 2>/dev/null | tail -n 1)\" df\nrow storage usb \"$(df -k \"$USB\" 2>/dev/null | tail -n 1)\" df\nrow database machine_db \"$(ls -l /root/machine.db 2>/dev/null)\" ls\nif [ -f /root/machine.db ]; then\n row database journal_header \"$(od -An -tu1 -j 18 -N 2 /root/machine.db 2>/dev/null)\" /root/machine.db\nfi\nrow software package_version 1.0.6 ipk\nif [ -e /lib/ld-linux-armhf.so.3 ]; then\n EXE=/usr/local/bin/f2_recover_sqlite_arm\nelse\n EXE=/usr/local/bin/f2_recover_sqlite_static_arm\nfi\necho \"F2_PACKAGE_VERSION=1.0.6\" > \"$LOG\"\necho \"SELECTED_EXECUTABLE=$EXE\" >> \"$LOG\"\n\"$EXE\" /root/machine.db \"$USB\" >> \"$LOG\" 2>&1\nRC=$?\necho \"SYSTEM_REPORT=$SYS\" >> \"$LOG\"\nsync\nexit \"$RC\"\n".encode()

def archive(entries):
 b=io.BytesIO()
 with tarfile.open(fileobj=b,mode='w:gz',format=tarfile.GNU_FORMAT) as tar:
  for name,data,mode in entries:
   t=tarfile.TarInfo(name);t.size=len(data);t.mode=mode;t.uid=0;t.gid=0;tar.addfile(t,io.BytesIO(data))
 return b.getvalue()
ct=archive([('./control',control,0o644),('./postinst',postinst,0o755)])
dt=archive([('./usr/local/bin/f2_recover_sqlite_arm',exe.read_bytes(),0o755),('./usr/local/bin/f2_recover_sqlite_static_arm',staticexe.read_bytes(),0o755)])
out=root/'f2_service_diagnostic_1.0.6.drcoffee.ipk'
with tarfile.open(str(out),'w',format=tarfile.GNU_FORMAT) as tar:
 for name,data in [('./debian-binary',b'2.0\n'),('./control.tar.gz',ct),('./data.tar.gz',dt)]:
  t=tarfile.TarInfo(name);t.size=len(data);t.mode=0o644;tar.addfile(t,io.BytesIO(data))
print(out)
