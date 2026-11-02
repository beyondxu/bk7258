# PWM 捕获模式示例

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[TOC]  

## 概述
BK7258总共有2组 PWM，每组 PWM 有3个 PWM 控制器，每个 PWM 控制器映射2路 IO 口，每1组共6个 Channel 。
1. 每组 PWM 的两两相邻通道，只能任选1路独立输出 ((PWMG0_0, PWMG0_1)相邻、(PWMG0_2, PWMG0_3)相邻、(PWMG0_4, PWMG0_5)相邻)，由于两两相邻的通道是由同一个PWM控制器输出的，同时输出会相互干扰导致波形不正确。
2. 由于两两相邻的通道是由同一个PWM控制器输出的，PWMG0 的 CH0, CH2, CH4 与 PWMG1 的 CH0, CH2, CH4 任意两通道组成互补输出，软件实现互补功能。
3. 另外要应用到灯或电机时，P8/P22，P9/P23 不建议使用, 这几个引脚在上电过程中受到 bootrom 的控制，会对 io 进行操作，导致在上电过程中电机转动一下或者灯闪烁一下。

本示例基于 PWM 组件，演示了 PWM 捕获模式的实现。它具体展示了如何：
1. 配置 PWM 为周期输出，设置输出通道为 PWMG0_0 频率为10KHz，占空比为 40%；
2. 配置 PWM 为捕获模式，设置捕获通道为 PWMG0_2；
3. 在任务中每隔1s 执行一次 PWM 捕获，读取捕获信息并打印；

## PWM通道
| PWMG0 IO |通道号 |
| --------- |  ------- |
|GPIO_6/GPIO_18  |GPIO_DEV_PWM0|
|GPIO_7/GPIO_19  |GPIO_DEV_PWM1|
|GPIO_8/GPIO_22 |GPIO_DEV_PWM2|
|GPIO_9/GPIO_23  |GPIO_DEV_PWM3|
|GPIO_24 |GPIO_DEV_PWM4|
|GPIO_25 |GPIO_DEV_PWM5|

| PWMG1 IO |通道号 |
| --------- |  ------- |
|GPIO_32 |GPIO_DEV_PWM6|
|GPIO_33 |GPIO_DEV_PWM7|
|GPIO_34 |GPIO_DEV_PWM8|
|GPIO_35 |GPIO_DEV_PWM9|
|GPIO_36 |GPIO_DEV_PWM10|
|GPIO_37 |GPIO_DEV_PWM11|

## PWM 配置
| 配置名                    | 说明                               |
| --------- |  ------- |
|CONFIG_PWM| pwm使能，配置为y|
|CONFIG_PWM_V1PX| pwm v1驱动使能，配置为y|
|CONFIG_PWM_USE_DEFAULT_GPIO_MAP| pwm是否使用默认SDK GPIO映射,默认配置y使用映射|

>**注意**：CONFIG_PWM_USE_DEFAULT_GPIO_MAP 宏配置在SDK版本v3.1.1.7后新增配置，不使用默认映射时，需在 usr_gpio_cfg.h 中自定义配置 pwm 通道和IO映射关系。

## 相关代码

