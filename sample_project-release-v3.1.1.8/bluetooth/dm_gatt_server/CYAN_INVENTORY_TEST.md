# 当前分支模式

当前为同地址D1/D2/双选对照，见 [DOMAIN_PAIR_TEST.md](DOMAIN_PAIR_TEST.md)。
下文保留历史试验说明，不是本分支默认动作。

# 原混色线中的青色长线喷嘴：固定清单，逐个打印，交替正反序

分支 `codex/printer-cyan-inventory`，只更新BK。
HC32保留当前实测CE=1.97us版本，电压、CE/S时序、字车速度、7480步行程、列距和尺寸不变。
不预设D1或D2更容易失效；本测试用于观察缺失是否随喷嘴编号、数据域或执行先后位置变化。

## 更新

```sh
cd ~/armino/bk_avdk_smp
git stash push -m before-cyan-inventory -- :/sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server/ap/app_bluetooth/gatt/printer/printer_diagnostics.h
git fetch origin
git switch codex/printer-cyan-inventory
git pull --ff-only
git log -1 --oneline
cd sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server
make bk7258 SDK_DIR=~/armino/bk_avdk_smp
```

烧录并重启，确认 `APP_CYAN_INVENTORY_FROZEN`。不要恢复旧诊断头文件；HC32不更新。
APP发送原F2-horizontal-edges.png一次。重启后第一次生成并缓存原第5条带和喷嘴清单。
随后再次发送F2，复用同一数据，仅倒转单喷嘴执行顺序。
两组之间不擦拭、不整机断电/复位；喷头每次扫描结束照常关闭电源。
先做两组短间隔测试并记录间隔，不同时改变CE或清洁条件。拍整张和缺线近照，保存两端日志。

## 纸面顺序

假设首次F2原条带中有8个符合条件的青色喷嘴，则依物理行升序固定命名C01..C08：

| 组 | 打印顺序 |
|---|---|
| 第1组 | 原混色线 → C01 → C02 → … → C08 → 原混色线 |
| 第2组 | 原混色线 → C08 → C07 → … → C01 → 原混色线 |
| 第3组 | 恢复第1组顺序，之后继续交替 |

每个编号始终对应相同物理喷嘴、地址、S段、D1/D2、原始列位置和墨点数。
无论顺序如何改变，同编号CRC保持一致，首尾原混色线数据也始终一致。
编号只写在日志里，不额外喷印文字。按两端混色线和约2mm线间距对齐照片上的槽位。
线不出墨时仍完成对应扫描及进纸，不会把后面的线挤到空位上。

原F2通常有7或8个长线青色喷嘴，实际数量以 `CYAN_INVENTORY_BEGIN nozzles=N` 为准。
每组共N+2次扫描，单喷嘴每列最多1位，保留自己的原长线点数（通常约895），不追加预喷。
线间距780脉冲约2mm，组后1560脉冲约4mm，仍避免相邻任务叠印。
N=8时每组约100秒，两组20条线墨迹中心跨度约40mm，含末尾组间进纸约44mm；留正常纸边距。
N不同，行数、所需纸长和时间也相应变化。总数最多12个长线喷嘴，不会自动打印全部600喷嘴。

## 选择范围和计划拒绝

只选择原第5条带中实际有至少256个请求点的全部青色喷嘴，不强行挑一个“最强”喷嘴。
选中喷嘴必须同属一个S段、物理行跨度不超过16、同时包含D1及D2，数量最多12；否则运动前拒绝。
这些限制对应本次F2局部带，保持物理间距和源控制负载可比，不是任意整图测试模式。
少于256点的零星青色数据不会当成长线打印，但 `omitted_short_nozzles/dots` 明确报告其数量。
未纳入测试的喷嘴不算失效。第一次仍需F2及原三色长线；后续合规APP输入只触发冻结计划。
重新启动会清空缓存、编号和组序计数；要延续对照就保持控制板运行。

## 日志

`CYAN_INVENTORY_CACHE`：首次CAPTURED、随后REUSED，baseline_crc不变，incoming_crc可变。
`CYAN_INVENTORY_BEGIN`：group=1 order=ASC，group=2 order=DESC，nozzles及scans=N+2。
`CYAN_INVENTORY_NOZZLE`：固定id、physical_row、addr、section、domain、mask、source_dots、crc32。
`CYAN_INVENTORY_PASS`：实际扫描次序和喷嘴id；首尾为ORIGINAL_CMY，中间均为CYAN_ONE。
`CYAN_INVENTORY_DONE`：本组全部扫描和进纸完成，提示下一组顺序。
对比按同一个id及CRC，而不是按照片上的第几条线直接对应。
HC32每扫描应有1180列，invalid=canceled=0。有效位统计只证明程序输入/调用，不能检测实际墨滴。
完整BIN、计划、所有扫描CRC在运动前验证；通信/进纸/CRC异常会停止并锁定，重启并恢复起点后再试。

## 怎么看

- 同一编号反复缺失：固定喷嘴或对应控制路径更可疑。
- 正反序后总是后打印的线弱，编号却变了：更支持打印过程中的状态变化。
- 缺失编号在相同条件下变化：与不稳定/随机缺失相容，但两组不足以证明随机性或概率相等。
- D1/D2都出现缺失：如实记录，不能因为某一组多几个就判定某个数据域坏。
- 首尾混色线也明显变化：当前组含状态漂移，解释单喷嘴缺失时要一起考虑。

这些都是后续检查方向，不能单张纸直接确认堵塞、气泡、供电或逻辑信号故障。

## 验证范围

24项主机测试通过；新增独立规格CSV逐位验证：每个符合条件的原C喷嘴无重复遗漏，
D1/D2均覆盖、原点数/列位置不变、正反序同编号数据相同、首尾混色控制逐字节相同。
实际原生BIN任务测试覆盖冻结缓存、不同APP输入、ASC/DESC/ASC/DESC、组序不受OOM影响、
恒定118KB缓存、780/1560进纸及失败停止锁定。控制器通过Cortex-M33对象编译。
完整BK SDK链接/烧录和纸面效果仍需实机验证。
