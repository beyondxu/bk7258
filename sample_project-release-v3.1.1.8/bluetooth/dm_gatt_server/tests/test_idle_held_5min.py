"""Five-minute held-power mode: same real renderer and motion trace harness."""
from pathlib import Path
from test_idle_ab import h,run_scenarios,D,P
assert '#define PRINTER_IDLE_HELD_ONLY 1' in (D/'printer_diagnostics.h').read_text()
assert '#define PRINTER_IDLE_AB_GAP_MS 300000U' in (D/'printer_diagnostics.h').read_text()
assert 'APP_IDLE_HELD_5MIN_V1' in (P/'ap/ap_main.c').read_text()
s=h.replace('#define PRINTER_IDLE_AB 1','#define PRINTER_IDLE_AB 1\n#define PRINTER_IDLE_HELD_ONLY 1\n#define PRINTER_IDLE_AB_GAP_MS 300000U')
s=s.replace('scenario==4&&commands==8','scenario==4&&commands==4')
s=s.replace("scenario==10&&current_phase=='A'", "scenario==10&&current_phase=='B'")
s=s.replace('scenario==2&&job_lines==10','scenario==2&&job_lines==4')
s=s.replace('static void rtos_delay_milliseconds(unsigned n){assert(!step);us+=(uint64_t)n*1000U;}', '''static void rtos_delay_milliseconds(unsigned n){
 assert(!step);us+=(uint64_t)n*1000U;
 if(scenario==7&&!corrupted&&job_lines==3&&last_active[2]&&us-last_active[2]>298000000ULL){us+=2000000ULL;corrupted=1;}
}''')
a=s.index('static void check_run(');b=s.index('int main(',a)
s=s[:a]+'''static void check_run(unsigned offset,bool unused){
 (void)unused;
 for(unsigned i=0;i<6;++i)assert(phase[offset+i]=='B'&&kind[offset+i]==(i<3?'R':'O'));
 uint64_t gap=first_active[offset+3]-last_active[offset+2];assert(gap>=299900000ULL&&gap<=300100000ULL);
}
'''+s[b:]
s=s.replace('scans==12&&feeds==12&&sends==12*1180','scans==6&&feeds==6&&sends==6*1180')
s=s.replace('ons==3&&offs==5&&gpio_inits==2&&boost_ons==2&&boost_offs==2','ons==1&&offs==3&&gpio_inits==1&&boost_ons==1&&boost_offs==1')
s=s.replace('scans==24&&feeds==24&&sends==24*1180','scans==12&&feeds==12&&sends==12*1180')
s=s.replace('check_run(12,true);assert(ons==6&&gpio_inits==4&&boost_ons==4&&boost_offs==4);','check_run(6,false);assert(ons==2&&gpio_inits==2&&boost_ons==2&&boost_offs==2);')
s=s.replace('scenario==2)assert(scans==10&&legs==19&&feeds==9)','scenario==2)assert(scans==4&&legs==7&&feeds==3)')
s=s.replace('scenario==4)assert(scans==12&&feeds==12)','scenario==4)assert(scans==6&&feeds==6)')
s=s.replace('scenario==6)assert(scans==9&&feeds==9)','scenario==6)assert(scans==3&&feeds==3)')
run_scenarios(s,[0,1,2,3,4,5,6,7,8,10,12])
print('PASS: six scans only, single ON, no idle power/boost reset, real 300s ACK gap and +/-100ms rejection, frozen data, repeat remains HELD_ONLY, all failure stops and ARM compilation')
