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

当前为两份连续上电对照，见 [SAME_POWER_REPEAT_TEST.md](SAME_POWER_REPEAT_TEST.md)。
下文是先前单份隔离试验记录。

# 原第5条带作为第一次物理扫描，不进纸

分支codex/printer-isolate-bottom-swath，基于当前完整图版本，只更新BK。
HC32保持3e7e161；不改变电压、CE、2秒等待、原速去程打印或喷嘴映射。
用户已补充原F2下横线并非全无出墨，而是后段偏红：本试验比较其作为首扫描时能否恢复。
照片的红色不能单独证明具体哪一域失效。

APP请发送原F2-horizontal-edges.png（上下两线）一次，不发F5上移图或TOP/END。
固件按原完整图路径做接收CRC、完整BIN解析、相同缩放及映射。
只选择0起索引4（原第5条带，输出y=768..959），其1180*100字节直接送第一次扫描。
跳过原第1..4条带和其他条带，不调用进纸，1次往返，预计约10秒。
下横线因此落在当前纸张起点附近，不再落在完整页下方；喷嘴行与列位置保持。
保留2秒等待、原X列顺序、同次接收数据中的原喷嘴选择及位图。

“逐字节一致”指同一次收到的BIN，隔离路径与完整路径的第5条带一致。
APP重新发送可能重新半色调，不能保证与历史task613的BIN逐字节相同。
这不是移动PNG，因为移动会改变喷嘴相位，不能用来作同喷嘴严格对照。

## 更新BK

```sh
cd ~/armino/bk_avdk_smp
git stash push -m before-isolate-bottom -- :/sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server/ap/app_bluetooth/gatt/printer/printer_diagnostics.h
git fetch origin
git switch codex/printer-isolate-bottom-swath
git pull --ff-only
git log -1 --oneline
cd sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server
make bk7258 SDK_DIR=~/armino/bk_avdk_smp
```

不stash pop；装相同喷头、相同纸，清洁方式与F2对照相同并记录。
放纸恢复字车起点，只发送一次原F2，标纸I（隔离下边）。不用示波器。
如果选中条带为空/越界会在运动前拒绝，不能把F5误发后不动当成故障。

```
[PAGE_ISOLATE] original_swath=5 first_physical_scan=1 feed=0 original_nozzle_rows_and_columns
[PAGE_SWATH] index=5/6 y=768 dots_CMY=...
[PAGE_HEAD_SETTLE] delay_ms=2000 ...
[CYAN_LINE_END] result=0 columns=1180/1180 forward=7480 reverse=7480 ...
[PAGE_ISOLATE_DONE] original_swath=5 feed=0 ink_unverified
[PAGE_RESULT] ... code=0 ...
```

PAGE_PLAN仍显示整幅原图尺寸、源swaths=6和配置feed_steps=3170，但这些不是实际执行次数；
本模式只扫描原第5条带，确实无进纸调用。APP完成表示本次诊断任务完成，不是整页已打印。
该F2隔离条带预计约18796点，实际以重新转换后的PAGE_SWATH为准，HC32总有效位应一致。
任务失败锁定、成功允许下次任务；不重复墨点，不盲目重试。

如果首次扫描仍只后段偏红，支持这批喷嘴/选择路径或扫描过程因素；
如果明显完整，支持先前任务、进纸/上下电经历、等待等过程因素。
都不是单张即能确认某个S段故障，清洁/喷头状态仍影响比较。
发纸面近照和两端日志，不需要再换第三张图。

## 验证

17项主机回归通过。新增实际控制器用完整渲染器第5条带作黄金样本，
断言第一次且唯一扫描逐字节相同，任意进纸调用直接使测试失败，
并检查空条带/越界/内存/坏输入无动作、失败锁定和成功后新任务。
当前环境未完成BK完整SDK编译或实机喷射验证。
回正常整页：切回codex/printer-full-page-settle并重新编译烧录，HC32不改。
