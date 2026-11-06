# 当前分支入口

本分支为固定地址逐段波形对照，实际4条青色线、0进纸，见[SECTION_SCOPE_TEST.md](SECTION_SCOPE_TEST.md)。
下文仅保留历史版本说明。

# 喷嘴检查：两批交错输出，原速运动

分支codex/printer-nozzle-batches。只更新BK7258；HC32保持能出墨的3e7e161。
不使用之前撤回的独立CE方案，不延长加热脉冲，不调电压。

## 改变的是什么

比较对象为14cd8b1原速检查图；本版恢复字车原速（TIME_SCALE=1），
每色改成两次往返。第一批打印每列第1、3、5..31条短线，第二批补第2、4..32条。
同一颜色两批之间不走纸，每个原始点仍仅在其所属批次发一次。
每色仍192条、每条96点；最终仍是原六列短线图，不是APP的TOP/END图片。

位图检查：两批按位或=原始图、按位与=0，整页总点数不变55296。
每批每色9216点，最多每列16点；原图为18432点/色、最多每列32点。
按实际地址/域数据分布检查，每地址最多1个有效bit，原图最多2个。
这些是数据计划的上限，不是测得的电流/墨滴或保证出墨恢复。

PRINTER_NOZZLE_BATCHES=2；PRINTER_NOZZLE_TIME_SCALE=1。
初始化和原3000→260us加减速、4us STEP高电平、7480步行程保持。
全任务6次往返、2次进纸3170步；预计约50秒，具体时间以实机日志为准。
相同颜色两次往返需机械重复定位，若线条错位须记录，不能当作喷嘴恢复。

## 下载与打印

```sh
cd ~/armino/bk_avdk_smp
git stash push -m before-nozzle-batches -- :/sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server/ap/app_bluetooth/gatt/printer/printer_diagnostics.h
git fetch origin
git switch codex/printer-nozzle-batches
git pull --ff-only
git log -1 --oneline
cd sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server
make bk7258 SDK_DIR=~/armino/bk_avdk_smp
```

烧录后不stash pop。使用原A/B相同喷头及纸种，按相同方式擦拭，尽快发送
原TOP/END图片一次作为任务触发。本模式生成检查图，APP预览与纸面不同是预期。
先只打印一张，标为P（分批），记录擦拭至打印间隔。等PAGE_RESULT完成再取纸。
颜色间两次进纸，两批之间无进纸，打印比原速单批多3次往返属于预期。

```
[NOZZLE_LOAD] forward_motor_delay_scale=1 start_low_us=3000 cruise_low_us=260 ...
[NOZZLE_BATCH] channel=0 batch=1/2 expected_dots=9216 no_feed_between_batches motor_scale=1
[CYAN_LINE_END] result=0 columns=1180/1180 ...
[NOZZLE_BATCH] channel=0 batch=2/2 expected_dots=9216 no_feed_between_batches motor_scale=1
... channel=0两批完成后进纸；channel=1两批完成后进纸；channel=2两批完成 ...
[NOZZLE_LINES_END] code=0 locked=0 ink_unverified
[PAGE_RESULT] ... code=0 ...
```

HC32每批仍1180列，other_bits加固定槽dots总和应9216，不再是18432。
同色两批合计18432。一次批次故障立即停止，后续批次/进纸不执行并锁到复位。
成功后可再收任务，不自动重试或提前伪报完成。

发P的整张与三色近照及两端日志。与原速A/B、慢速L相同位置短线比较：
明显恢复支持与每次驱动负载相关的供电/公共驱动/供墨等因素，不能单独证明电压不足；
固定行仍缺失则保留喷嘴局部与公共分段问题。若实际出墨仍差，不据此加长CE。
本版与慢速L比较同时涉及恢复原速，主要单变量参照是原速A/B；不同清洁状态仍是限制。

## 软件验证

18个主机脚本通过。实际C生成器的两批重组无丢点/重叠，验证每色两批9216点，
每列/每地址有效位上限；实际调度器验证先两批后进纸、第二批失败不进纸、
进纸失败停止、内存/输入失败不运动、完成后允许下一任务。
便携映射模块通过M33对象编译。本环境未完成完整BK SDK构建或板上出墨验证。
检查结束可回codex/printer-full-page-groups/7e642a4恢复完整图，HC32不变。
