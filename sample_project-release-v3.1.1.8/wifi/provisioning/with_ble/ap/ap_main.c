#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include "ble_cfg_utils.h"
#include <components/log.h>
#include "modules/pm.h"

extern uint32_t bk_misc_get_ap_reset_reason(void);
extern uint32_t bk_misc_get_cp_reset_reason(void);



#include <stdio.h>
#include "motor_sdk_driver.h"
#include "tph_driver.h"

#include "bk_misc.h"

// 假设 SDK 内部的串口打印接口
#define TEST_LOGD(...)   printf("[MOTOR_TEST] " __VA_ARGS__)
#define TEST_LOGE(...)   printf("[MOTOR_TEST_ERR] " __VA_ARGS__)

// ==========================================
// 1. 故障保护状态自检
// ==========================================
static uint8_t check_motor_fault(void) {
    // 读取 U4 和 U5 的故障引脚 (低电平代表异常)
    uint8_t u4_fault = (bk_gpio_get_input(TPH_MOT_nFA) == 0);
    uint8_t u5_fault =(bk_gpio_get_input(PAPER_MOT_nFA) == 0);

    if (u4_fault || u5_fault) {
        TEST_LOGE("Hardware Protection Triggered! U4 Fault: %d, U5 Fault: %d\r\n", u4_fault, u5_fault);
        return 0; // 状态正常
        // 紧急关闭两路电机的 DMOS 输出
        bk_gpio_set_output_high(TPH_MOT_EN);
        bk_gpio_set_output_high(PAPER_MOT_EN);
        
        // 关断 15V 供电，切断回路
        bk_motor_boost_power(0);
        return 1; // 存在故障
    }
    return 0; // 状态正常
}

