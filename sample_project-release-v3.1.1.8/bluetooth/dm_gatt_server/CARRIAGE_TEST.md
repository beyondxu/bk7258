# 字车独立往返测试

分支 `codex/printer-carriage-test`。只更新 BK7258；HC32 保持配套 `codex/printer-bounded-line` / `f87a450`。
本版用于排查短距离、抖动、卡顿；不代表已确认根因或修好实际运动。

## 更新

在 Ubuntu SDK 根目录执行，先保存本地的 PRINT=1 配置，不要 stash pop：

```sh
cd ~/armino/bk_avdk_smp
git stash push -m before-carriage-test -- sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server/ap/app_bluetooth/gatt/printer/printer_diagnostics.h
git fetch origin
git switch codex/printer-carriage-test
git pull --ff-only
cd sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server
make bk7258 SDK_DIR=~/armino/bk_avdk_smp
```

默认 `PRINTER_CARRIAGE_TEST=1`、`PRINTER_BOUNDED_PRINT=0`，不需要改参数。
烧录后把字车恢复到之前打印用的起始位置（照片箭头的起点），它没有自动寻零。
两个 MCU 断电重启，不需要 APP 或纸张；启动握手成功后自动执行一次往返。
只发送喷头 OFF 指令，不发送任何喷墨列；纸张电机关闭，不发走纸脉冲。
如果启动自检/喷头 OFF 握手不通过，不运行电机。

## 运动与日志

保留原有 MS=111 细分和单程 7480 步。前后行程使用相同 RTC 计时引擎，
STEP 高电平至少 40us，低电平从 2500us 缓慢降到 1500us，再回到 2500us。
保持中断开启，每 16 步让出一次调度。单程约 12～15 秒或更久，停止 0.5 秒后返回。

检查日志 `[CARRIAGE_TEST]` BEGIN、FORWARD、REVERSE、END。
正常软件结果应为两程 pulses=7480、result=0。
每步检查 U4 nFAULT：若为低，立即停机、下电，不执行整段盲目返回。
RTC 无法计时时有有限循环保护，避免永久阻塞。

这不是位置反馈控制：脉冲计数不能证明实际行程，堵转也不一定触发 nFAULT。
观察是否平稳扫过原有行程并返回；如果仍卡住或撞到边界，立即断电，不继续反复试。
反馈两个 MCU 的启动日志，以及实际运动情况（正向/返回各自是否平稳、是否回到原位）。

## 已做验证与限制

主机模拟运行实际脉冲循环及测试控制器，验证步数、方向、RTC 最小脉宽、故障停机、
时钟停滞退出、喷头 OFF 握手失败不运动、失败不盲目返回。原有协议和映射测试通过。
未完成完整 BK SDK 固件编译，未实机验证。RTC API 已存在于该 SDK，项目配置启用 AON_RTC。
不更改 HC32 波形或照片映射，本版先把运动问题单独验证。
