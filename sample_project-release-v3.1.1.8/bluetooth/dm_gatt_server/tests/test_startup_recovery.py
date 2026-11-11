"""1x/2x/1x startup-recovery experiment: real stream, renderer and GPIO helpers.
Independent zlib CRC checks distinguish per-column duplication from frame repeat.
"""
from test_idle_ab import h,D,P,blob,Renderer
from pathlib import Path
import tempfile,subprocess,zlib
assert '#define PRINTER_STARTUP_RECOVERY 1' in (D/'printer_diagnostics.h').read_text()
assert 'APP_STARTUP_RECOVERY_V1' in (P/'ap/ap_main.c').read_text()
s=h.replace('#define PRINTER_IDLE_AB 1','#define PRINTER_IDLE_AB 1\n#define PRINTER_IDLE_REFERENCE_ONLY 1\n#define PRINTER_STARTUP_RECOVERY 1')
s=s.replace('scenario==4&&commands==8','scenario==4&&commands==4')
s=s.replace('static unsigned line_index;', '''static unsigned line_index,planned_repeats,line_sends;
static uint32_t line_wire_crc,job_wire_crc;
#define EXPECTED_X2 @X2@
#define EXPECTED_JOB @JOB@
''')
s=s.replace(' if(strstr(text,"[IDLE_AB_LINE_BEGIN]")){',''' if(strstr(text,"[RECOVERY_BEGIN]"))job_wire_crc=0xffffffffU;
 if(strstr(text,"[RECOVERY_RX_EXPECT]")){
  unsigned columns,bytes;unsigned long crc;
  assert(sscanf(text,"[RECOVERY_RX_EXPECT] columns=%u bytes=%u crc32=%lx",&columns,&bytes,&crc)==3);
  assert(columns==4720&&bytes==472000&&crc==EXPECTED_JOB);
 }
 if(strstr(text,"[RECOVERY_EXPECT_LINE]")){
  char stage[30];unsigned logical,fired,repeat;unsigned long crc;
  assert(sscanf(text,"[RECOVERY_EXPECT_LINE] stage=%29s logical_columns=%u fire_columns=%u wire_crc32=%lx repeats=%u",stage,&logical,&fired,&crc,&repeat)==5);
  planned_repeats=repeat;line_sends=0;line_wire_crc=0xffffffffU;
  assert(repeat==1||repeat==2);assert(logical==1180&&fired==1180*repeat);
  assert(crc==(repeat==1?printer_crc32(gold,118000):EXPECTED_X2));
 }
 if(strstr(text,"[RECOVERY_SENT_LINE]")){
  assert(line_sends==1180*planned_repeats);
  assert((line_wire_crc^0xffffffffU)==(planned_repeats==1?printer_crc32(gold,118000):EXPECTED_X2));
 }
 if(strstr(text,"[IDLE_AB_END] code=0"))assert((job_wire_crc^0xffffffffU)==EXPECTED_JOB);
 if(strstr(text,"[IDLE_AB_LINE_BEGIN]")){''')
