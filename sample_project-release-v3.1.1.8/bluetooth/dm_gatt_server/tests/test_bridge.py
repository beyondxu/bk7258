"""Exercise the actual BK SPI bridge with deterministic GPIO/SPI mocks."""
from pathlib import Path
import subprocess
import tempfile

source = Path(__file__).resolve().parents[1] / 'ap/app_bluetooth/gatt/printer/tph_driver.c'
harness = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define TPH_DRIVER_H
#define MOTOR_SDK_DRIVER_H
#define __IRAM_SEC
#define BK_OK 0
#define BK_FAIL -1
#define TPH_COLUMN_BYTES 100
#define PIN_BK_READY_IN 0
#define PIN_BK_SYNC_OUT 1
#define GPIO_UP(pin) bk_gpio_set_output_high(pin)
#define SPI_ID_1 1
#define GPIO_OUTPUT_ENABLE 1
#define GPIO_INPUT_ENABLE 2
#define GPIO_PULL_DISABLE 0
#define GPIO_PULL_UP_EN 1
#define GPIO_SECOND_FUNC_DISABLE 0
#define SPI_ROLE_MASTER 1
#define SPI_BIT_WIDTH_8BITS 8
#define SPI_POLARITY_HIGH 1
#define SPI_PHASE_2ND_EDGE 1
#define SPI_4WIRE_MODE 4
#define SPI_MSB_FIRST 1
#define SPI_DMA_MODE_DISABLE 0
typedef int bk_err_t;
typedef int gpio_id_t;
typedef unsigned UINT32;
typedef struct {int io_mode, pull_mode, func_mode;} gpio_config_t;
typedef struct {int role, bit_width, polarity, phase, baud_rate, wire_mode, bit_order, dma_mode;} spi_config_t;
typedef struct {uint8_t c_data[25], m_data[25], y_data[25];} TphTestColumn_t;
static int scenario, reads, writes, lows, highs, spi_result, init_result;
static uint8_t sent[100];
int gpio_dev_unmap(int p) {return 0;}
void bk_gpio_set_config(int p, const gpio_config_t *c) {}
void bk_delay_us(UINT32 us) {}
void rtos_delay_milliseconds(int ms) {}
void bk_motor_boost_power(int on) {}
void bk_gpio_set_output_low(int p) {assert(p == 1); lows++;}
void bk_gpio_set_output_high(int p) {assert(p == 1); highs++;}
int bk_gpio_get_input(int p) {
    assert(p == 0); ++reads;
    if (scenario == 1) return 0; /* never ready */
    if (!lows) return 1;
    if (scenario == 2) return 1; /* no DMA acknowledgement */
    if (!highs || scenario == 3) return 0; /* busy / never rearmed */
    return 1;
}
int bk_spi_init(int id, const spi_config_t *cfg) {return init_result;}
int bk_spi_write_bytes(int id, const void *p, uint32_t n) {
    assert(id == 1 && n == 100 && !lows);
    memcpy(sent, p, n); ++writes; return spi_result;
}
bk_err_t tph_fire_raw_column_bytes(const uint8_t *);
#define os_printf printf
#include "REAL_SOURCE"
static void reset(int s) {
    scenario=s; reads=writes=lows=highs=0; spi_result=init_result=0;
    memset(sent, 0, sizeof(sent));
}
int main(void) {
    uint8_t col[100]={0}; col[40]=1;
    reset(0); assert(tph_fire_raw_column_bytes(col)==BK_OK);
    assert(writes==1 && lows==1 && highs==1 && sent[40]==1);
    reset(1); assert(tph_fire_raw_column_bytes(col)==BK_FAIL);
    assert(writes==0 && lows==0);
    reset(2); assert(tph_fire_raw_column_bytes(col)==BK_FAIL);
    assert(writes==1 && lows==1 && highs==1);
    reset(3); assert(tph_fire_raw_column_bytes(col)==BK_FAIL);
    assert(writes==1 && highs==1);
    reset(0); spi_result=BK_FAIL;
    assert(tph_fire_raw_column_bytes(col)==BK_FAIL && lows==0);
    reset(0); assert(tph_fire_raw_column_bytes(NULL)==BK_FAIL && writes==0);
    assert(tph_fire_one_column_via_mcu(NULL)==BK_FAIL);
    reset(0); init_result=BK_FAIL; assert(tph_mcu_comm_init()==BK_FAIL);
    reset(0); TphTestColumn_t pixels={0}; pixels.c_data[0]=1;
    assert(tph_fire_one_column_via_mcu(&pixels)==BK_OK);
    assert(sent[0]==1); for(int i=1;i<100;i++) assert(sent[i]==0);
    puts("PASS: success, not ready, no ACK, no rearm, SPI/init errors, NULL, one-bit packing");
}
'''
with tempfile.TemporaryDirectory() as tmp:
    root = Path(tmp)
    (root / 'driver').mkdir()
    (root / 'driver/spi.h').write_text('/* test SDK declarations are in the harness */\n')
    (root / 'test.c').write_text(harness.replace('REAL_SOURCE', str(source)))
    subprocess.run(['cc', '-std=c99', '-I', tmp, str(root/'test.c'), '-o', str(root/'test')], check=True)
    subprocess.run([str(root/'test')], check=True)
