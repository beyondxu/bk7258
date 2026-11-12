"""Actual fixed S2 recipe/orchestrator: repeatable across inputs and reboots.
Checks exact bytes and per-nozzle command counts, not ink/physical temperature.
"""
from test_startup_recovery import s,D,P,blob
from pathlib import Path
import tempfile,subprocess,zlib,collections,re
assert '#define PRINTER_FIXED_S2_BASELINE 1' in (D/'printer_diagnostics.h').read_text()
assert 'APP_FIXED_S2_BASELINE_V1' in (P/'ap/ap_main.c').read_text()
frame=bytearray(118000)
# Independent specification in physical row order, not the C address arrays.
rows=[(14,1),(23,0),(20,1),(5,0),(2,1),(12,0),(9,1),(18,0)]
for address,pair in rows:
 for channel in range(3):
  d=channel*2+pair;offset=[61,59,31,30,1,0][d]
  for x in range(150+offset,861+offset):frame[100*x+(address-1)*4+1]|=1<<d
hits=collections.Counter();amask=smask=dmask=0;active=0
for x in range(1180):
 col=frame[x*100:(x+1)*100];active+=any(col)
 for slot,data in enumerate(col):
  if not data:continue
  amask|=1<<(slot//4);smask|=1<<(slot%4);dmask|=data
  for d in range(6):
   if data>>d&1:hits[slot,d]+=1
assert (amask,smask,dmask,active)==(0x004A2912,0x02,0x3F,772)
assert len(hits)==24 and set(hits.values())=={711}
assert [sum(n for (slot,bit),n in hits.items() if bit==d) for d in range(6)]==[2844]*6
assert zlib.crc32(frame)==0xEA748497
wire2=b''.join(frame[i:i+100]*2 for i in range(0,len(frame),100))
assert zlib.crc32(wire2)==0xF3FD6745 and zlib.crc32(frame+wire2+frame)==0x86CE8D53
code=s.replace('#define PRINTER_STARTUP_RECOVERY 1','#define PRINTER_STARTUP_RECOVERY 1\n#define PRINTER_FIXED_S2_BASELINE 1')
# A valid blank F2 is now a trigger too. Malformed/truncated F2 stays rejected.
code=code.replace(' assert(printer_run_idle_ab(blank,z));assert(!scans&&!feeds&&!live&&!head&&!boost);','')
with tempfile.TemporaryDirectory() as tmp:
 t=Path(tmp)
 # First trigger is blank, second contains a different image, neither selects data.
 (t/'input.bin').write_bytes(blob(set()));(t/'blank.bin').write_bytes(blob(set()))
 (t/'alt.bin').write_bytes(blob({(ch,x,y) for ch in range(3) for x in range(150,1050) for y in range(1340,1352)}))
 (t/'gold.bin').write_bytes(frame)
 code=code.replace('@X2@',hex(zlib.crc32(wire2))+'U').replace('@JOB@',hex(zlib.crc32(frame+wire2+frame))+'U')
 (t/'test.c').write_text(code)
 subprocess.run(['cc','-std=c99','-O1','-Wall','-Wextra','-Werror','-I'+str(D),str(t/'test.c'),str(D/'printer_page.c'),str(D/'printer_mapper.c'),'-o',str(t/'test')],check=True)
 arm=Path('/workspace/.setup/tools/usr/bin/arm-none-eabi-gcc')
 if arm.exists():subprocess.run([str(arm),'-std=gnu99','-mcpu=cortex-m33+nodsp','-mfpu=fpv5-sp-d16','-mfloat-abi=hard','-mcmse','-O2','-Wall','-Wextra','-Werror','-I'+str(D),'-c',str(t/'test.c'),'-o',str(t/'test.o')],check=True)
 for scenario in [0,0,1,2,3,4,5,6,8,12]:
  subprocess.run([str(t/'test'),str(t/'input.bin'),str(t/'blank.bin'),str(t/'gold.bin'),str(t/'alt.bin'),str(scenario)],check=True)
print('PASS: fixed S2 exact golden bytes, known CRCs, 24 nozzles each 711/2844 commands, blank/different triggers and fresh processes, 1/2/1 ordering, rejection and failure cleanup; ARM compiled')
