"""Actual APP-triggered feed-only branch: zero fire/carriage, head OFF first."""
from test_fixed_s2_colors import code,D,blob
from pathlib import Path
import tempfile,subprocess,os
code='#define PRINTER_U5_FEED_ONLY 1\n'+code
at=code.index('#include "printer_idle_ab_job.h"')
code=code[:at]+r'''
static unsigned u5_feeds;
static int motor_page_feed(uint32_t steps,uint8_t direction,uint32_t*sent){
 assert(!head&&!boost&&!scans&&!feeds&&!sends&&offs==1);assert(steps==780&&direction==0);
 ++u5_feeds;*sent=scenario==5?0:steps;return scenario==5?-1:0;
}
'''+code[at:]
at=code.index('int main(int argc,char**argv)')
code=code[:at]+r'''
int main(int argc,char**argv){
 assert(argc==3);scenario=atoi(argv[2]);uint32_t n;uint8_t*b=readfile(argv[1],&n);
 link=0;assert(printer_run_idle_ab(b,n)==-7&&!u5_feeds&&!offs);link=1;
 int rc=printer_run_idle_ab(b,n);assert(!live&&!head&&!boost&&!scans&&!feeds&&!sends&&!ons);
 if(scenario==5){assert(rc==-10&&u5_feeds==1&&offs==1);assert(printer_run_idle_ab(b,n)==-7&&u5_feeds==1);}
 else{assert(!rc&&u5_feeds==1&&offs==1);}
 free(b);puts("PASS: valid APP input triggers one U5-only feed, head OFF, no carriage/no fire; failure lock retained");
}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.c').write_text(code);(p/'input.bin').write_bytes(blob(set()))
 subprocess.run(['cc','-std=c99','-O1','-fsanitize=address,undefined','-Wall','-Wextra','-Werror','-I'+str(D),str(p/'test.c'),str(D/'printer_page.c'),str(D/'printer_mapper.c'),'-o',str(p/'test')],check=True)
 for scenario in [0,5]:subprocess.run([str(p/'test'),str(p/'input.bin'),str(scenario)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