``` C
    // PWM 输出
    pwm_init_config_t pwm_config = {0};
    pwm_config.period_cycle = PERIOD_CYCLE;
    pwm_config.psc = PSC;
    pwm_config.duty_cycle = DUTY_CNT;
    pwm_config.duty2_cycle = 0;
    pwm_config.duty3_cycle = 0;
    // 初始化PWM通道
    bk_pwm_init(chan_id, &pwm_config);
    // 设置PWM初始信号为低电平
    bk_pwm_set_init_signal_low(chan_id);
    // 启动PWM输出
    bk_pwm_start(chan_id);

    // PWM 捕获
    pwm_capture_init_config_t capture_config = {0};
    capture_config.edge = PWM_CAPTURE_NEG;              // 下降沿捕获
    capture_config.isr = NULL;
    // 初始化并启动PWM捕获通道
    bk_pwm_capture_init(cap_chan_id, &capture_config);
    bk_pwm_capture_start(cap_chan_id);

    // 获取两次下降沿的周期数 计算出频率，
    uint32_t period_cycle = bk_pwm_capture_get_period_duty_cycle(cap_chan_id, 1000);
    LOGI("pwm_capture period_cycle:%d, freq:%dHz\n", period_cycle, _PERIOD_2_FREQ(period_cycle));
    // 停止并反初始化PWM捕获通道
    bk_pwm_capture_stop(cap_chan_id);
    bk_pwm_capture_deinit(cap_chan_id);

    capture_config.edge = PWM_CAPTURE_EDGE;             // 双边沿捕获
    capture_config.isr = NULL;
    bk_pwm_capture_init(cap_chan_id, &capture_config);
    bk_pwm_capture_start(cap_chan_id);

    // 获取到两个边沿的周期数 计算出占空比， 统计的高电平段
    uint32_t duty_cycle = bk_pwm_capture_get_period_duty_cycle(cap_chan_id, 1000);
    float duty_ratio = (period_cycle == 0) ? 0 : (float)duty_cycle / (float)period_cycle;
    LOGI("pwm_capture duty_cycle:%d, duty_ratio:%f\r\n", duty_cycle, duty_ratio);
    bk_pwm_capture_stop(cap_chan_id);
    bk_pwm_capture_deinit(cap_chan_id);
     
```

## 编译

运行以下命令进行编译构建

``` Shell
export SDK_DIR=~/armino/bk_avdk_smp  //设置smp sdk路径到环境变量
cd sample_project/peripherals/pwm/capture_mode
make clean
make bk7258
```

## 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录sample_project/peripherals/pwm/capture_mode/build/bk7258/capture_mode/package/all-app.bin；
- 上电运行后，板子运行程序，使用杜邦线连接GPIO_18和 GPIO_8，将pwm捕获通道与输出通道连接起来，在任务中间隔 1s 读取一次 PWM 捕获值，打印出 PWM 捕获值;

以下是上电运行后的串口log 输出：

```
ap0:hal:W(470):gpio: 18 is used.Please confirm unmap isn't impact is working module.!
ap0:pwm_capt:I(470):pwm output start success, Channel: 0                                            // PWM输出通道0启动成功
ap0:pwm_capt:I(471):pwm output frequency: 10000Hz, duty: 40%                                        // PWM输出通道0频率10KHz，占空比40%
ap0:OS:D(471):create pwm_capture_task, tcb=607242c8, stack=[60723aa0-607242a0:2048], prio=3, xCoreID=-1     // 创建PWM捕获任务
ap1:pwm_capt:I(472):pwm capture task chan_id: 2                                                             // PWM捕获通道2启动成功
ap0:pwm_capt:I(472):/home/qintao.shen/work/7258_smp/sdk_sample_project/sample_project/peripherals/pwm/capture_mode/ap/ap_main.c:172:main:PASS

◆ap0:pwm_capt:I(1472):pwm_capture period_cycle:0, freq:0Hz
ap0:hal:W(1473):gpio: 8 is used.Please confirm unmap isn't impact is working module.!
ap0:pwm:W(1473):pwm_capture, period_cycle:0
ap0:pwm_capt:I(1473):pwm_capture duty_cycle:0, duty_ratio:0.000000
ap0:hal:W(4475):gpio: 8 is used.Please confirm unmap isn't impact is working module.!

[11:37:15.310]收←◆ap0:hal:W(10147):gpio: 8 is used.Please confirm unmap isn't impact is working module.!
ap0:pwm_capt:I(10152):pwm_capture period_cycle:2600, freq:10000Hz                                // PWM捕获通道周期数2600，频率10KHz
ap0:hal:W(10153):gpio: 8 is used.Please confirm unmap isn't impact is working module.!
ap0:pwm_capt:I(10169):pwm_capture duty_cycle:1040, duty_ratio:0.400000                           // PWM捕获通道有效电平周期数1040，占空比 1040/2600 = 40%
```
## 更多PWM开发指南请参考 [技术文档](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/ap_doc/bk7258/zh_CN/v3.1.1/examples/peripheral/bk_pwm.html)。


## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。
