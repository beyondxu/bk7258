"""Actual async producer and shared worker: no callback motor work or BIN fallback."""
from pathlib import Path
import subprocess,tempfile,os
D=Path(__file__).resolve().parents[1]/'ap/app_bluetooth/gatt/printer'
s=(D/'hw_printer_stub.c').read_text();a=s.index('#if PRINTER_MAINTENANCE\nstatic volatile uint8_t s_maintenance_request');b=s.index('void hw_printer_start_job_async',a)
h=r'''
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include "printer_maintenance.h"
#include "printer_audit_utils.h"
#define PRINTER_MAINTENANCE 1
#define PRINTER_WASTE_MAINTENANCE 1
#define PRINTER_FULL_PAGE 1
#define PRINTER_APP_PAYLOAD_AUDIT 0
#define PRINTER_DATA_AUDIT 0
#define PRINTER_MAP_AUDIT 0
#define PRINTER_BOUNDED_LINE 0
#define PRINTER_BOUNDED_PRINT 0
#define PRINTER_AUDIT_DUMP_MAX_BYTES 32
#define BK_OK 0
#define BEKEN_WAIT_FOREVER -1
#define os_printf printf
static void*s_print_sem=(void*)1,*s_print_task_handle=(void*)1;
static uint8_t*s_print_buffer;
static uint32_t s_buffer_len;
static unsigned queued,sem_calls,sem_fail,runs,completions,expected_mode;
static int run_result;
static jmp_buf stop;
static int rtos_set_semaphore(void**sem){assert(*sem);if(sem_fail)return -1;++queued;return 0;}
static void rtos_get_semaphore(void**sem,int timeout){assert(*sem&&timeout==-1);if(sem_calls++)longjmp(stop,1);assert(queued==1);--queued;}
static int printer_run_maintenance(uint8_t mode){assert(mode==expected_mode&&!s_print_buffer&&!s_buffer_len);++runs;return run_result;}
static void printer_protocol_maintenance_complete(uint8_t mode,int result);
static bool printer_protocol_job_buffer_valid(uint32_t n,uint32_t crc){(void)n;(void)crc;abort();}
static int printer_run_full_page(const uint8_t*p,uint32_t n){(void)p;(void)n;abort();}
static void hw_printer_clear_buffer(void){abort();}
static void printer_protocol_page_complete(uint32_t n,uint32_t c,int r){(void)n;(void)c;(void)r;abort();}
static void rtos_delay_milliseconds(unsigned n){(void)n;}
static void printer_run_mapping_audit(const uint8_t*p,uint32_t n){(void)p;(void)n;abort();}
static int printer_run_bounded_line(const uint8_t*p,uint32_t n){(void)p;(void)n;abort();}
static void printer_protocol_bounded_complete(uint32_t n,uint32_t c,int r){(void)n;(void)c;(void)r;abort();}
static void printer_protocol_audit_complete(uint32_t n,uint32_t c){(void)n;(void)c;abort();}
static void parse_and_execute_stream_perfect(const uint8_t*p,uint32_t n){(void)p;(void)n;abort();}
WORKER
static void printer_protocol_maintenance_complete(uint8_t mode,int result){assert(mode==expected_mode&&result==run_result&&!s_maintenance_request);++completions;}
int main(void){
 assert(!hw_printer_start_maintenance_async(3));
 s_print_sem=NULL;assert(!hw_printer_start_maintenance_async(0));s_print_sem=(void*)1;
 s_print_buffer=(void*)1;assert(!hw_printer_start_maintenance_async(0));s_print_buffer=NULL;
 sem_fail=1;assert(!hw_printer_start_maintenance_async(1)&&!s_maintenance_request&&!runs&&!queued);sem_fail=0;
 for(unsigned mode=0;mode<3;++mode){
  expected_mode=mode;run_result=mode?-10:0;sem_calls=0;
  assert(hw_printer_start_maintenance_async(mode)&&s_maintenance_request==mode+1&&queued==1);
  assert(runs==mode&&!hw_printer_start_maintenance_async(mode));
  if(!setjmp(stop))printer_worker_thread(NULL);
  assert(!queued&&!s_maintenance_request&&runs==mode+1&&completions==mode+1);
 }
 puts("PASS actual async maintenance queue: no motion at enqueue, rejects duplicate/unready/BIN ownership, completes success/failure without print fallback");
}
'''.replace('WORKER',s[a:b])
# Public completion declaration has external linkage.
h=h.replace('static void printer_protocol_maintenance_complete','void printer_protocol_maintenance_complete')
with tempfile.TemporaryDirectory() as tmp:
 t=Path(tmp);(t/'test.c').write_text(h)
 subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-Wno-unused-function','-fsanitize=address,undefined','-I'+str(D),str(t/'test.c'),'-o',str(t/'test')],check=True)
 subprocess.run([str(t/'test')],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
