"""Actual fixed S2 C/M/Y/Y/M/C controller, exact frames and failure paths."""
from test_startup_recovery import s,D,P,blob
from pathlib import Path
import tempfile,subprocess,zlib,collections,os
assert '#define PRINTER_FIXED_S2_COLOR_CHECK 1' in (D/'printer_diagnostics.h').read_text()
assert 'APP_FIXED_S2_COLOR_CHECK_V1' in (P/'ap/ap_main.c').read_text()
full=bytearray(118000)
for address,pair in [(14,1),(23,0),(20,1),(5,0),(2,1),(12,0),(9,1),(18,0)]:
 for ch in range(3):
  d=ch*2+pair;offset=[61,59,31,30,1,0][d]
  for x in range(150+offset,861+offset):full[x*100+(address-1)*4+1]|=1<<d
frames=[bytes(v&mask for v in full) for mask in [3,12,48]]
plan=[0,1,2,2,1,0]
assert [zlib.crc32(f) for f in frames]==[0x93764BC7,0xA4034E9D,0xDD0181CD]
assert zlib.crc32(b''.join(frames[c] for c in plan))==0x7764411C
hits=collections.Counter()
for color in plan:
 for x in range(1180):
  for slot,v in enumerate(frames[color][x*100:(x+1)*100]):
   for d in range(6):
    if v>>d&1:hits[slot,d]+=1
