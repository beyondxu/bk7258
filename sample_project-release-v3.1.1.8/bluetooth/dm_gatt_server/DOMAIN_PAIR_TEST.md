# 同地址、同S2：D1 / D2 / D1+D2逐项对照

BK分支 `codex/printer-domain-pair`。HC32只更新接收统计头文件，保留用户当前实测CE=1.97us的喷射函数源码及延时设置。
本轮目标：排除上轮D1与D2使用不同地址的干扰，并用HC32接收CRC验证整段数据确实到达。
不是据软件调用计数判断喷嘴已经出墨，也不是直接认定D1硬件坏。

## 最新供电补充

用户要求现有一路升压中解决，不改VL/VP分路。
技术支持新提供了电压/时间参考，先读 [SUPPORT_ENERGY_REVIEW.md](SUPPORT_ENERGY_REVIEW.md)。
用户已确认当前一路15V，本轮固定15V及CE1.97us进行同地址比较，不同时改电压和CE。
CE保持当前值，不能用低于S采样总时间的目标来代替供电核实。

## 1. 更新BK

```sh
cd ~/armino/bk_avdk_smp
git stash push -m before-domain-pair -- :/sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server/ap/app_bluetooth/gatt/printer/printer_diagnostics.h
git fetch origin
git switch codex/printer-domain-pair
git pull --ff-only
git log -1 --oneline
cd sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server
make bk7258 SDK_DIR=~/armino/bk_avdk_smp
```

烧录BK并重启，确认 `APP_DOMAIN_PAIR_FROZEN`。
不要恢复旧诊断头文件。所有扫描仍去回各7480脉冲，原速度、列距、供电与2秒上电等待不变。

## 2. HC32只取统计头文件，不切换分支

不要在HC32仓库执行switch到审计分支或stash全部改动，这会影响用户本地CE调整。
下面只取 `printer_row_audit.h`；不修改 `tph_m0333n.c`、`tph_driver.c`、main.c或Keil配置。

```powershell
cd E:\AI_START\mcuHCF460\HC32F460_DDL_Rev3.3.0
git fetch origin
$hcAuditPath = "projects/ev_hc32f460_lqfp100_v2/examples/spi/spi_dma/source/printer_row_audit.h"
if (!(Test-Path "$hcAuditPath.before-rx-audit.bak")) { Copy-Item $hcAuditPath "$hcAuditPath.before-rx-audit.bak" }
git restore --source=origin/codex/printer-domain-audit -- $hcAuditPath
```

在这个目录对应的Keil SPI DMA工程执行Rebuild all并下载。
原启动标记可以不变，更新是否生效以每次扫描关头后出现 `[HC32 RX_AUDIT]` 为准。
该头文件仅统计每列收到的100字节，计数在现有TPH_FireOneColumn返回后执行。
没有改动CE高电平内部指令；会增加少量列间处理及末尾串口打印时间，对照各情况统一使用这份统计版本。
CRC查表在每次上电统计开始前准备一次，列内不打印；新增末尾统计为一条紧凑日志。

## 3. 打印操作

发送原F2-horizontal-edges.png一次，首次捕获原第5条带作为混色参照。
长线模板优先取原数据中的地址2/S2/D2；若不存在足够长的该迹线，则选择实际存在的长D2/S2迹线。
模板来源、点数和全部8个扫描点阵都冻结到控制板重启。后续APP数据仍校验，但只触发原计划。
六个诊断情况使用同一组有效列，单域选1个青色喷嘴，双域选2个青色喷嘴。
不增加CE宽度、预喷或单喷嘴重复次数；双选总请求点数为单选的两倍。

第一组8次扫描：

