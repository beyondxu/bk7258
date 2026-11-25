"""Actual timer, SPI cache and stationary purge: optical positioning, zero motion while firing."""
from pathlib import Path
import ast,os,subprocess,tempfile
D=Path(__file__).resolve().parents[1]/'ap/app_bluetooth/gatt/printer'
H=Path(os.environ.get('HC32_SOURCE','/workspace/hc32f460/projects/ev_hc32f460_lqfp100_v2/examples/spi/spi_dma/source'))
s=(Path(__file__).with_name('test_buffered_motion.py')).read_text();tree=ast.parse(s)
node=next(n for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='h' for t in n.targets)).value
while isinstance(node,ast.Call):node=node.func.value
h=ast.literal_eval(node);h=h[:h.index('int main(')]
transport=(D/'tph_driver.c').read_text(encoding='latin1');transport=transport[transport.index('/* LBUFv1:'):]
f=(D/'printer_bidirectional_print.h').read_text();finish=f[f.index('static int printer_bidirectional_finish(void)'):f.rindex('#endif')]
h=h.replace('TRANSPORT',transport).replace('REAL_FINISH',finish)
h=h.replace('static unsigned prior_step_period,', 'static unsigned stationary_run;\nstatic unsigned prior_step_period,')
h=h.replace('return now_us/32U*32U;', 'return stationary_run&&mode==47?0U:now_us/32U*32U;')
h=h.replace('mode==29||mode==30)', 'mode==29||mode==30||mode==40||mode==43||mode==44||mode==45||mode==46||mode==47||mode==48)')
h=h.replace('if(((mode==13', 'if((mode==51&&!oc3_events&&oc3_right_pulses==101U)||((mode==13')
h=h.replace('mode!=18&&mode!=24)', 'mode!=18&&mode!=24&&mode!=43)')
h=h.replace('if(cache.state==PL_ARMED){', 'if(cache.state==PL_ARMED&&mode!=45){')
h=h.replace('assert(timer_enabled);assert(pos==', 'assert(timer_enabled||stationary_run);if(stationary_run)assert(!timer_enabled&&pos==(int32_t)PRINTER_OC3_REFERENCE_STEPS);else assert(pos==')
h=h.replace('if(mode!=4){', 'if(stationary_run){assert(!timer_enabled&&pos==(int32_t)PRINTER_OC3_REFERENCE_STEPS);}else if(mode!=4){')
h=h.replace('++cache.fired;++hc_fires;\n   if(cache.fired==1180)', '++cache.fired;++hc_fires;\n   if(mode==46&&cache.fired==10){pl_fail(&cache,PL_TIMING);continue;}\n   if(cache.fired==1180)')
h=h.replace('return !sync_high&&(cache.state==PL_RUNNING', 'if(stationary_run&&mode==48&&cache.fired>=10)return 1U;if(stationary_run&&mode==46&&!sync_high&&cache.state==PL_ERROR)return 0U;return !sync_high&&(cache.state==PL_RUNNING')
h=h.replace('if(mode==2&&cache.state==PL_LOADING', 'if((mode==2||mode==44)&&cache.state==PL_LOADING')
h=h.replace('mode==4&&s_buf_steps==3000', '(mode==4||mode==50)&&s_buf_steps==3000')
h=h.replace('pl_status(&cache,p);if(mode==1)', 'pl_status(&cache,p);if(mode==52||mode==54||(mode==53&&cache.state==PL_READY)){uint8_t*q=p;if(mode==52)q[10]=0;if(mode==53)q[11]=0;if(mode==54)q[32]=0;pl_put32(q+96,pl_crc(q,96));}if(mode==1)')
h=h.replace('static void rtos_delay_milliseconds(unsigned n){assert(n==1);advance(mode==5?7000U:1000U);}', 'static void rtos_delay_milliseconds(unsigned n){if(n==20U){assert(!timer_enabled&&sync_high);advance(20000U);if(mode==43)oc3_level=1;return;}assert(n==1);advance(mode==5?7000U:1000U);}')
h=h.replace('pl_status(&cache,p);if(mode==52', 'pl_status(&cache,p);if(mode==55&&cache.state==PL_READY)oc3_level=1;if(mode==52')
h=h.replace('++cache.fired;++hc_fires;', '++cache.fired;++hc_fires;if(stationary_run&&mode==56&&hc_fires==10U)oc3_level=1;')
h=h.replace('(int32_t)PRINTER_SERVICE_SPAN_STEPS','(int32_t)PRINTER_OC3_REFERENCE_STEPS')
h=h.replace('assert(cache.period_us==PRINTER_CANVAS_COLUMN_US);', 'assert(cache.period_us==(cache.fire_profile==PL_FIRE_PRESPRAY?PL_PRESPRAY_PERIOD_US:PRINTER_CANVAS_COLUMN_US));')
h=h.replace('pl_status(&cache,p);if(mode==55', 'pl_status(&cache,p);if(mode==57||mode==58){uint8_t*q=p;if(mode==57)q[10]&=~PL_CAP_PRESPRAY_FAST;else pl_put16(q+33,500U);pl_put32(q+96,pl_crc(q,96));}if(mode==55')
h+=r'''
#include "printer_waste_motion.h"
int main(int argc,char**argv){
 assert(argc==2);unsigned requested=atoi(argv[1]);mode=requested>=52?40:requested;
 pos=-(int32_t)PRINTER_MAINTENANCE_EXTRA_STEPS;s_bidi_left_offset=pos;s_bidi_at_wiper=true;
 uint8_t*row=malloc(PL_ROW_BYTES);for(unsigned i=0;i<PL_ROW_BYTES;i++)row[i]=(i%100U<52U&&i/100U%9U<8U)?1U:0U;
 if(mode==49){assert(printer_waste_fire(row)==-7&&!hc_fires&&!oc3_right_pulses);free(row);return 0;}
 if(mode==42)oc3_level=0;
 int rc=printer_waste_position();
 if(mode==41||mode==42||mode==43||mode==50||mode==51){
  assert(rc==((mode==41||mode==43)?PRINTER_OC3_NOT_FOUND:mode==50?-14:mode==51?-7:PRINTER_OC3_STUCK));
  assert(!s_waste_station_ready&&!hc_fires&&!timer_enabled&&sync_high);
  if(mode==42)assert(!oc3_right_pulses);
  free(row);return 0;
 }
 assert(!rc&&s_waste_station_ready&&s_bidi_known&&s_bidi_right&&!s_bidi_at_wiper&&s_bidi_right_offset==(int32_t)PRINTER_PRINT_ENDPOINT_INSET_STEPS);
 assert(pos==(int32_t)PRINTER_OC3_REFERENCE_STEPS&&!oc3_level&&!timer_enabled&&!s_buf_claimed);
 assert(s_blank_spi_steps[0]==0); /* No commanded left retreat before flash. */
 unsigned steps=s_blank_spi_steps[0]+s_blank_spi_steps[1],feeds=total_paper;
 uint64_t before=now_us;stationary_run=1;if(requested>=52)mode=requested;
 rc=requested>=52?printer_waste_fire_prespray(row):printer_waste_fire(row);
 assert(pos==(int32_t)PRINTER_OC3_REFERENCE_STEPS&&steps==s_blank_spi_steps[0]+s_blank_spi_steps[1]&&feeds==total_paper&&!timer_enabled&&sync_high);
 assert(now_us-before<900000U);
 if(requested==56){assert(rc==-13&&hc_fires>=10&&hc_fires<=27&&!s_waste_station_ready);printer_buffered_stop();free(row);puts("PASS optical release during firing aborts promptly");return 0;}
 if(requested>=52){assert(rc==-13&&!hc_fires&&!s_waste_station_ready);printer_buffered_stop();free(row);puts("PASS missing capability/wrong prespray profile/pad rejected before ARM and fire");return 0;}
 if(mode==40){
  assert(!rc&&cache.state==PL_DONE&&cache.fired==1180&&hc_fires==1180&&s_waste_station_ready);
  assert(cache.fire_profile==PL_FIRE_NORMAL);
  before=now_us;
  assert(!printer_waste_fire_prespray(row)&&cache.fire_profile==PL_FIRE_PRESPRAY);
  assert(cache.period_us==58U&&now_us-before>=68440U&&now_us-before<100000U);
  assert(pos==(int32_t)PRINTER_OC3_REFERENCE_STEPS&&steps==s_blank_spi_steps[0]+s_blank_spi_steps[1]&&feeds==total_paper);
  assert(!printer_waste_fire(row)&&cache.fire_profile==PL_FIRE_NORMAL);
  stationary_run=0;s_waste_station_ready=0;
  unsigned left_before=s_blank_spi_steps[0];
  assert(!printer_buffered_park_at_wiper()&&pos==-(int32_t)PRINTER_MAINTENANCE_EXTRA_STEPS);
  assert(s_blank_spi_steps[0]-left_before==PRINTER_OC3_FINAL_RETURN_STEPS);
 }else{
  assert(rc==(mode==47?-14:-13)&&!s_waste_station_ready);
  if(mode==44||mode==45)assert(!hc_fires);
  if(mode==46)assert(hc_fires==10);
  if(mode==48)assert(hc_fires>=10&&hc_fires<=12);
  if(mode==47)assert(hc_fires==1180);
 }
 printer_buffered_stop();free(row);
 puts("PASS real waste positioning and stationary cache playback: fixed dose, no U4/U5 steps, fault/clock/READY safeguards");
}
'''
with tempfile.TemporaryDirectory() as tmp:
 t=Path(tmp);(t/'driver').mkdir();(t/'driver/timer.h').write_text('/* stub provided in test */\n');(t/'test.c').write_text(h)
 subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-Wno-unused-function','-fsanitize=address,undefined','-I'+str(t),'-I'+str(D),'-I'+str(H),str(t/'test.c'),'-o',str(t/'test')],check=True)
 for mode in range(40,59):subprocess.run([str(t/'test'),str(mode)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
