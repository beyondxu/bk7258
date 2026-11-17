"""Real new timer API uses fixed auto-reload counts, never current+delay."""
from pathlib import Path
import subprocess,tempfile
R=Path(__file__).resolve().parents[4];src=R/'ap/middleware/driver/timer/timer_driver.c'
s=src.read_text();code=s[s.index('/* Printer-owned fixed-period timer API.'):s.index('bk_err_t bk_timer_start(timer_id_t')]
h=r'''
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
typedef int bk_err_t;typedef int timer_id_t;typedef void(*timer_isr_t)(timer_id_t);
#define BK_OK 0
#define BK_FAIL -1
#define BK_ERR_TIMER_IS_RUNNING -3
#define TIMER_ID0 0
#define TIMER_UNIT_US 1
#define BIT(i) (1U<<(i))
#define CONFIG_TIMER_SUPPORT_ID_BITS 56
#define TIMER_RETURN_ON_NOT_INIT() do{}while(0)
#define TIMER_RETURN_ON_INVALID_ID(i) do{if((unsigned)(i)>=6)return -1;}while(0)
#define TIMER_RETURN_TIMER_ID_IS_ERR(i) do{if(!(56U&BIT(i)))return -1;}while(0)
#define TIMER_PM_CHECK_RESTORE(i) do{(void)i;}while(0)
static struct{struct{void*hw;}hal;}s_timer;
static timer_isr_t s_timer_isr[6];
static unsigned enabled,count,period,starts,clocks,ints;
static void timer_chan_init_common(int id){assert(id==4);++clocks;}
static void timer_chan_enable_interrupt_common(int id){assert(id==4);++ints;}
static unsigned timer_hal_get_enable_status(void*p){(void)p;return enabled;}
static unsigned timer_hal_cal_end_count(int id,uint64_t us,unsigned div,unsigned unit){assert(id==4&&div==1&&unit==1);return us*26U;}
static void timer_hal_init_timer(void*p,int id,unsigned us,unsigned unit){(void)p;assert(unit==1);period=timer_hal_cal_end_count(id,us,1,unit);}
static void timer_hal_start_common(void*p,int id){(void)p;assert(s_timer_isr[id]);enabled|=BIT(id);++starts;}
static unsigned timer_hal_get_count(void*p,int id){(void)p;assert(id==4);return count;}
static void timer_ll_set_end_count(void*p,int id,unsigned end){(void)p;assert(id==4);period=end;}
CODE
static void cb(int id){(void)id;}
int main(void){
 assert(bk_timer_start_us(0,120,cb)==-1);assert(bk_timer_start_us(4,49,cb)==-1);assert(bk_timer_start_us(4,120,NULL)==-1);
 enabled=BIT(5);assert(!bk_timer_start_us(4,1000,cb));assert(period==26000&&enabled==(BIT(4)|BIT(5))&&starts==1);
 assert(bk_timer_start_us(4,120,cb)==-3&&starts==1&&clocks==1&&ints==1);
 count=80;assert(!bk_timer_set_period_us(4,120));assert(period==3120); /* NOT3200=current+delta */
 count=3000;assert(bk_timer_set_period_us(4,120)==-1&&period==3120);
 assert(bk_timer_set_period_us(4,0)==-1);
}
'''.replace('CODE',code)
with tempfile.TemporaryDirectory() as t:
 p=Path(t);(p/'test.c').write_text(h)
 subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror',str(p/'test.c'),'-o',str(p/'test')],check=True);subprocess.run([str(p/'test')],check=True)
print('PASS fixed microsecond periods, bounds, reserved/busy channel, callback-before-enable, no relative-count drift')
