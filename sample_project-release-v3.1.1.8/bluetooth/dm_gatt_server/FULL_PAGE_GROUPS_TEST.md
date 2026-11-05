# 完整图分组扫描对照：青色单独，品红+黄色随后

分支 `codex/printer-full-page-groups`，从完整图及3170步校准版 `f2eef83` 建立，
不基于微小喷嘴覆盖图分支。覆盖图数据完整但没有可见墨迹，未获得可靠覆盖结论，暂不重复。
本次直接继续完整图，排除同次多色驱动对青色的干扰；不是声称已修复喷头或证明电源原因。

## 变化

每个条带仍先完整解析/等比缩放/映射全部CMY，原100字节列点阵不变。
PRINTER_PAGE_COLOR_MODE=2时，依次执行：

1. 青色组：只保留D1/D2，mask=1，打印并返回。
2. 品红+黄色组：只保留D3..D6，mask=6，打印并返回。
3. 两组都完成后才进纸一次3170步。

两组之间不进纸。纸面原有每个bit只在对应组发一次，不重复加热同一个点，不增加电压/脉宽，
不改HC32波形。单次峰值颜色组合减少，但总墨点保持一致；机械重复定位和供墨仍需实测。
MODE=1保留原来的CMY同一扫描作为对照，不要同时调整位置或步数。
空组跳过，空白条带也不扫。每页仍5次进纸，但最多12次往返，速度比原来慢；
当前结构图通常5个非空条带，即最多10次往返。不是新增了页或自动重复同一任务。
成功后允许下一页，任一组失败仍锁定到重启，不盲目进纸或返回。

## 更新与测试

只更新BK7258，HC32保持当前f5d53d4，不切换其他驱动。

```sh
cd ~/armino/bk_avdk_smp
git stash push -m before-full-page-groups -- :/sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server/ap/app_bluetooth/gatt/printer/printer_diagnostics.h
git fetch origin
git switch codex/printer-full-page-groups
git pull --ff-only
git log -1 --oneline
cd sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server
make bk7258 SDK_DIR=~/armino/bk_avdk_smp
```

保持默认参数，不要stash pop。默认FULL_PAGE=1、PAGE_COLOR_MODE=2、FEED_STEPS=3170、
列距3000、方向-1、变换0。无需改CHANNEL_MASK，它对完整图路径不起选色作用。
换纸、恢复字车起点、两端断电重启，APP发送同一张完整结构图，质量中、正向、不镜像。
保持当前喷头状态作对照；如果需要清洁，要记录，避免把清洁效果误认为分组修改的效果。

结构图：
https://raw.githubusercontent.com/beyondxu/bk7258/codex/printer-full-page-groups/sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server/tests/page-images/01-full-page-grid.png

预计耗时比原版增加，等到PAGE_DONE及APP完成后再取纸；若APP报超时而仍在运行，
不要重复发任务，保留日志以确认实际总时长。代码不会提前伪报完成。
卡顿或撞到边界立即断电，不推通电字车。

## 日志

```text
[PAGE_PLAN] ... feed_steps=3170 ... color_mode=2 ...
[PAGE_PASS] swath=1 group=1/2 mask=1 dots=... no_feed_between_passes
[CYAN_LINE_END] ...
[PAGE_PASS] swath=1 group=2/2 mask=6 dots=... no_feed_between_passes
[CYAN_LINE_END] ...
[PAGE_FEED] after=1 pulses=3170/3170 ...
...
[PAGE_DONE] ...
```

每次HC32的统计会按每个组分别重置，青色组的M/Y固定槽计数应为0，
第二组C固定槽计数应为0；ROW固定槽并非整图总点数，bars/outside不按旧三段图解释。

请提供整张结构图照片和两个MCU完整日志，重点看青色竖框/斜线是否恢复、
颜色边缘是否因多次返回而错位、条带接缝是否仍有整条空缺。走纸40..42mm先保持。
此步是完整图输出调度对照，不继续喷嘴微笔/防堵实验；结果不能单独证明所有喷嘴正常。

## 验证

C过滤两组重组后逐字节等于原图，各组无其他域位、输入不被修改，非法源bit/重叠缓冲拒绝。
主机直接执行实际页面控制器，验证单组/两组的扫描次数、两组完成再进纸、成功后新页、
第二组失败不进纸且锁定、内存失败无运动；原完整图及协议/运动回归通过。
可移植模块通过M33对象编译；完整SDK编译、耗时、真实出墨与机械复位精度仍需本地验证。
