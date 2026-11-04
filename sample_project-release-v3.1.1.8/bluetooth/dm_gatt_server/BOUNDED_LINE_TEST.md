> 本分支 `codex/printer-cyan-line` 的当前步骤以 [CYAN_LINE_TEST.md](CYAN_LINE_TEST.md) 为准；下文是早期测试说明。

# 单色横线测试（BK7258 / HC32 必须配套）

本分支用于定位“黑线变成多条不同颜色的线”，不是完整照片打印版本。
两个仓库都使用 `codex/printer-bounded-line`；不能把实际打印模式与旧 HC32 固件混用。
HC32 新增 PCTLv1 控制帧，上电不自动打开喷头；旧固件不能识别该控制帧。

## 更新与编译

BK SDK 根目录执行（保存本地诊断配置，暂时不要把 stash 再应用回来）：

```sh
git stash push -m before-bounded-line -- sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server/ap/app_bluetooth/gatt/printer/printer_diagnostics.h
git fetch origin
git switch codex/printer-bounded-line
git pull --ff-only
cd sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server
make bk7258 SDK_DIR=~/armino/bk_avdk_smp
```

若其他本地文件导致切分支被拒绝，保留这些改动，不要使用 reset --hard。
HC32 仓库同样 fetch / switch 到该分支，用原来的 MDK 工程编译烧录：
`projects/ev_hc32f460_lqfp100_v2/examples/spi/spi_dma/MDK/spi_dma.uvprojx`。
无需向 MDK 增加 C 源文件，新控制头文件由 main.c 引用。

## 第一步：默认仅生成计划

`ap/app_bluetooth/gatt/printer/printer_diagnostics.h` 保持：

```c
#define PRINTER_DATA_AUDIT 1
#define PRINTER_MAP_AUDIT 1
#define PRINTER_BOUNDED_LINE 1
#define PRINTER_BOUNDED_PRINT 0
#define PRINTER_BOUNDED_CHANNEL_MASK 1
#define PRINTER_STARTUP_DIAGNOSTIC 0
```

仍从 APP 发送之前的 `tests/audit-images/03-black-horizontal-1px.png`，质量中、方向正向、不镜像。
默认不启动 SPI 握手、不升压、不运动、不出墨。查看 `[LINE_PLAN]`：
`result=0`、`mask=1`、只有 C 点数非零、点数至少 64、first/last 在 200..999 内。
随后应有 `PLAN_ONLY no_SPI_no_motion_no_ink`。

之前提供的最后一次横线 BIN 在本机生成：C=369、first=231、last=948。
APP 再次转换图片可能使点数变化，不能要求每次都是 369。
如 result 非零，先提供完整日志，不启用实际打印。

## 第二步：只打印青色

确认两个 MCU 都已烧录此配套分支；更改 BK 的 `PRINTER_BOUNDED_PRINT` 为 `1`，重新编译烧录。
其余参数不变，使用原来的打印起始位置、原来的纸张放置方式；这段代码没有自动寻零。
断电重新启动两个 MCU，再从 APP 发送同一张黑横线。

预期日志顺序：开机 SPI 回环自检通过；HC32 `[HC32 CONTROL] PCTLv1 ready power=OFF`；
接收/校验/计划成功；HC32 `power=OFF`、`power=ON`；列数据；`power=OFF`；
BK `[LINE_DONE] sweep_and_return=1 feed=0 ink_unverified` 和 `[LINE_RESULT] ... PASS`。

它只取逻辑条带 y=576 的第 175 行和选定颜色，不执行整页走纸。
沿用原来的 1180 列 × 6 微步，加减速各 200 步，总共 7480 步扫描；关喷头后返回 7480 步。
纸张电机不发送步进脉冲。一个开机周期最多尝试一次实际打印；重复测试要重启。
SPI 失败立即停机，不盲目执行整段返回。PASS 仅说明软件/传输完成，不能证明实际出墨。

若有墨，预期为一条青色线（抖动后的点列可能不连续）；请提供照片和两个 MCU 的完整日志。
若仍白纸，也提供日志；本次保留旧喷墨波形，不能据此判断喷头损坏。
先完成青色测试，再分别把 CHANNEL_MASK 改为 2（品红）、4（黄）；每次重编译并重启。

## 实现边界及验证

BIN 的长度与 CRC 先通过，完整解码后才允许运动。选择条带、行、颜色；所选有效点若越出
200..999 列、缺少条带、没有墨点、格式不支持或不足 64 点，拒绝实际打印。
喷嘴采用规格书坐标生成的映射，每个逻辑点只选择一个物理喷嘴；颜色偏移仍是候选参数。
没有增加行程，没有修改原有喷墨脉冲；完整图片、走纸、双向打印、颜色标定尚未完成。

已验证主机端接收协议、CRC、BIN 解析、576 个逻辑喷嘴映射、限定输出与失败停机顺序；
HC32 主机握手测试覆盖电源指令、DMA 未完成、取消、无电源和非法数据，三个源文件通过 ARM 对象编译。
此环境未完成完整 BK SDK 固件构建，也没有实机验证；最终编译和烧录需在本地完成。