// ==========================================
// 2. 双马达测试程序主逻辑
// ==========================================
void run_dual_motor_testx(void) {
     bk_err_t err;

	
    TEST_LOGD("=== Starting Stepper Motor Test (ATD5984) =\r\n");
#if 0
    // Step 1: 初始化 BK7258 主控端的 GPIO
    TEST_LOGD("1. Initializing BK7258 GPIO settings...\r\n");
    bk_motor_gpio_config_init();

    // Step 2: 启动 LDO/Boost 15V 供电轨
    TEST_LOGD("2. Enabling 15V Boost Power...\r\n");
    bk_motor_boost_power(1);

    // Step 3: 喷头字车移动电机 (U4) 独立测试
    TEST_LOGD("3. Testing Carriage Motor (U4) - 1/16 Microstepping...\r\n");
    bk_motor_set_state(1, 1);                 // 激活 U4
    bk_motor_set_microstep(1, MODE_1_16_STEP); // 设置为 1/16 细分
    
    TEST_LOGD("   Carriage rotating Forward 400 steps...\r\n");
    err = bk_motor_move_run(1, 400, 1, 1000); // 1000us 间距
    if (err != BK_OK) goto fault_handler;

    delay_ms(500); // 停顿 500ms

    TEST_LOGD("   Carriage rotating Backward 400 steps...\r\n");
    err = bk_motor_move_run(1, 400, 0, 1000);
    if (err != BK_OK) goto fault_handler;

    bk_motor_set_state(1, 0); // 释放并让 U4 进入休眠
    TEST_LOGD("   Carriage Motor Test Passed.\r\n");

    delay_ms(500);
#endif	
#if 1	

	// Step 1: 初始化 BK7258 主控端的 GPIO
	   TEST_LOGD("1. Initializing BK7258 GPIO settings...\r\n");
	   bk_motor_gpio_config_init();
	
	   TEST_LOGD("00 falut pin,TPH_MOT_nFA=%d,PAPER_MOT_nFA=%d...\r\n",bk_gpio_get_input(TPH_MOT_nFA),bk_gpio_get_input(PAPER_MOT_nFA));
	   // Step 2: 启动 LDO/Boost 15V 供电轨
	   TEST_LOGD("2. Enabling 15V Boost Power...\r\n");
	   bk_motor_boost_power(1);

	   // Step 3: 喷头字车移动电机 (U5) 独立测试
	   TEST_LOGD("3. Testing Carriage Motor (U5) - 1/16 Microstepping...\r\n");
	   bk_motor_set_state(0, 1);				 // 激活 U4
	   bk_motor_set_microstep(0, MODE_1_16_STEP); // 设置为 1/16 细分
	   
	   TEST_LOGD("	 Carriage rotating Forward 400 steps...\r\n");
	   err = bk_motor_move_run(0, 400000, 1, 1000); // 1000us 间距
	   if (err != BK_OK) goto fault_handler;
	
	   TEST_LOGD(" 111 falut pin,TPH_MOT_nFA=%d,PAPER_MOT_nFA=%d...\r\n",bk_gpio_get_input(TPH_MOT_nFA),bk_gpio_get_input(PAPER_MOT_nFA));
	   delay_ms(500); // 停顿 500ms
	
	   TEST_LOGD("	 Carriage rotating Backward 400 steps...\r\n");
	 //  err = bk_motor_move_run(0, 40000, 0, 1000);
	 //  if (err != BK_OK) goto fault_handler;
	
	   bk_motor_set_state(0, 0); // 释放并让 U5 进入休眠
	   TEST_LOGD("	 Carriage Motor Test Passed.\r\n");
	
	   delay_ms(500);
#endif

#if 0
    // Step 5: 双马达协同循环测试（模拟打印机连续工作3行）
    TEST_LOGD("5. Starting Cooperative Print Cycle Simulation (3 Rows)...\r\n");
    
    // 同时激活两路驱动
    bk_motor_set_state(1, 1);
    bk_motor_set_state(0, 1);
    bk_motor_set_microstep(1, MODE_1_16_STEP);
    bk_motor_set_microstep(0, MODE_1_8_STEP);

    for (int row = 1; row <= 3; row++) {
        TEST_LOGD("   [Row %d/3] Printing scan (Carriage sweeps forward)...\r\n", row);
        err = bk_motor_move_run(1, 600, 1, 800); // 模拟向右打印扫掠 600 步
        if (err != BK_OK) goto fault_handler;

        delay_ms(100);

        TEST_LOGD("   [Row %d/3] Feed motor advancing paper...\r\n", row);
        err = bk_motor_move_run(0, 80, 1, 1500);  // 纸张前进 80 步
        if (err != BK_OK) goto fault_handler;

        delay_ms(100);

        TEST_LOGD("   [Row %d/3] Carriage returning to home...\r\n", row);
        err = bk_motor_move_run(1, 600, 0, 400);  // 快速复位退回左侧起跑点
        if (err != BK_OK) goto fault_handler;

        delay_ms(300);
    }
#endif
    // Step 6: 正常测试结束，执行安全下电
    TEST_LOGD("6. Standard Test Cycle Finished. Entering low-power shutdown...\r\n");
    bk_motor_set_state(1, 0);  // U4 睡眠
    bk_motor_set_state(0, 0);  // U5 睡眠
    bk_motor_boost_power(0);   // 关闭 15V 主电轨
    TEST_LOGD("=== All Stepper Motors Tested Successfully ===\r\n");
    return;

fault_handler:
    TEST_LOGE("!!! Test Aborted due to Motor fault state !!!\r\n");
    check_motor_fault(); // 运行应急保护流程
}
bk_err_t gpio_dev_unmap(gpio_id_t gpio_id);
//static beken_thread_t gpio_output_thread = NULL;
void run_dual_motor_test(void) {
    bk_err_t err;

    TEST_LOGD("=== Starting Dual Motor Independent Test ===\r\n");

    // 1. 初始化引脚与 15V
    bk_motor_gpio_config_init();
    bk_motor_boost_power(1);

    // 2. 唤醒 U4 与 U5 驱动，并打出物理电平日志
    TEST_LOGD("--- Waking up U4 (Carriage Motor) ---\r\n");
    bk_motor_set_state(1, 1); // 唤醒 U4

    TEST_LOGD("--- Waking up U5 (Paper Motor) ---\r\n");
    bk_motor_set_state(0, 1); // 唤醒 U5

    bk_motor_set_microstep(1, MODE_HALF_STEP);
    bk_motor_set_microstep(0, MODE_HALF_STEP);

    // ==========================================
    // A. 测试 U4 (字车马达)
    // ==========================================
    TEST_LOGD("[TEST A] Testing U4 Carriage Motor Forward 200 steps...\r\n");
    err = bk_motor_move_run(1, 200, 1, 3000); 
    if (err == BK_OK) {
        TEST_LOGD("[TEST A] U4 Motor Forward Passed! Testing Backward...\r\n");
        rtos_delay_milliseconds(300);
        bk_motor_move_run(1, 200, 0, 3000); 
    } else {
        TEST_LOGE("[TEST A] U4 Motor Failed!\r\n");
    }

    rtos_delay_milliseconds(500);

    // ==========================================
    // B. 测试 U5 (进纸马达)
    // ==========================================
    TEST_LOGD("[TEST B] Testing U5 Paper Motor Forward 200 steps...\r\n");
    err = bk_motor_move_run(0, 200, 1, 3000);
    if (err == BK_OK) {
        TEST_LOGD("[TEST B] U5 Motor Forward Passed! Testing Backward...\r\n");
        rtos_delay_milliseconds(300);
        bk_motor_move_run(0, 200, 0, 3000); 
    } else {
        TEST_LOGE("[TEST B] U5 Motor Failed!\r\n");
    }

    // 结束测试下电
    TEST_LOGD("Test Finished, entering sleep...\r\n");
    bk_motor_set_state(1, 0); 
    bk_motor_set_state(0, 0); 
    bk_motor_boost_power(0);  
}

static void gpio_output_config(gpio_id_t io)
{
    // 取消GPIO映射
    gpio_dev_unmap(io);
    // 配置GPIO为输出模式
    gpio_config_t io_config = {0};
    io_config.io_mode = GPIO_OUTPUT_ENABLE;
    io_config.func_mode = GPIO_SECOND_FUNC_DISABLE;
    io_config.pull_mode = GPIO_PULL_DISABLE;
    bk_gpio_set_config(io, &io_config);
    // 初始化GPIO输出低电平
    bk_gpio_set_output_low(io);
}

