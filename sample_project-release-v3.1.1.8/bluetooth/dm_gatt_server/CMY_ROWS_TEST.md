> 当前 `codex/printer-full-page` 的步骤见 [FULL_PAGE_TEST.md](FULL_PAGE_TEST.md)，下文为旧固定三排图案说明。

# 同纸三排颜色定位图

分支 `codex/printer-cmy-rows`。当前目标是比较同一次扫描中的三色端点和长度，
不是继续凭不同照片调整补偿，也不代表全喷嘴打印已完成。
沿用实机通过的起步3000、巡航260、7480步运动；候选列距固定30um、方向-1。
不改变HC32固件、喷墨波形、电压或脉宽，不走纸，不读取nFAULT。

## 获取和操作

本地之前有颜色/方向/列距参数修改，先保存，不要stash pop：

```sh
cd ~/armino/bk_avdk_smp
git stash push -m before-cmy-rows -- :/sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server/ap/app_bluetooth/gatt/printer/printer_diagnostics.h
git fetch origin
git switch codex/printer-cmy-rows
git pull --ff-only
git log -1 --oneline
cd sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server
make bk7258 SDK_DIR=~/armino/bk_avdk_smp
```

参数保持默认：ALIGN_PATTERN=2、CHANNEL_MASK=7、X_PITCH_CENTI_UM=3000、X_DIRECTION=-1，
BOUNDED_PRINT=1、CARRIAGE_TEST=0。SOLID_LINE=1表示生成图案，不是打印APP像素。
只烧录BK7258，HC32保持PCTLv1配套版本。
放一张新纸，恢复字车起点，两个MCU断电重启。开机不运动；APP发同一张黑横线触发。
先检查完整接收CRC及BIN格式，随后在一次前进中打印三排，关喷头后返回，不走纸。
一次开机最多尝试一次；若卡住/撞到边界立即断电，不用手推动通电字车。

## 图案与新增验证点

每排均是逻辑x=300..449、500..649、700..849三段，各150点，共450点。
三排在喷嘴排列方向分开，不依赖走纸，候选排间距约1.693mm。

| 颜色 | 逻辑喷嘴行 | 地址/分区 | 数据槽/数据 | 延迟列 |
|---|---|---|---|---|
| 青 | 95 | 19 / 2 | 73 / 0x02 | 59 |
| 品红 | 135 | 12 / 3 | 46 / 0x08 | 30 |
| 黄色 | 175 | 4 / 4 | 15 / 0x20 | 0 |

黄色沿用此前验证过的喷嘴；青色和品红改用另外两个喷嘴，以便错开高度，
所以这次也在检查新的地址和分区。若某一排缺失，不能直接认定是补偿错误。
三色均用偶列域D2/D4/D6；不扩大同时开启的喷嘴数量，不做预喷或重复加热。
在规格书坐标中行号增加对应Y坐标减小，实际照片上下顺序可能因纸张摆放不同而改变，
按颜色识别各排，不把照片上方直接称作物理原点。

## 日志与反馈

BK的ALIGN_ROUTE应分别包含row=95/135/175、slot=73/46/15、delay=59/30/0。

```text
[ALIGN_PATTERN] source=GENERATED mask=7 ... dots_per_color=450 row_mode=2 ...
[LINE_PLAN] result=0 mask=7 dots_CMY=450,450,450 first=300 last=908 ...
[CYAN_LINE_END] result=0 columns=1180/1180 forward=7480 reverse=7480 ...
```

HC32首次非零通常为最早开始的黄色slot15/data0x20；仅记录首次列不代表只打印黄色。
CYAN_LINE是共用执行器的历史日志名。

请提供整张定位图（同一画面包含三排和毫米尺）、端点近照、两个MCU完整日志。
重点看各排对应三段的起点/终点是否在同一横向位置，以及哪一侧短线是否共同偏短。
先不根据深浅调整能量：颜色、不同喷嘴及分区的出墨情况还需分开判断。
候选30um列距并非精确标定；同纸图便于下一步量化残余偏差。

## 验证边界

复用原三段图案生成核心，只将三色行号分开。主机测试覆盖全部颜色组合、两个X方向、
逐字节输出、450点计数、独立规格书CSV的行间距和X补偿，以及越界拒绝不部分写入。
接收、一次任务限制、运动、电源和通信失败回归通过。
未完整构建BK SDK固件或实机验证；仅三个喷嘴，不是整张图片打印。
