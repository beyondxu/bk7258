"""Exercise actual buffer functions/worker in receive-only mode with host RTOS stubs."""
from pathlib import Path
import os
import subprocess
import tempfile

base=Path(__file__).resolve().parents[1]/'ap/app_bluetooth/gatt/printer'
s=(base/'hw_printer_stub.c').read_text()
buffers=s[s.index('bool hw_printer_prepare_buffer'):s.index('static int decode_packbits_full')]
worker=s[s.index('static void printer_worker_thread'):s.index('void hw_printer_start_job_async')]
harness=r'''
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include "DIAGNOSTICS"
#include "UTILS"
#define os_printf printf
#define psram_malloc malloc
#define psram_free free
#define BEKEN_WAIT_FOREVER -1
static uint8_t *s_print_buffer;
static uint32_t s_max_buffer_size,s_buffer_len;
static void *s_print_sem;
static jmp_buf done;
static unsigned sem_calls, physical_calls, completed, mapping_calls;
static void printer_run_mapping_audit(const uint8_t *p,uint32_t length) {assert(length==9);++mapping_calls;}
static int printer_run_bounded_line(const uint8_t *p,uint32_t n) {assert(n==9);return 0;}
static void printer_protocol_bounded_complete(uint32_t n,uint32_t crc,int result) {abort();}
static void rtos_delay_milliseconds(unsigned ms) {}
static void rtos_get_semaphore(void **s,int timeout) {if(sem_calls++)longjmp(done,1);}
static void parse_and_execute_stream_perfect(const uint8_t *p,uint32_t len) {++physical_calls;}
static void printer_protocol_audit_complete(uint32_t length,uint32_t crc) {
 assert(length==9 && crc==0xCBF43926);
 assert(!s_print_buffer && s_buffer_len==0);++completed;
}
BUFFERS
WORKER
int main(void) {
 assert(!hw_printer_prepare_buffer(0));
 assert(!hw_printer_prepare_buffer(4U*1024U*1024U+1));
 assert(hw_printer_prepare_buffer(9));
 assert(hw_printer_append_data((const uint8_t*)"1234",4));
 assert(!hw_printer_append_data((const uint8_t*)"overflow",8));
 assert(s_buffer_len==4);
 assert(hw_printer_append_data((const uint8_t*)"56789",5));
 if(!setjmp(done))printer_worker_thread(NULL);
 assert(completed==1 && physical_calls==0 && mapping_calls==1);
 puts("PASS: actual worker dumps stored BIN, reports saved CRC, frees before completion and never calls print engine");
}
'''.replace('DIAGNOSTICS',str(base/'printer_diagnostics.h')).replace('UTILS',str(base/'printer_audit_utils.h')).replace('BUFFERS',buffers).replace('WORKER',worker)
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.c').write_text(harness)
 subprocess.run(['cc','-std=c99','-DPRINTER_BOUNDED_PRINT=0','-DPRINTER_FULL_PAGE=0','-DPRINTER_IDLE_AB=0','-DPRINTER_MAP_AUDIT=1','-DPRINTER_AUDIT_DUMP_MAX_BYTES=65536','-fsanitize=address,undefined',str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
