# 纯字车基准：跳过 SPI / HC32 握手 / BLE 打印服务

分支 `codex/printer-carriage-test`。仅更新 BK7258，HC32 保持已烧录的
`codex/printer-bounded-line` / `f87a450`（该版没有控制命令就不开喷头电源）。

上次 nFAULT=IGNORED 版本两程各约 2.186s，实际仍不动，移除 nFAULT 没有解决问题。
本次先复现用户已验证的独立运行条件，不再推测电机损坏，也不声称已修好。

## 更新

```sh
cd ~/armino/bk_avdk_smp
git stash push -m before-pure-carriage-reference -- sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server/ap/app_bluetooth/gatt/printer/printer_diagnostics.h
git fetch origin
git switch codex/printer-carriage-test
git pull --ff-only
cd sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server
make bk7258 SDK_DIR=~/armino/bk_avdk_smp
```

不要 stash pop。默认 PRINTER_CARRIAGE_TEST=1、PRINTER_BOUNDED_PRINT=0、
PRINTER_CARRIAGE_REFERENCE_DELAY=1。不需要改参数。
把字车恢复到已验证行程的起点，烧录后两个 MCU 断电重新启动，不需要 APP 或纸。

## 本次运行路径

应用任务进入后，在任何 SPI 初始化/自检之前调用独立往返；完成后退出初始化线程，
不启动 BLE 打印服务或打印后台任务。系统底层蓝牙初始化仍由 SDK 启动流程负责。
不调用 HC32 通信、不发送任何喷墨列、不读取 nFAULT、不输出走纸脉冲。
HC32 停留在 Waiting for BK7258 的自检等待状态是本次的正常现象。

电机初始化按用户代码：GPIO 初始化，升压后等 60ms，细分 111，SLEEP 拉高等5ms，
RESET 拉高，ENABLE 拉低等10ms；前行7480步，等200ms，返回7480步，等300ms后下电。
每程250步加减速，延时参数1000→260→1000，STEP高电平参数4，方向建立参数50。

默认 delay_mode=1 使用测试局部的原始 us*2 次 nop 忙循环。它不是真实微秒，速度可能很快，
本次只往返一次，目的是复现原代码而不是标定速度。没有位置反馈或自动寻零；
如果出现卡顿或碰到边界立即断电，不反复运行。

日志开头应有 `[CARRIAGE_REFERENCE] standalone before_SPI before_HC32 before_BLE_printer`。
随后 `[CARRIAGE_TEST] ... delay_mode=1 ... nFAULT=IGNORED ... no_SPI`，两程脉冲和耗时。
本次不应出现启动 SPI 回环自检、BLE ADV ENABLED 或等待APP打印的后续流程。

## 之后的单变量对比

先反馈默认模式的日志、是否实际运动、是否回到起始位置。
如果默认模式已能平稳往返，可只把诊断头文件 PRINTER_CARRIAGE_REFERENCE_DELAY 改为2，
重新编译烧录。模式2保持同一个纯运动启动路径，只把局部延时改为 SDK 硬件定时器；
预计单程约2.2秒或更长。不要同时恢复SPI/打印。
默认模式失败时先停在基准对比，不继续盲目加回其他功能。

## 验证边界

主机测试检查全部7480步梯形参数、单次往返、不读取nFAULT、无任何HC32控制调用、
两种实际延时分支、启动测试位于SPI初始化前。完整固件仍需本地编译、实机验证。
原有协议、解析和喷嘴映射未修改；此分支当前不执行这些服务。
