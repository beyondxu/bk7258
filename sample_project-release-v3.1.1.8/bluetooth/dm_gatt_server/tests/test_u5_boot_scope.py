"""Actual boot scope loop with fake RTC; no real one-minute wait."""
from test_page_feed import h,src
from pathlib import Path
import tempfile,subprocess
source=src.read_text();boot=source[source.index('/* Independent boot-only scope session.'):]
code=h[:h.index('int main(void)')]
code=code.replace('static unsigned pulses,high,boost,dir,boost_calls,inits;',
 'static uint64_t fake_us;static uint64_t bk_aon_rtc_get_us(void){return fake_us;}\nstatic unsigned pulses,high,boost,dir,boost_calls,inits;')
code=code.replace('static void bk_delay_us(unsigned us){','static void bk_delay_us(unsigned us){fake_us+=us;')
code=code.replace('(void)ms;', 'fake_us+=(uint64_t)ms*1000;')
code=code.replace('pin==18||pin==55||pin==44||pin==53||pin==54','pin==18||pin==55||pin==44||pin==53||pin==54||pin==46||pin==47||pin==17')
code=code.replace('assert(us==4)', 'assert(us==10)')
code += "\ntypedef enum {MODE_HALF_STEP=1} MotorStepMode;\n#define TPH_MOT_MS1 23\n#define TPH_MOT_MS2 22\n#define TPH_MOT_MS3 15\n"
setter=source[source.index('void bk_motor_set_microstep('):source.index('// ★ 关键修正：移除脉冲循环内的 rtos_delay_milliseconds')]
code += setter
code=code.replace('assert(!high&&boost);', 'assert(!high&&boost);assert(regs[46]==2&&regs[47]==0&&regs[17]==0);')
code+=boot+r'''
int main(void){
 motor_u5_boot_scope_test();assert(pulses>19000&&pulses<20000&&!boost&&!high);
 assert(fake_us>=60075000ULL&&fake_us<60079000ULL);
 unsigned old=pulses;config_fail=1;motor_u5_boot_scope_test();assert(pulses==old&&!boost&&!high);
 puts("PASS: actual boot scope runs 60 seconds by RTC, known half-step/10us/3000us profile, bounded pulses, boost OFF, no pulses on config failure");
}
'''
main=(src.parents[3]/'ap_main.c').read_text()
start=main.index('#if PRINTER_U5_BOOT_SCOPE',main.index('static void app_printer_task_entry'))
end=main.index('#endif',start)
branch=main[start:end]
assert 'motor_u5_boot_scope_test();' in branch and 'while (1) rtos_delay_milliseconds(1000);' in branch
assert end<main.index('bk_spi_master_selftest_100b()',end)
assert 'dm_gatts' not in branch and 'tph_mcu_set_head_power' not in branch
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.c').write_text(code)
 subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror',str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
