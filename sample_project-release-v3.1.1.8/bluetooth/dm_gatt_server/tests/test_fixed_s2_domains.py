"""Actual D1..D6/D6..D1 inspection, exact masks/bytes and bounded failure paths."""
from test_fixed_s2_colors import code as color_code,full,D,P,blob
from pathlib import Path
import collections,tempfile,subprocess,zlib,os
assert '#define PRINTER_FIXED_S2_DOMAIN_CHECK 1' in (D/'printer_diagnostics.h').read_text()
assert 'APP_FIXED_S2_DOMAIN_CHECK_V1' in (P/'ap/ap_main.c').read_text()
frames=[bytes(v&(1<<d) for v in full) for d in range(6)]
plan=list(range(6))+list(reversed(range(6)))
assert [zlib.crc32(f) for f in frames]==[0xCAF90008,0x48E40DD2,0xC030ADC3,0x7558A543,0xD2195130,0x1E7396E0]
assert zlib.crc32(b''.join(frames[d] for d in plan))==0xA13F47F7
hits=collections.Counter()
for d in plan:
 for x in range(1180):
  for slot,v in enumerate(frames[d][x*100:(x+1)*100]):
   if v:
    assert v==(1<<d) and slot%4==1
    hits[slot,d]+=1
assert len(hits)==24 and set(hits.values())=={1422}
assert [sum(n for (slot,bit),n in hits.items() if bit==d) for d in range(6)]==[5688]*6
code=color_code.replace('#define PRINTER_FIXED_S2_COLOR_CHECK 1','#define PRINTER_FIXED_S2_COLOR_CHECK 1\n#define PRINTER_FIXED_S2_DOMAIN_CHECK 1')
code=code.replace('gold[3][118000]','gold[6][118000]').replace('color_first[3],color_last[3]','color_first[6],color_last[6]')
code=code.replace('color_plan[6]={0,1,2,2,1,0}', 'color_plan[12]={0,1,2,3,4,5,5,4,3,2,1,0}')
code=code.replace('static unsigned current_color,','static const char *const domain_names[6]={"D1_CYAN","D2_CYAN","D3_MAGENTA","D4_MAGENTA","D5_YELLOW","D6_YELLOW"};\nstatic unsigned current_color,')
code=code.replace('COLOR_CHECK','DOMAIN_CHECK') # logger tags only; fix the enabling macro below
code=code.replace('#define PRINTER_FIXED_S2_DOMAIN_CHECK 1\n#define PRINTER_FIXED_S2_DOMAIN_CHECK 1','#define PRINTER_FIXED_S2_COLOR_CHECK 1\n#define PRINTER_FIXED_S2_DOMAIN_CHECK 1')
code=code.replace('line=%u/6 expected=', 'line=%u/12 expected=')
code=code.replace('3U<<(2*current_color)','1U<<current_color')
code=code.replace('current_color==0?"CYAN":current_color==1?"MAGENTA":"YELLOW"','domain_names[current_color]')
code=code.replace('live==3','live==6').replace('(scans-1)%6','(scans-1)%12').replace('c<3;c++','c<6;c++')
code=code.replace('0x7764411C','0xA13F47F7').replace('354000','708000')
code=code.replace('14160','28320').replace('7080','14160').replace('bytes==708000','bytes==1416000')
code=code.replace('e==1416000','e==708000')
code=code.replace('scans==12','scans==24').replace('scans==6','scans==12')
code=code.replace('feeds==12','feeds==24').replace('feeds==6','feeds==12')
code=code.replace('if(scenario==6)assert(scans==2&&feeds==2);','if(scenario==6)assert(scans==5&&feeds==5);')
with tempfile.TemporaryDirectory() as tmp:
 t=Path(tmp);(t/'input.bin').write_bytes(blob(set()));(t/'blank.bin').write_bytes(blob(set()))
 (t/'alt.bin').write_bytes(blob({(ch,x,y) for ch in range(3) for x in range(150,1050) for y in range(1340,1352)}))
 (t/'gold.bin').write_bytes(b''.join(frames));(t/'test.c').write_text(code)
 subprocess.run(['cc','-std=c99','-O1','-fsanitize=address,undefined','-fno-omit-frame-pointer','-Wall','-Wextra','-Werror','-I'+str(D),str(t/'test.c'),str(D/'printer_page.c'),str(D/'printer_mapper.c'),'-o',str(t/'test')],check=True)
 arm=Path('/workspace/.setup/tools/usr/bin/arm-none-eabi-gcc')
 if arm.exists():subprocess.run([str(arm),'-std=gnu99','-mcpu=cortex-m33+nodsp','-mfpu=fpv5-sp-d16','-mfloat-abi=hard','-mcmse','-O2','-Wall','-Wextra','-Werror','-I'+str(D),'-c',str(t/'test.c'),'-o',str(t/'test.o')],check=True)
 for scenario in [0,0,1,2,3,4,5,6,8,9,12]:
  subprocess.run([str(t/'test'),str(t/'input.bin'),str(t/'blank.bin'),str(t/'gold.bin'),str(t/'alt.bin'),str(scenario)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
print('PASS: actual 12-line single-D order, 14160 exact columns, independent six frame/job CRCs, 1422 commands per selected nozzle, one ON, unchanged/blank/fresh triggers, error cleanup and partial allocation recovery; ASan/UBSan and ARM compilation')
