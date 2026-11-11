"""Short three-line reference control with actual renderer and motion helpers."""
from test_idle_ab import h,run_scenarios,D,P
assert '#define PRINTER_IDLE_REFERENCE_ONLY 1' in (D/'printer_diagnostics.h').read_text()
assert '#define PRINTER_IDLE_OFF_ONLY 0' in (D/'printer_diagnostics.h').read_text()
s=h.replace('#define PRINTER_IDLE_AB 1','#define PRINTER_IDLE_AB 1\n#define PRINTER_IDLE_REFERENCE_ONLY 1')
s=s.replace('scenario==4&&commands==8','scenario==4&&commands==4')
s=s.replace('scenario==2&&job_lines==10','scenario==2&&job_lines==3')
a=s.index('static void check_run(');b=s.index('int main(',a)
s=s[:a]+'''static void check_run(unsigned offset,bool unused){
 (void)unused;
 for(unsigned i=0;i<3;++i)assert(phase[offset+i]=='C'&&kind[offset+i]=='R');
 assert(us<60000000ULL); // no 60/300-second wait or observation train
}
'''+s[b:]
s=s.replace('scans==12&&feeds==12&&sends==12*1180','scans==3&&feeds==3&&sends==3*1180')
s=s.replace('ons==3&&offs==5&&gpio_inits==2&&boost_ons==2&&boost_offs==2','ons==1&&offs==3&&gpio_inits==1&&boost_ons==1&&boost_offs==1')
s=s.replace('scans==24&&feeds==24&&sends==24*1180','scans==6&&feeds==6&&sends==6*1180')
s=s.replace('check_run(12,true);assert(ons==6&&gpio_inits==4&&boost_ons==4&&boost_offs==4);','check_run(3,false);assert(ons==2&&gpio_inits==2&&boost_ons==2&&boost_offs==2);')
s=s.replace('scenario==2)assert(scans==10&&legs==19&&feeds==9)','scenario==2)assert(scans==3&&legs==5&&feeds==2)')
s=s.replace('scenario==4)assert(scans==12&&feeds==12)','scenario==4)assert(scans==3&&feeds==3)')
run_scenarios(s,[0,1,2,3,4,5,8,12])
print('PASS: three reference scans only, frozen bytes on repeat, no idle or observation, one ON per job, cleanup/failure stops, native and ARM checks')
