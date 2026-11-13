"""Compile the actual protocol with storage/notification stubs; no SDK needed."""
from pathlib import Path
import os
import subprocess
import tempfile
source = Path(__file__).resolve().parents[1] / 'ap/app_bluetooth/gatt/printer/printer_protocol.c'
harness = r'''
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#define os_printf printf
static uint8_t storage[4096], rsp[128];
static uint32_t stored;
static int starts, fail_append;
int printer_ntf_data(uint8_t *p, uint16_t n) {memcpy(rsp,p,n); return 0;}
void hw_printer_create_task(void) {}
bool hw_printer_prepare_buffer(uint32_t n) {stored=0; return n<=sizeof(storage);}
bool hw_printer_append_data(const uint8_t *p,uint32_t n) {
 if(fail_append || n>sizeof(storage)-stored) return false;
 memcpy(storage+stored,p,n);stored+=n;return true;
}
void hw_printer_clear_buffer(void) {stored=0;}
void hw_printer_start_job_async(void) {++starts;}
#include "SOURCE"
static unsigned frame(uint8_t *p, uint8_t sub, const uint8_t *params,unsigned n) {
 uint8_t h[]={0x1d,0,(uint8_t)(5+n),0,6,sub,1,(uint8_t)n,0};
 memcpy(p,h,9);memcpy(p+9,params,n);memset(p+9+n,0,4);p[13+n]=0x1e;return n+14;
}
static unsigned start(uint8_t *p, const uint8_t *bin,unsigned n,uint32_t crc) {
 uint8_t params[]={1,2,0,0x22,0x11,2,4,0,(uint8_t)n,(uint8_t)(n>>8),0,0,
                   3,4,0,(uint8_t)crc,(uint8_t)(crc>>8),(uint8_t)(crc>>16),(uint8_t)(crc>>24)};
 return frame(p,3,params,sizeof(params));
}
int main(void) {
 uint8_t f[128],finish[32],id[]={0x22,0x11};
 uint8_t bin[]={0x1d,0,5,0,6,1,1,0,0,0,0,0,0,0x1e,0x1b,0x25,0x36,0x80};
 uint32_t crc=printer_crc32(bin,sizeof(bin));
 assert(printer_crc32((const uint8_t*)"123456789",9)==0xCBF43926);
 printer_protocol_init();
 unsigned n=start(f,bin,sizeof(bin),crc);
 printer_process_data(f,20);assert(s_job.state==JOB_STATE_IDLE);
 printer_process_data(f+20,n-20);assert(s_job.state==JOB_STATE_RECEIVING_BIN && rsp[9]==0);
 for(unsigned i=0;i<sizeof(bin);i++)printer_process_data(bin+i,1);
 assert(stored==sizeof(bin) && !memcmp(storage,bin,stored));
 unsigned m=frame(finish,4,id,2);printer_process_data(finish,m);
 assert(starts==1 && s_job.state==JOB_STATE_PRINTING && rsp[17]==2);
 printer_protocol_reset();assert(stored==sizeof(bin)); /* disconnect must not free worker memory */
 uint32_t saved=stored;hw_printer_clear_buffer();printer_protocol_audit_complete(saved,crc);
 assert(s_job.state==JOB_STATE_IDLE && rsp[17]==0);
 /* CRC mismatch cannot start the worker. */
 n=start(f,bin,sizeof(bin),crc^1);printer_process_data(f,n);
 printer_process_data(bin,sizeof(bin));printer_process_data(finish,m);
 assert(starts==1 && rsp[17]==1 && stored==0);
 /* Start+BIN+finish in one BLE callback works with declared lengths. */
 uint8_t batch[256];n=start(batch,bin,sizeof(bin),crc);
 memcpy(batch+n,bin,sizeof(bin));memcpy(batch+n+sizeof(bin),finish,m);
 printer_process_data(batch,n+sizeof(bin)+m);assert(starts==2);
 saved=stored;hw_printer_clear_buffer();printer_protocol_audit_complete(saved,crc);
 /* A mismatched finish ID cannot be reported as success. */
 n=start(f,bin,sizeof(bin),crc);printer_process_data(f,n);
 printer_process_data(bin,sizeof(bin));
 uint8_t wrong_id[]={0x23,0x11};unsigned bad_finish=frame(finish,4,wrong_id,2);
 printer_process_data(finish,bad_finish);assert(starts==2 && rsp[17]==1);
 m=frame(finish,4,id,2);
 /* Truncated TLV is rejected without allocating a job. */
 uint8_t bad[]={1,4,0,1};n=frame(f,3,bad,sizeof(bad));printer_process_data(f,n);
 assert(rsp[9]==1 && s_job.state==JOB_STATE_IDLE);
 /* Failed storage writes do not advance receive count or launch a worker. */
 n=start(f,bin,sizeof(bin),crc);printer_process_data(f,n);fail_append=1;
 printer_process_data(bin,sizeof(bin));assert(starts==2 && s_job.state==JOB_STATE_IDLE);
 /* Execution failures retain failed finish without claiming corrupt input. */
 fail_append=0;
 const int results[]={-7,-8,-9,-10,-12,-2,-11,-4,-1,0};
 for(unsigned mode=0;mode<2;mode++)for(unsigned i=0;i<sizeof(results)/sizeof(results[0]);i++){
  n=start(f,bin,sizeof(bin),crc);printer_process_data(f,n);
  printer_process_data(bin,sizeof(bin));printer_process_data(finish,m);
  assert(s_job.state==JOB_STATE_PRINTING);
  saved=stored;hw_printer_clear_buffer();
  int rc=results[i];
  if(mode)printer_protocol_bounded_complete(saved,crc,rc);
  else printer_protocol_page_complete(saved,crc,rc);
  assert(s_job.state==JOB_STATE_IDLE&&!s_status.work);
  assert(rsp[17]==(rc?1:0));
  assert(s_status.fault==((rc==-11||rc==-4||rc==-1)?8:0));
 }
 for(unsigned mode=0;mode<2;mode++){
  n=start(f,bin,sizeof(bin),crc);printer_process_data(f,n);
  printer_process_data(bin,sizeof(bin));printer_process_data(finish,m);
  saved=stored;hw_printer_clear_buffer();
  if(mode)printer_protocol_bounded_complete(saved,crc^1,-7);
  else printer_protocol_page_complete(saved,crc^1,-7);
  assert(rsp[17]==1&&s_status.fault==8&&s_job.state==JOB_STATE_IDLE);
 }
 puts("PASS: execution errors preserve failed finish; real data faults retain bit3");
 puts("PASS: fragmented/coalesced frames, binary frame signatures, CRC, TLV bounds, storage failure, worker ownership");
}
'''.replace('SOURCE',str(source))
with tempfile.TemporaryDirectory() as tmp:
 root=Path(tmp)
 for name in ('components/log.h','os/mem.h','os/str.h','os/os.h'):
  p=root/name;p.parent.mkdir(exist_ok=True);p.write_text('/* host stub */\n')
 (root/'test.c').write_text(harness)
 subprocess.run(['cc','-std=c99','-fsanitize=address,undefined','-g','-I',tmp,str(root/'test.c'),'-o',str(root/'test')],check=True)
 subprocess.run([str(root/'test')],check=True,env={**os.environ,"ASAN_OPTIONS":"detect_leaks=0"})
