# Timer 硬件定时器示例

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[TOC]  

## 概述
硬件Timer（定时器）是芯片内部的专用硬件模块，它利用内部时钟源计数，能够实现高精度、可编程的计时、周期性任务触发（中断）等功能。与软件实现相比，硬件Timer更精确、更可靠。

BK7258的Timer有两个定时器组Timer0和Timer1，各包含三个定时器，因此共有6个定时器通道。SDK将定时器定义为Timer通道 Channel 0 ~ 5，在CPU间划分如下。
- 对于CP： Channel 0 ~ 2, 其中通道2被默认用作喂狗的定时器；
- 对于AP： Channel 3 ~ 5

本示例基于 Timer 驱动，演示了 Timer 定时器的基本操作。它具体展示了如何：
1. 定时器驱动初始化与配置
2. 定时器启动/停止控制
3. 定时器中断处理
4. 定时器计数值读取
5. 定时器周期值获取

## 相关配置
|配置|说明|
|----|----|
|CONFIG_TIMER|使能定时器功能,配置为y|
|CONFIG_TIMER_PM_CB_SUPPORT|使能定时器低功耗回调支持|
|CONFIG_TIMER_US|Timer使能us级别的delay函数API|

## 相关代码
``` C
    #include <driver/timer.h>
    
    #define TIMER_CHAN_ID       TIMER_ID3
    #define TIMER_PRIORD_MS     500
```
### 中断处理函数

``` C
    static void timer_example_isr(timer_id_t timer_id)
    {
        timer_interrupt_count++;
        LOGI("timer_id %d interrupt count %d \r\n",  timer_id, timer_interrupt_count);
    }
```
### 定时器启动
``` C
    BK_LOG_ON_ERR(bk_timer_driver_init());
    ret = bk_timer_start(TIMER_CHAN_ID, TIMER_PRIORD_MS, timer_example_isr);
    if (ret != BK_OK) {
        LOGE("bk_timer_start failed, error code:%x\n", ret);
        TEST_FAIL();
        return;
    }
``` 

## 编译

运行以下命令进行编译构建

``` Shell
export SDK_DIR=~/armino/bk_avdk_smp //设置smp sdk路径到环境变量
cd sample_project/peripherals/timer
make clean
make bk7258
```

## 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录sample_project/peripherals/timer/build/bk7258/output/package/all-app.bin；
- 上电运行后，板子运行程序，在任务中间隔 500ms 触发一次定时器中断;
- 定时器示例不需要外部硬件连接，定时器功能完全在芯片内部实现。如果需要观察定时器中断效果，可以检查定时器中断计数是否正常增加（每500ms触发一次）
- 确认定时器在10秒后或达到20次中断后自动停止

以下是上电运行后的串口log 输出：

```
ap0:OS:D(451):create timer_example_task, tcb=607242c8, stack=[60723aa0-607242a0:2048], prio=3, xCoreID=-1
ap0:Timer_ex:I(452):rtos_create_thread success
ap1:Timer_ex:I(452):Starting timer channel 3 with period 500 ms         // 启动定时器通道3，周期为500ms
ap0:Timer_ex:I(952):timer_id 3 interrupt count 1 
ap0:Timer_ex:I(1452):timer_id 3 interrupt count 2 
ap0:Timer_ex:I(1952):timer_id 3 interrupt count 3 
ap0:Timer_ex:I(2452):timer_id 3 interrupt count 4 
ap0:Timer_ex:I(2952):timer_id 3 interrupt count 5 
ap0:Timer_ex:I(3452):timer_id 3 interrupt count 6 
ap0:Timer_ex:I(3952):timer_id 3 interrupt count 7 
ap0:Timer_ex:I(4452):timer_id 3 interrupt count 8 
ap0:Timer_ex:I(4952):timer_id 3 interrupt count 9 
ap0:Timer_ex:I(5452):timer_id 3 interrupt count 10 
ap0:Timer_ex:I(5452):Received 10 interrupts, basic timer example completed        // 定时器通道3触发了10次中断
ap0:Timer_ex:I(5452):/home/qintao.shen/work/7258_smp/sdk_sample_project/sample_project/peripherals/timer/ap/ap_main.c:70:timer_base_operate:PASS
ap0:Timer_ex:I(5453):Basic timer operations example completed
```
## 更多Timer开发指南请参考 [技术文档](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/ap_doc/bk7258/zh_CN/v3.1.1/examples/peripheral/bk_timer.html)。


## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。
