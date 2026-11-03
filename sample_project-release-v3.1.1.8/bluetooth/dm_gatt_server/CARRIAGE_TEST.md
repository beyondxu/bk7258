# 字车往返：复现用户已验证的梯形曲线

分支 `codex/printer-carriage-test`，仅更新 BK7258；HC32 保持 `codex/printer-bounded-line` / `f87a450`。
这一版保留用户梯形曲线，并将过快的 NOP 忙循环改为 SDK 硬件定时器延时；未实机验证。

## 更新与编译

```sh
cd ~/armino/bk_avdk_smp
git stash push -m before-shuttle-update -- sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server/ap/app_bluetooth/gatt/printer/printer_diagnostics.h
git fetch origin
git switch codex/printer-carriage-test
git pull --ff-only
cd sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server
make bk7258 SDK_DIR=~/armino/bk_avdk_smp
```

不要 stash pop。保持默认 PRINTER_CARRIAGE_TEST=1、PRINTER_BOUNDED_PRINT=0，不需要改参数。
两个 MCU 断电前把字车恢复到之前正常打印的起始位置（照片箭头起点），烧录后一起重新启动。
不用 APP、不需要纸张。启动握手成功后，只发送喷头 OFF 指令，再让字车自动往返一次。
纸张电机保持初始化的禁用状态，全程不发走纸脉冲；不发送喷墨列。

## 与用户代码的对应

- 单程 7480 脉冲，MS1/MS2/MS3=111。
- 只初始化电机 GPIO，升压后等待 60ms，SLEEP 拉高等待 5ms，RES 拉高，EN 拉低后等待 10ms。
- 方向建立 delay_us(50)，STEP 高 delay_us(4)，低电平梯形参数 1000→260→1000，斜坡各 250 步。
- 前进后暂停 200ms，返回后暂停 300ms，关闭字车 EN/SLEEP/RES 和升压。
- motor_sdk_driver.c 的 delay_us() 现调用 bk_delay_us()，项目 CONFIG_TIMER_US=y 时走 SDK 的 26MHz TIMER0 延时。
  编译时强制要求 CONFIG_TIMER_US，防止落入该 SDK 中空的 arch_delay_us() 回退实现。
  不再用 us*2 次 nop 冒充微秒。单程请求延时总和约 2.161 秒，实机还包含 GPIO、函数调用及中断开销。
- 步进循环不插入 SPI、GPIO 输入读取、打印日志或 RTOS 调度调用；中断保持正常状态。

此前测试发出 96 脉冲后读到 GPIO48=0，软件因此停止；尚未确认它是真实驱动故障还是读取/配置问题。
本次为了对比已验证运动代码，仅在每程前后记录 nFAULT，日志标记 MONITOR_ONLY，不因它提前停止。
没有修改电机驱动芯片硬件保护；这也不意味着低电平已经被证明无害。
如仍卡住或碰到边界，立即断电，不反复执行。代码没有位置反馈或自动寻零。

## 反馈与验证

提供 `[CARRIAGE_TEST]` 和 `[CARRIAGE_NFAULT]` 全部日志，以及前进/返回是否平稳、是否回到起始位置。
日志应显示 timing=USER_RAMP_SDK_TIMER，两程 pulses=7480；
每程新增 [CARRIAGE_TIME] elapsed_us=...，由 RTC 独立测量整程耗时。
预计约 2.2 秒或更长，不能仍只有 0.204 秒；不要要求与理想值完全相等。
计数与时间仍不能证明物理运动完成。

用户最新日志显示每程约 204ms，nFAULT 两程前后均为高，说明此前名义延时远短于微秒参数。
主机测试逐个检查两程全部延时参数、delay_us 对 SDK 的转发、理论耗时、脉冲数量、单次往返、
喷头只关不打开、OFF 握手失败不运动。
完整 BK SDK 编译及实际运动仍需本地验证。先验证纯运动，再单独加入空白 SPI 列，最后恢复青色打印，
以区分纯步进问题和 SPI/喷墨与步进交错造成的节奏问题。
