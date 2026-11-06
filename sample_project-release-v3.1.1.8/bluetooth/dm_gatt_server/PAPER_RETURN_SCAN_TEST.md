# 回程喷墨对照：相同R图，相反扫描方向

分支codex/printer-paper-return-scan，基于c1630c5的R图版。仅更新BK。
HC32固定3e7e161，不改CE/S时序或电压，不要求示波器。

## 纸面和动作

实际仍是R版青色D2、100喷嘴、S1..S4各25个地址的五列短线。
不再次改变列分组：R与新纸V应按同列同高度比较，不能再套T到R的反排表。
每喷嘴96点、每段2400点、4次往返、不进纸、原3000→260us曲线与4us STEP高电平。
唯一主要对照为喷墨发生于相反运动方向：

1. 关喷头电源，空白去程7480步，不发送列数据。
2. 右端等待200ms，开启喷头电源，回程喷墨7480步。
3. 关喷头电源，等待300ms，关字车驱动及升压电源。

失败即关断，后续段不执行，锁定到重启；不盲目补走或归位。
成功允许下一次明确APP任务，不自动重试。
仍4次往返，预计约26秒，实际以日志为准。去程完全没有墨属于预期。
这不是把马达方向引脚随意颠倒，未增加行程。

## 列的位置为何不镜像

旧去程在i=200+6*k处、STEP脉冲之前发送列k。
去程完成后理论位置7480。回程以i=206+6*j、源列k=1179-j发送，
理论位置7480-i=7274-6*j=200+6*k，逐列相同。
回程有效步索引206..7280（含端点），正好1180列，之后回到理论起点0。
不是简单在回程i=200..7274直接反排，会产生6步偏移。
以上是脉冲坐标相同，不是物理位置反馈：反向间隙、失步、喷墨飞行偏移需实测。
若整图位移或错位，先记录，不改步数补偿混入本次对照。

## 更新

```sh
cd ~/armino/bk_avdk_smp
git stash push -m before-paper-return -- :/sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server/ap/app_bluetooth/gatt/printer/printer_diagnostics.h
git fetch origin
git switch codex/printer-paper-return-scan
git pull --ff-only
git log -1 --oneline
cd sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server
make bk7258 SDK_DIR=~/armino/bk_avdk_smp
```

烧录后不stash pop。默认SECTION_PAPER=1、SECTION_SCOPE=0、RETURN_SCAN=1、
REVERSE_COLUMNS=1、TIME_SCALE=1。不要改APP镜像或旋转来补偿。
使用与R相同喷头、纸种、擦拭方式和纸面朝向，记录擦拭至打印间隔。
APP发送原TOP/END图一次作为触发，仍只生成固定青色短线，不打印预览图片。
打印一张标V（回程），保留R，发两端日志及V带尺近照；先不重复打印。

```
[SECTION_PAPER_ORDER] reverse_columns=1 expected_column_mapping=5,4,3,2,1 ...
[SECTION_PAPER_SCAN] ink_on_return=1 same_R_chart_same_nozzles ...
[RETURN_LINE_BEGIN] forward=blank_head_OFF reverse=ink source_columns=1179..0 reverse_fire_steps=206..7280 total_steps_each=7480 feed=0
[RETURN_LINE_END] result=0 columns=1180/1180 forward=7480 reverse=7480 ...
... 共四段 ...
[SECTION_PAPER_END] code=0 locked=0 ...
[PAGE_RESULT] ... code=0 ...
```

每段HC32仍1180列，other_bits加固定统计总和2400。
收到正确数据并不证明喷墨成功或物理重复定位准确。

## 如何判断

若R纸面左侧（原去程开始侧）较差，V转为右侧（回程开始侧）较差，
支持与扫描开始/时间过程相关的驱动建立、供墨等因素。
若仍机器同一物理侧较差，保留位置相关FPC接触、纸面间隙等问题。
若同一喷嘴的缺失始终不变，保留固定喷嘴/地址路径问题。
清洁、等待时间及头状态仍影响比较，不用单张V证明唯一原因或决定延长CE。

## 验证范围

22项主机脚本通过。实际回程代码逐列验证源1179..0、物理坐标与去程完全对应、
各7480步、加减速序列不变、去程头OFF、回程头ON、无额外SPI或进纸；
SPI/上电/下电失败停止且不盲走。原去程实现保留，原运动回归继续通过。
纸面控制器分别覆盖去程/回程选择路径与失败锁定。
回程模块在模拟接口下通过M33对象编译；本环境未完成完整BK SDK构建或实际回程喷墨/机械定位验证。
需要回完整图时切回codex/printer-full-page-groups/7e642a4并重新编译烧录BK，HC32不改。