static void gpio_output_task(void *arg)
{
    // 配置GPIO为输出模式, 
    gpio_output_config(GPIO_13);
    // 在任务中间隔 1s 反转GPIO输出电平
    while(1)
    {
        bk_gpio_set_output_high(GPIO_13);
        TEST_LOGE("GPIO_25 output high\r\n");
        rtos_delay_milliseconds(1000);
      
      
    }
}

int mainxx(void)
{
    bk_init();
  //  media_service_init();

    int ret = BK_OK;
   // ret = rtos_create_thread(&gpio_output_thread, 6, "gpio_output_thread", gpio_output_task, 2048, NULL);
    if(ret != BK_OK)
    {
        TEST_FAIL();
        return -1;
    }
    gpio_output_config(GPIO_13);
    // 在任务中间隔 1s 反转GPIO输出电平
  //  while(1)
    {
        bk_gpio_set_output_high(GPIO_13);
        TEST_LOGE("GPIO_25 output high\r\n");
        rtos_delay_milliseconds(1000);
      
      
    }
    return 0;
}
extern void TPH_Init_LUT(void);
void u5_paper_motor_line_feed_test(void) ;
void u5_paper_motor_constant_speed_test(void);
void dwt_delay_init(void);
void run_full_printer_demo(void);
void print_50x50mm_test_pattern(void);
void print_real_icon_demo(void);
void print_magenta_vs_yellow_demo(void);

int main(void)
{
    //bk_init();
// 
 //  cli_ble_cfg_init();
 //  ble_network_cfg_start();

   if (bk_misc_get_ap_reset_reason() != RESET_SOURCE_FORCE_DEEPSLEEP)
    {
    	bk_pm_module_vote_cpu_freq(PM_DEV_ID_DEFAULT, PM_CPU_FRQ_480M);
        bk_init();
	    dwt_delay_init();	
		
	    cli_ble_cfg_init();
        ble_network_cfg_start();
		BK_LOGE(TAG, "XQJ PMXQJ 00  \n" );
		
	
    #ifdef CONFIG_LDO3V3_ENABLE
        BK_LOG_ON_ERR(gpio_dev_unmap(LDO3V3_CTRL_GPIO));
        bk_gpio_disable_pull(LDO3V3_CTRL_GPIO);
        bk_gpio_enable_output(LDO3V3_CTRL_GPIO);
        bk_gpio_set_output_high(LDO3V3_CTRL_GPIO);
    #endif
	TPH_Init_LUT();
	
	bool need_start_record = false; 
//while(1)
     {
    
            
            // 将标志位置为 true，稍后系统初始化完毕后触发录音
            need_start_record = true; 

            // 按键唤醒后的马达震动反馈
      //  #if CONFIG_MOTOR
			BK_LOGE(TAG, "XQJ PMXQJ 11  \n" );
           // motor_open(PWM_MOTOR_CH_3);
          // u5_paper_motor_line_feed_test();
         //u5_paper_motor_constant_speed_test();
         //run_dual_motor_test();
         //  tph_test_static();
       //  run_full_printer_demo();
      // print_50x50mm_test_pattern();
		 //  print_50x50mm_real_photo();
		  // print_real_icon_demo();
		   print_magenta_vs_yellow_demo();

		   //  rtos_delay_milliseconds(5000); // 震动100ms
        //    motor_close(PWM_MOTOR_CH_3);
       // #endif
    }

    #if CONFIG_LED_BLINK
        led_driver_init();
        led_app_set(LED_ON_GREEN,LED_LAST_FOREVER);
    #endif

 

    
    #if CONFIG_APP_EVT
        app_event_init();
    #endif
	

    #if CONFIG_BK_NETWORK_TRANSFER
         // 这里原来是 RTC 网络传输，后面我们要改成 HTTP/WebSocket 发送给阿里云
            // ntwk_trans_init(); 
	BK_LOGE(TAG, "22 main \n" );

	//aliyun_mqtt_init();
    #endif

    #if CONFIG_BK_SMART_CONFIG
        bk_sconf_init();
    #endif
	// ==========================================
			// ★ 核心逻辑：如果是按键唤醒的，立即调用开始录音！
	// ==========================================
	if (need_start_record) {
				BK_LOGE(TAG,"System init done. Triggering audio_engine_start_record()...\n");
				
		}

    //Notice need to wait other services to initialize
    #if CONFIG_BUTTON
        bk_key_service_init();
    #endif

    #if CONFIG_BAT_MONITOR
        extern void battery_monitor_init(void);
        battery_monitor_init();
    #endif

    #if CONFIG_USBD_MSC
        extern void msc_storage_init(void);
        msc_storage_init();
    #endif
    }
    else
    {
        bk_init();
		BK_LOGE(TAG, "11 bk_init \n" );
     
    }

 
	
     BK_LOGE(NULL, "%s, XQJ WITH_BLE \n", __func__);


	 
    return 0;
}
