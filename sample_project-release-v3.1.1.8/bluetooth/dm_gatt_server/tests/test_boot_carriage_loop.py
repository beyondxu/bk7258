"""Temporary boot loop: actual timer/optical motor engine, never ink or paper."""
from pathlib import Path
import ast,os,subprocess,tempfile
D=Path(__file__).resolve().parents[1]/'ap/app_bluetooth/gatt/printer'
H=Path(os.environ.get('HC32_SOURCE',str(Path(__file__).resolve().parents[4].parent/'hc32f460/projects/ev_hc32f460_lqfp100_v2/examples/spi/spi_dma/source')))
s=Path(__file__).with_name('test_buffered_motion.py').read_text();tree=ast.parse(s)
node=next(n for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='h' for t in n.targets)).value
while isinstance(node,ast.Call):node=node.func.value
h=ast.literal_eval(node);h=h[:h.index('int main(')]
transport=(D/'tph_driver.c').read_text(encoding='latin1');transport=transport[transport.index('/* LBUFv1:'):]
f=(D/'printer_bidirectional_print.h').read_text();finish=f[f.index('static int printer_bidirectional_finish(void)'):f.rindex('#endif')]
h=h.replace('TRANSPORT',transport).replace('REAL_FINISH',finish)
h=h.replace('#define os_printf printf','static int loop_printf(const char*,...);\n#define os_printf loop_printf')
h=h.replace('period>=82U&&period<=400U','period>=75U&&period<=400U')
h=h.replace('prior_step_period==82U&&timer_period==82U','prior_step_period==75U&&timer_period==75U').replace('now_us-step_rise_us==82U','now_us-step_rise_us==75U')
h=h.replace('static int motor_oc3_ready(void){return mode!=20;}', 'static unsigned oc3_reads,oc3_ready_reads;\nstatic int motor_oc3_ready(void){++oc3_ready_reads;return mode!=20;}')
h=h.replace('static int motor_oc3_active(void){return !oc3_level;}', 'static int motor_oc3_active(void){++oc3_reads;return !oc3_level;}')
h=h.replace('++oc3_events;oc3_level=0;pos=PRINTER_OC3_REFERENCE_STEPS;', '++oc3_events;oc3_level=0;if(!PRINTER_BOOT_LOOP_IGNORE_OC3)pos=PRINTER_OC3_REFERENCE_STEPS;')
h=h.replace('assert(oc3_handler);oc3_handler();', 'if(PRINTER_BOOT_LOOP_IGNORE_OC3){assert(!oc3_handler);}else{assert(oc3_handler);oc3_handler();}')
h+=r'''
#define PRINTER_BOOT_CARRIAGE_LOOP 1
#define PRINTER_FULL_PAGE 1
#define PRINTER_PAGE_BIDIRECTIONAL 1
static unsigned rounds,which;
static bool printer_link_is_ready(void){return which!=1;}
static int loop_printf(const char*fmt,...){
 if(strstr(fmt,"[BOOT_CARRIAGE_LOOP_ROUND]")){
  ++rounds;
  /* End an otherwise infinite loop using the real late-IRQ guard next leg. */
  if(rounds==3)mode=4;
 }
 return 0;
}
#include "printer_boot_carriage_loop.h"
int main(int argc,char**argv){
 assert(argc==2);which=atoi(argv[1]);mode=29;
 pos=-(int32_t)PRINTER_MAINTENANCE_EXTRA_STEPS;
 s_bidi_left_offset=pos;s_bidi_at_wiper=true;
 if(which==2){oc3_level=0;if(PRINTER_BOOT_LOOP_IGNORE_OC3)mode=24;}
 if(which==3)mode=PRINTER_BOOT_LOOP_IGNORE_OC3?20:0; /* Sensor never trips: stop at the bounded seek cap. */
 if(which==4)mode=13; /* GPIO falls during the outward stroke. */
 int rc=printer_run_boot_carriage_loop();
 assert(!timer_enabled&&!s_buf_boot_loop_profile&&!hc_fires&&!total_paper&&sync_high);
 if(which==1)assert(rc==-7&&!rounds&&!boost&&!oc3_right_pulses);
 else if(which==2&&!PRINTER_BOOT_LOOP_IGNORE_OC3)assert(rc==PRINTER_OC3_STUCK&&!rounds&&!boost&&!oc3_right_pulses);
 else if(which==3&&!PRINTER_BOOT_LOOP_IGNORE_OC3)assert(rc==PRINTER_OC3_NOT_FOUND&&!rounds&&!boost&&oc3_right_pulses==PRINTER_OC3_BOOT_SEARCH_STEPS);
 else{
  assert(rc==-14&&rounds==3&&!boost);
#if PRINTER_BOOT_LOOP_IGNORE_OC3
  assert(!oc3_reads&&!oc3_ready_reads&&!oc3_handler);
  assert(s_blank_spi_steps[1]==3U*PRINTER_BOOT_LOOP_STEPS+3000U);
  assert(s_blank_spi_steps[0]==3U*PRINTER_BOOT_LOOP_STEPS);
#else
  assert(oc3_events==3);
#endif
  assert(fast_edges>60000U); /*75us plateau in both directions, not task timing. */
  assert(pos==-(int32_t)PRINTER_MAINTENANCE_EXTRA_STEPS+3000);
 }
 puts("PASS75us loop: optical mode and GPIO-independent fixed legs, exact round-trip counts, IRQ fault stop, no ink/feed");
}
'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t);(p/'test.c').write_text(h);(p/'driver').mkdir();(p/'driver/timer.h').write_text('/* deterministic timer API */\n')
 for ignore in [0,1]:
  subprocess.run(['cc',f'-DPRINTER_BOOT_LOOP_IGNORE_OC3={ignore}','-std=c99','-O1','-Wall','-Wextra','-Werror','-Wno-unused-function','-fsanitize=address,undefined','-I'+str(p),'-I'+str(D),'-I'+str(H),str(p/'test.c'),'-o',str(p/'test')],check=True)
  for mode in range(5 if ignore else 4):subprocess.run([str(p/'test'),str(mode)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
# The boot entry is before BLE, and blocks printing on every exit/failure.
main=(D.parents[2]/'ap_main.c').read_text(encoding='latin1')
a=main.index('    if (PRINTER_BOOT_CARRIAGE_LOOP) {');b=main.index('    if (link_ready && PRINTER_CARRIAGE_TEST)',a)
block=main[a:b]
assert a<main.index('dm_gatts_init();')
assert 'if (link_ready)' in block and 'link_ready=false;s_printer_link_ready=false;' in block

assert main.index('[BOOT_MODE] TEMP_COUNTED_NO_OC3_V43')<a
assert '[BOOT_CARRIAGE_LOOP_RESULT]' in (D/'printer_boot_carriage_loop.h').read_text()
