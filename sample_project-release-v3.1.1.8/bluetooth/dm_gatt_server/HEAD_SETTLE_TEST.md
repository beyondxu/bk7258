# 回程喷墨前：喷头上电后静置2秒

分支codex/printer-head-settle，基于fde6b00回程V版。只更新BK，HC32仍3e7e161。
本次只增加上电后的等待，不改变点阵、喷嘴、扫描方向、速度、步数或CE/S时序。

## 软件顺序

每段：头OFF → 空白去程7480步 → 右端等待200ms → 头ON →
静置2000ms（不发列SPI、不发送STEP）→ 回程1180列喷墨 → 头OFF → 结束。
原回程列1179..0、步索引206..7280、各7480步均保持，纸面仍是V/R同一五列青色短线。
静置期间不发送喷墨命令；这是等电源/状态建立，不是预喷或延长CE。
四段各等2秒，总时长由原约26秒增加到约34秒，4次往返、0进纸。
不在固件等待时操作APP重复发任务。

## 更新

```sh
cd ~/armino/bk_avdk_smp
git stash push -m before-head-settle -- :/sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server/ap/app_bluetooth/gatt/printer/printer_diagnostics.h
git fetch origin
git switch codex/printer-head-settle
git pull --ff-only
git log -1 --oneline
cd sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server
make bk7258 SDK_DIR=~/armino/bk_avdk_smp
```

烧录后不stash pop。默认PRINTER_HEAD_SETTLE_MS=2000。
同一喷头、纸种、擦拭方式与纸张朝向，记录擦拭至打印间隔。
APP发送原TOP/END图一次作为触发，实际仍打印固定青色五列短线。
只打印一张标W（等待），与现有V同列同高度比较，不要求示波器。
不要重新清洗/更换喷头后把差异全部归因于这2秒；清洁状态本身仍会影响对照。

```
[RETURN_LINE_BEGIN] forward=blank_head_OFF reverse=ink ...
[HEAD_SETTLE] delay_ms=2000 head=ON carriage=STILL no_column_SPI
... 约2秒 ...
[HEAD_SETTLE_DONE] begin_return_scan
[RETURN_LINE_END] result=0 columns=1180/1180 forward=7480 reverse=7480 ...
... 共四段 ...
[SECTION_PAPER_END] code=0 locked=0 ...
[PAGE_RESULT] ... code=0 ...
```

每段HC32总点数仍2400，不能用ROW_C=0判断缺青色数据。
发W照片与两端日志，重点看V原右侧起始端的短线是否恢复。
改善支持上电建立/等待相关因素，不单独证明电压不足；无明显改善说明单纯静置不足，
不据此直接延长加热脉冲或断言喷嘴损坏。未验证前不添加废墨区外预喷。

## 验证范围

22项主机回归通过；回程测试同时覆盖0ms旧模式和2000ms新模式。
等待发生时断言头ON、字车位于右端、尚未发第一列；上电命令失败则不等待/不喷墨，
原逐列物理步坐标、总步数及异常停止检查继续通过。
回程模块在模拟接口下通过M33对象编译，未完成完整BK SDK构建或板上实测。