assert len(hits)==24 and set(hits.values())=={1422}
assert [sum(n for (slot,d),n in hits.items() if d==b) for b in range(6)]==[5688]*6
code=s.replace('#define PRINTER_STARTUP_RECOVERY 1','#define PRINTER_STARTUP_RECOVERY 1\n#define PRINTER_FIXED_S2_BASELINE 1\n#define PRINTER_FIXED_S2_COLOR_CHECK 1')
code=code.replace('gold[118000]','gold[3][118000]')
code=code.replace('#define EXPECTED_X2 @X2@','#define EXPECTED_X2 0U').replace('#define EXPECTED_JOB @JOB@','#define EXPECTED_JOB 0x7764411CU\nstatic const unsigned color_plan[6]={0,1,2,2,1,0};\nstatic unsigned current_color,color_first[3],color_last[3];')
code=code.replace('static unsigned live,oom,','static unsigned frame_allocations;static unsigned live,oom,')
code=code.replace('return NULL;void*p=malloc(n);','return NULL;if(n==118000&&scenario==9&&++frame_allocations==2)return NULL;void*p=malloc(n);')
code=code.replace('scenario==4&&commands==4','scenario==4&&commands==3')
code=code.replace('static void printer_map_yield_cpu(void){}', 'static void printer_map_yield_cpu(void){if(scenario==8&&!corrupted&&frozen_ptr){frozen_ptr[9]^=1;corrupted=1;}}')
a=code.index('static void log_message(');b=code.index('#define os_printf log_message',a)
code=code[:a]+r'''
static void log_message(const char*format,...){
 char text[700];va_list ap;va_start(ap,format);vsnprintf(text,sizeof(text),format,ap);va_end(ap);
 if(strstr(text,"[COLOR_CHECK_BEGIN]"))job_wire_crc=0xffffffffU;
 if(strstr(text,"[COLOR_CHECK_RX_EXPECT]")){
  unsigned columns,bytes;unsigned long crc;
  assert(sscanf(text,"[COLOR_CHECK_RX_EXPECT] columns=%u bytes=%u crc32=%lx",&columns,&bytes,&crc)==3);
  assert(columns==7080&&bytes==708000&&crc==0x7764411CU);
 }
 if(strstr(text,"[COLOR_CHECK_LINE]")){
  unsigned line,mask;unsigned long crc;char color[20];
  assert(sscanf(text,"[COLOR_CHECK_LINE] line=%u/6 expected=%19s D_mask=%x frame_crc32=%lx",&line,color,&mask,&crc)==4);
  current_color=color_plan[line-1];assert(line==job_lines+1);
  assert(mask==(3U<<(2*current_color))&&crc==printer_crc32(gold[current_color],118000));
  assert(!strcmp(color,current_color==0?"CYAN":current_color==1?"MAGENTA":"YELLOW"));
  line_sends=0;line_wire_crc=0xffffffffU;
 }
 if(strstr(text,"[IDLE_AB_LINE_BEGIN]")){
  line_index=scans;assert(head&&boost&&live==3);current_phase='C';
 }
 if(strstr(text,"[RECOVERY_SENT_LINE]")){
  assert(line_sends==1180);
  assert((line_wire_crc^0xffffffffU)==printer_crc32(gold[current_color],118000));
 }
 if(strstr(text,"[COLOR_CHECK_END] code=0"))assert((job_wire_crc^0xffffffffU)==0x7764411CU);
}
'''+code[b:]
a=code.index('static int tph_fire_raw_column_bytes(');b=code.index('static int motor_page_feed_keep_power(',a)
code=code[:a]+r'''
static int tph_fire_raw_column_bytes(const uint8_t*p){
 assert(head&&boost&&dir&&!step);assert(s_recovery_repeats==1);
 unsigned x=line_sends;assert(x<1180&&current_color==color_plan[(scans-1)%6]);
 assert(!memcmp(p,gold[current_color]+x*100,100));++sends;++line_sends;us+=200;
 line_wire_crc=printer_crc32_update(line_wire_crc,p,100);
 job_wire_crc=printer_crc32_update(job_wire_crc,p,100);
 if(x==color_first[current_color])first_active[line_index]=us;
 if(x==color_last[current_color])last_active[line_index]=us;
 if((scenario==1&&sends==11)||(scenario==2&&job_lines==2&&line_sends==12))return -1;
 return 0;
}
'''+code[b:]
# This stub precedes the real stream globals: check repeats inside log callback instead.
code=code.replace('assert(s_recovery_repeats==1);','')
a=code.index('static void check_run(')
code=code[:a]+r'''
int main(int argc,char**argv){
 assert(argc==6);scenario=atoi(argv[5]);uint32_t n,z,e,alt_n;
 uint8_t*b=readfile(argv[1],&n),*blank=readfile(argv[2],&z),*expected=readfile(argv[3],&e),*alt=readfile(argv[4],&alt_n);
 assert(e==354000);memcpy(gold,expected,e);free(expected);
 for(unsigned c=0;c<3;c++){color_first[c]=1180;for(unsigned x=0;x<1180;x++){unsigned any=0;for(unsigned i=0;i<100;i++)any|=gold[c][x*100+i];if(any){if(color_first[c]==1180)color_first[c]=x;color_last[c]=x;}}}
 assert(printer_run_idle_ab(b,n-1));assert(!scans&&!feeds&&!live&&!head&&!boost);
 oom=1;assert(printer_run_idle_ab(b,n));assert(!scans&&!feeds&&!live&&!head&&!boost);oom=0;frozen_ptr=NULL;
 link=0;assert(printer_run_idle_ab(b,n));assert(!scans&&!feeds&&!live&&!head&&!boost);link=1;
 int rc=printer_run_idle_ab(b,n);assert(!live&&!head&&!boost&&!step);
 if(scenario==9){assert(rc==-2&&!scans&&!feeds);scenario=0;frozen_ptr=NULL;rc=printer_run_idle_ab(b,n);}
 if(!scenario){
  assert(!rc&&scans==6&&feeds==6&&sends==7080&&ons==1&&offs==2);
  job_lines=job_feeds=0;assert(!printer_run_idle_ab(alt,alt_n));
  assert(scans==12&&feeds==12&&sends==14160&&ons==2&&offs==4&&!live&&!head&&!boost);
 }else{
  assert(rc);unsigned old=scans;assert(printer_run_idle_ab(b,n)==-7&&scans==old);
  if(scenario==1)assert(scans==1&&legs==1&&!feeds);
  if(scenario==2)assert(scans==2&&legs==3&&feeds==1);
  if(scenario==3||scenario==8)assert(!scans&&!feeds);
  if(scenario==4)assert(scans==6&&feeds==6);
  if(scenario==5||scenario==12)assert(scans==1&&feeds==1);
  if(scenario==6)assert(scans==2&&feeds==2);
 }
 free(alt);free(blank);free(b);
}
'''
code=code.replace('static unsigned active_first,active_last;','')
code=code.replace('static char phase[24],kind[24];','')
code=code.replace('static unsigned job_lines,job_feeds,run_number;','static unsigned job_lines,job_feeds;')
code=code.replace('line_index,planned_repeats,line_sends','line_index,line_sends')
code=code.replace('current_phase,current_kind','current_phase')
with tempfile.TemporaryDirectory() as tmp:
 t=Path(tmp);(t/'input.bin').write_bytes(blob(set()));(t/'blank.bin').write_bytes(blob(set()))
 (t/'alt.bin').write_bytes(blob({(ch,x,y) for ch in range(3) for x in range(150,1050) for y in range(1340,1352)}))
 (t/'gold.bin').write_bytes(b''.join(frames));(t/'test.c').write_text(code)
 subprocess.run(['cc','-std=c99','-O1','-fsanitize=address,undefined','-fno-omit-frame-pointer','-Wall','-Wextra','-Werror','-I'+str(D),str(t/'test.c'),str(D/'printer_page.c'),str(D/'printer_mapper.c'),'-o',str(t/'test')],check=True)
 arm=Path('/workspace/.setup/tools/usr/bin/arm-none-eabi-gcc')
 if arm.exists():subprocess.run([str(arm),'-std=gnu99','-mcpu=cortex-m33+nodsp','-mfpu=fpv5-sp-d16','-mfloat-abi=hard','-mcmse','-O2','-Wall','-Wextra','-Werror','-I'+str(D),'-c',str(t/'test.c'),'-o',str(t/'test.o')],check=True)
 for scenario in [0,0,1,2,3,4,5,6,8,9,12]:
  subprocess.run([str(t/'test'),str(t/'input.bin'),str(t/'blank.bin'),str(t/'gold.bin'),str(t/'alt.bin'),str(scenario)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
print('PASS: actual C/M/Y/Y/M/C order, 7080 exact columns, golden color/job CRCs, 1422 commands per nozzle, fresh/changed APP triggers, one head session, cleanup on CRC/SPI/power/feed/OOM/link faults; ARM compiled')
