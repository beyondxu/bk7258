# 当前分支模式

当前为同地址D1/D2/双选对照，见 [DOMAIN_PAIR_TEST.md](DOMAIN_PAIR_TEST.md)。
下文保留历史试验说明，不是本分支默认动作。

# 当前分支模式

当前为原青色喷嘴正反序清单对照，见 [CYAN_INVENTORY_TEST.md](CYAN_INVENTORY_TEST.md)。
下文保留历史试验说明，不是本分支默认动作。

# 当前分支模式

当前为五条长线对照，见 [LONG_LINE_CONTROL_TEST.md](LONG_LINE_CONTROL_TEST.md)。
下文保留历史试验说明，不是本分支默认动作。

# 当前分支模式

当前为分色逐喷嘴检查，见 [NOZZLE_CHECK_TEST.md](NOZZLE_CHECK_TEST.md)。
下文为历史试验说明，不是当前默认动作。

# 当前分支入口

本分支把原第5条带在同一次上电期间打印两份，中间进纸一次，见[SAME_POWER_REPEAT_TEST.md](SAME_POWER_REPEAT_TEST.md)。
下文保留原完整图说明，当前诊断模式不执行完整页。

# 实际APP完整图：每个条带上电等待2秒

分支codex/printer-full-page-settle，恢复实际完整图片路径，基于7e642a4。
只更新BK；HC32保持能出墨的3e7e161。停止固定短线替代图与示波器测试。
不调VL/VP，不缩短CE，不添加预喷，不把完整数据输出等同于纸面已完全正确。

## 默认行为

FULL_PAGE=1，PAGE_COLOR_MODE=1（CMY同一扫描），PAGE_HEAD_SETTLE_MS=2000，
FEED_STEPS=3170、方向LOW，列距3000、X方向-1、变换0。
APP收到的完整原生BIN先做CRC与语法/尺寸/边界检查，解析/缩放/映射实际像素。
不调用固定位图生成器。APP换图会实际改变打印内容；不是此前检查图模式。
仍沿用1180x1476原生图与已知1536填充布局，错误格式在运动前拒绝。
不是任意原生BIN尺寸均接受，APP已有图片转换路径输出符合该布局。

每条非空条带：头OFF→初始化马达/升压→头ON→静止等2秒（不发列、不走步）→
原速去程打印CMY图像1180列→头OFF→空白回程。之后条带间进纸3170步。
最后空白条带不扫描、不额外出纸。成功允许下一任务，执行失败锁定到重启。
没有提前伪报完成。动力/数据/电源命令失败后不盲目继续。

与实验版的区别：恢复完整图原去程扫描，不再是V/W的回程扫描或单域短线，
所以此步不是W缺线原因的单变量判定。保留2秒等待只是当前已观察较好的设置。
与7e642a4分组版相比，默认CMY同扫，避免两组各加等待导致完整照片耗时过长。
MODE=2支持代码仍保留用于历史回归，本次不手动打开。

预期TOP/END结构图通常5个非空条带：5次往返、5次进纸；
完整填满的照片可能6次往返、5次进纸，预计约65～80秒，以实机为准。
启动不自动打印。等待PAGE_DONE/PAGE_RESULT后再操作APP发送下一任务。

## 更新

```sh
cd ~/armino/bk_avdk_smp
git stash push -m before-full-page-settle -- :/sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server/ap/app_bluetooth/gatt/printer/printer_diagnostics.h
git fetch origin
git switch codex/printer-full-page-settle
git pull --ff-only
git log -1 --oneline
cd sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server
make bk7258 SDK_DIR=~/armino/bk_avdk_smp
```

不要stash pop或拷回旧诊断头文件。烧录后应见APP_FULL_PAGE_SETTLE actual_APP_image。
HC32确认3e7e161，不继续使用1.95us的4f4ef84或独立CE实验c929259。

不接示波器，放纸并恢复正常字车起点，先发送原TOP/END结构图一次，
APP保持原图、正向、不镜像、质量中。按之前方式清洁，同一喷头同类纸并记录。
这次纸面应是边框、数字1..6、斜线、TOP/END与三色色条，不再是五列青色短线。
结构图正常完成后再选择一张实际照片打印；先不连发多个任务。

源结构图：
https://raw.githubusercontent.com/beyondxu/bk7258/codex/printer-full-page-settle/sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server/tests/page-images/01-full-page-grid.png

## 日志与照片反馈

```
[PAGE_PLAN] source=1180x1476 output=1119x992 ... feed_steps=3170 ... color_mode=1 ...
[PAGE_DOTS] source_CMY=... resized_CMY=...
[PAGE_SWATH] index=1/6 ... dots_CMY=...
[PAGE_PASS] swath=1 group=1/1 mask=7 dots=...
[PAGE_HEAD_SETTLE] delay_ms=2000 head=ON carriage=STILL no_column_SPI
[PAGE_HEAD_SETTLE_DONE] begin_forward_image_scan
[CYAN_LINE_END] result=0 columns=1180/1180 forward=7480 reverse=7480 ...
[PAGE_FEED] after=1 pulses=3170/3170 result=0 ...
...
[PAGE_DONE] ...
[PAGE_RESULT] ... code=0 ...
```

CYAN_LINE只是历史函数标签，mask=7时实际传送CMY三色，不是只打印青色。
HC32 old ROW_C/M/Y只统计固定三个喷嘴，不能把它当整个条带总点数或按旧三条条形解释；
other_bits加三色固定槽dots才是该次全部有效位总数，和BK PAGE_SWATH的CMY之和对照。
请发整张图照片、图案带尺近照和两端完整日志。不要求波形；
先评估是否打印全部条带、方向/比例、接缝与颜色位置，再记录缺墨范围。
供电/喷头不稳定仍未定因，不据主机验证宣称所有喷嘴已恢复。

## 验证

16项主机回归通过，包括完整图边界/点数、同扫和分组控制器、完整BIN所有权与应答、
第二任务、失败锁定、协议接收和原运动。运动测试分别运行0ms和2000ms等待，
断言等待时头ON/升压ON、尚未运动/发送列，列数据/脉冲/加减速/故障行为保持。
未完成本环境完整SDK构建或板上完整图质量验证。