| 扫描 | 名称 | 地址 | S段 | 数据 | 正常应见 |
|---|---|---|---|---|---|
| 1 | ORIGINAL_CMY | 原数据 | 原数据 | 原数据 | 原混色参照线 |
| 2 | A2_D1 | 2 | S2 | 0x01 | 1条青线 |
| 3 | A2_D2 | 2 | S2 | 0x02 | 1条青线 |
| 4 | A2_D1_D2 | 2 | S2 | 0x03 | 2条分开的青线 |
| 5 | A5_D1 | 5 | S2 | 0x01 | 1条青线 |
| 6 | A5_D2 | 5 | S2 | 0x02 | 1条青线 |
| 7 | A5_D1_D2 | 5 | S2 | 0x03 | 2条分开的青线 |
| 8 | ORIGINAL_CMY | 与1相同 | 与1相同 | 与1相同 | 原混色参照线 |

第二组首尾参照不变，六个诊断情况倒序，之后各组继续交替。
D1/D2都是青色，不能用颜色区分；双选产生两条墨迹，不是一条加粗线。
按照规格，地址2的D1/D2分别为物理行85/60；地址5分别为59/84，单扫描两线相距约1.06mm。
因此实际墨线间距不是均匀2mm：纸张每扫描进2mm，但喷嘴自身Y位置也不同。拍整张并保留扫描顺序日志。
组后进纸4mm，不叠印下一组。8次扫描最多10条可见墨迹，约80秒；两组纸上跨度约32mm，留正常边距。

先连续做两组，中间不擦拭、不整机断电或复位；喷头电源照常在每次扫描后关闭。
不要求示波器。保留BK/HC32完整日志与同框照片，先不增加额外清洗或调整CE。

## 4. 核对数据，再解释纸面

每次BK的 `DOMAIN_PAIR_PASS crc32` 应与HC32紧随该次关头的 `RX_AUDIT crc32` 相同。
每次应为1180列、118000字节、invalid=canceled=0。
BK的 `DOMAIN_PAIR_EXPECT` 给出HC32应收到的D/A/S位图和D1..D6点数。

假设模板有895个有效列：

- D1：D=01，dots_D1_D6=895,0,0,0,0,0。
- D2：D=02，dots_D1_D6=0,895,0,0,0,0。
- 双选：D=03，dots_D1_D6=895,895,0,0,0,0。
- 地址2：A=00000002；地址5：A=00000010；S2：S=02。

点数以该任务模板日志为准，不强制等于895。第一、末次混色参照没有这个单地址约束，但收发CRC仍应相同。
接收统计只证明HC32缓冲区中的数据及调用情况，不证明GPIO实际波形、内部加热或墨滴形成。
若CRC/计数不符，先处理数据链，不能把那次缺墨归因于喷嘴。

## 5. 本轮判据

- 同一地址D1不出、D2出，且收发CRC/字段吻合：不同地址及接收数据差异不再是这几次结果的解释，优先查D1相关路径/喷头组。
- D1单独不出、双选时两条都出现：说明单选与双选条件有关，查选通、信号耦合、驱动/负载状态；不能直接等同某个唯一原因。
- 异常主要跟随地址：转查地址路径或对应喷嘴。
- 两个地址都表现D1异常：再做同地址跨S段验证，不能直接宣布PA0损坏。
- 首尾参照也变化：同组存在状态变化，须一起解释。
- 所有情况恢复：记录当前条件，不能仅因一次恢复就宣布故障根因已找到。

进度及排除范围见 [DIAGNOSTIC_STATUS.md](DIAGNOSTIC_STATUS.md)。

## 验证

BK26项、HC32四项主机测试通过。新点阵与独立CSV/原数据比较，确认两个地址、S2、三种域选择、原有效列与点数及正反序。
实际BIN及缓存任务测试包含通信/CRC/进纸/内存失败后停止锁定；两组数据冻结。
HC32接收CRC与独立zlib对照，覆盖零列、单/双域及六域混合，检查输入不被修改、重置与紧凑末尾日志。
BK控制器通过M33对象编译，HC统计通过M4对象编译；完整SDK/Keil链接和物理信号/出墨仍需实机确认。
