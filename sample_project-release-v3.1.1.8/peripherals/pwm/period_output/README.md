# PWM 周期输出示例

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[TOC]  

## 概述
BK7258总共有2组 PWM，每组 PWM 有3个 PWM 控制器，每个 PWM 控制器映射2路 IO 口，每1组共6个 Channel 。
1. 每组 PWM 的两两相邻通道，只能任选1路独立输出 ((PWMG0_0, PWMG0_1)相邻、(PWMG0_2, PWMG0_3)相邻、(PWMG0_4, PWMG0_5)相邻)，由于两两相邻的通道是由同一个PWM控制器输出的，同时输出会相互干扰导致波形不正确。
2. 由于两两相邻的通道是由同一个PWM控制器输出的，PWMG0 的 CH0, CH2, CH4 与 PWMG1 的 CH0, CH2, CH4 任意两通道组成互补输出，软件实现互补功能。
3. 另外要应用到灯或电机时，P8/P22，P9/P23 不建议使用, 这几个引脚在上电过程中受到 bootrom 的控制，会对 io 进行操作，导致在上电过程中电机转动一下或者灯闪烁一下。

本示例基于 PWM 组件，演示了 PWM 周期输出的实现。它具体展示了如何：
1. 配置 PWM 为周期输出，设置输出通道为 CH0 (GPIO_18)频率为10KHz，占空比为 40%；

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

## PWM 相关配置
| 配置名                    | 说明                               |
| --------- |  ------- |
|CONFIG_PWM| pwm使能，配置为y|
|CONFIG_PWM_V1PX| pwm v1驱动使能，配置为y|
|CONFIG_PWM_USE_DEFAULT_GPIO_MAP| pwm是否使用默认SDK GPIO映射,默认配置y使用映射|

>**注意**：CONFIG_PWM_USE_DEFAULT_GPIO_MAP 宏配置在SDK版本v3.1.1.7后新增配置，不使用默认映射时，需在 usr_gpio_cfg.h 中自定义配置 pwm 通道和IO映射关系。


## 相关代码

``` C
    #include <driver/pwm.h>
    // 初始化PWM驱动
    bk_pwm_driver_init();
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
     
```

## 编译

运行以下命令进行编译构建

``` Shell
export SDK_DIR=~/armino/bk_avdk_smp  //设置smp sdk路径到环境变量
cd sample_project/peripherals/pwm/period_output
make clean
make bk7258
```

## 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录sample_project/peripherals/pwm/period_output/build/bk7258/period_output/package/all-app.bin；
- 上电运行后，板子运行程序，在pwm ch0(GPIO_18) 输出10KHz 频率，占空比为 40% 的方波, 使用示波器或逻辑分析仪查看输出信号。

以下是上电运行后的串口log 输出：

```  
ap0:hal:W(487):gpio: 18 is used.Please confirm unmap isn't impact is working module.!
ap0:pwm_outp:I(488):pwm output start success, Channel: 0                            // 配置PWM输出通道为CH0，频率为10KHz，占空比为40%
ap0:pwm_outp:I(488):pwm output frequency: 10000Hz, duty: 40%
ap0:pwm_outp:I(488):/home/qintao.shen/work/7258_smp/sdk_sample_project/sample_project/peripherals/pwm/period_output/ap/ap_main.c:79:main:PASS

```
## 更多PWM开发指南请参考 [技术文档](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/ap_doc/bk7258/zh_CN/v3.1.1/examples/peripheral/bk_pwm.html)。


## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。