a=s.index('static int tph_fire_raw_column_bytes(');b=s.index('static int motor_page_feed_keep_power(',a)
s=s[:a]+'''static int tph_fire_raw_column_bytes(const uint8_t*p){
 assert(head&&boost&&dir&&!step);assert(planned_repeats==1||planned_repeats==2);
 unsigned x=line_sends/planned_repeats;assert(x<1180);
 assert(!memcmp(p,gold+x*100,100));++sends;++line_sends;us+=200;
 line_wire_crc=printer_crc32_update(line_wire_crc,p,100);
 job_wire_crc=printer_crc32_update(job_wire_crc,p,100);
 if(x==active_first)first_active[line_index]=us;
 if(x==active_last)last_active[line_index]=us;
 if((scenario==1&&sends==11)||(scenario==2&&job_lines==2&&line_sends==12))return -1;
 return 0;
}
'''+s[b:]
s=s.replace('us+=1800000;*sent=', 'if(scenario==6&&feeds==2){assert(frozen_ptr);frozen_ptr[9]^=1;}\n us+=1800000;*sent=')
a=s.index('static void check_run(');b=s.index('int main(',a)
s=s[:a]+'''static void check_run(unsigned offset,bool unused){
 (void)unused;
 assert(phase[offset]=='S'&&kind[offset]=='B');
 assert(phase[offset+1]=='S'&&kind[offset+1]=='D');
 assert(phase[offset+2]=='S'&&kind[offset+2]=='A');
 assert(us<60000000ULL);
}
'''+s[b:]
s=s.replace('scans==12&&feeds==12&&sends==12*1180','scans==3&&feeds==3&&sends==4*1180')
s=s.replace('ons==3&&offs==5&&gpio_inits==2&&boost_ons==2&&boost_offs==2','ons==1&&offs==3&&gpio_inits==1&&boost_ons==1&&boost_offs==1')
s=s.replace('scans==24&&feeds==24&&sends==24*1180','scans==6&&feeds==6&&sends==8*1180')
s=s.replace('check_run(12,true);assert(ons==6&&gpio_inits==4&&boost_ons==4&&boost_offs==4);','check_run(3,false);assert(ons==2&&gpio_inits==2&&boost_ons==2&&boost_offs==2);')
s=s.replace('scenario==2)assert(scans==10&&legs==19&&feeds==9)','scenario==2)assert(scans==2&&legs==3&&feeds==1)')
s=s.replace('scenario==4)assert(scans==12&&feeds==12)','scenario==4)assert(scans==3&&feeds==3)')
s=s.replace('scenario==6)assert(scans==9&&feeds==9)','scenario==6)assert(scans==2&&feeds==2)')
with tempfile.TemporaryDirectory() as tmp:
 t=Path(tmp)
 data=blob({(ch,x,y) for ch in range(3) for x in range(100,1000) for y in range(1225,1237)})
 (t/'input.bin').write_bytes(data);(t/'blank.bin').write_bytes(blob(set()))
 renderer=Renderer();rc,page=renderer.prepare(data);assert not rc;gold,_=renderer.render(page,4);renderer.close(page);(t/'gold.bin').write_bytes(gold)
 (t/'alt.bin').write_bytes(blob({(ch,x,y) for ch in range(3) for x in range(150,1050) for y in range(1340,1352)}))
 twice=b''.join(gold[i:i+100]*2 for i in range(0,len(gold),100))
 assert twice!=gold+gold
 code=s.replace('@X2@',hex(zlib.crc32(twice))+'U').replace('@JOB@',hex(zlib.crc32(gold+twice+gold))+'U')
 (t/'test.c').write_text(code)
 subprocess.run(['cc','-std=c99','-O1','-Wall','-Wextra','-Werror','-I'+str(D),str(t/'test.c'),str(D/'printer_page.c'),str(D/'printer_mapper.c'),'-o',str(t/'test')],check=True)
 arm=Path('/workspace/.setup/tools/usr/bin/arm-none-eabi-gcc')
 if arm.exists():subprocess.run([str(arm),'-std=gnu99','-mcpu=cortex-m33+nodsp','-mfpu=fpv5-sp-d16','-mfloat-abi=hard','-mcmse','-O2','-Wall','-Wextra','-Werror','-I'+str(D),'-c',str(t/'test.c'),'-o',str(t/'test.o')],check=True)
 for scenario in [0,1,2,3,4,5,6,8,12]:
  subprocess.run([str(t/'test'),str(t/'input.bin'),str(t/'blank.bin'),str(t/'gold.bin'),str(t/'alt.bin'),str(scenario)],check=True)
print('PASS: actual per-column 1/2/1 order and 4720 ACKs, independent zlib wire/aggregate CRCs, three scans/one ON, no idle/extra motion, failure cleanup, cache freeze and ARM compilation')
