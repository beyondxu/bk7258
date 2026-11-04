> 当前 `codex/printer-cmy-rows` 的步骤见 [CMY_ROWS_TEST.md](CMY_ROWS_TEST.md)，下文为同排叠合的历史测试说明。

# CMY 三色位置重合测试

分支 `codex/printer-cmy-align`，保留已验证的3000起步、260巡航、7480步运动和原HC32喷墨波形。
只更新BK7258，HC32不改。此前仅分别验证了三个选定喷嘴；本次验证它们的相对位置。
不是完整照片或全喷嘴测试，不调整电压/加热脉宽/墨量。

## 更新与测试

先保存本地改过的颜色参数，不要stash pop：

```sh
cd ~/armino/bk_avdk_smp
git stash push -m before-cmy-align -- :/sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server/ap/app_bluetooth/gatt/printer/printer_diagnostics.h
git fetch origin
git switch codex/printer-cmy-align
git pull --ff-only
git log -1 --oneline
cd sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server
make bk7258 SDK_DIR=~/armino/bk_avdk_smp
```

默认 ALIGN_PATTERN=1、SOLID_LINE=1、CHANNEL_MASK=7、BOUNDED_PRINT=1、CARRIAGE_TEST=0。
参数保持默认，烧录后放好纸、恢复字车起点，两个MCU断电重启。
开机不会自动运动。用APP发送之前同一张黑横线，作为触发。
先验证完整接收CRC和BIN格式，然后使用固件生成的图案，与APP抖动像素无关。
每次开机最多尝试一次实际打印；不走纸，不读取nFAULT。

## 图案和候选位置补偿

同一逻辑行175，三色分别使用此前已验证的D2/D4/D6、地址4、分区4。
逻辑图案为三段短线：x=300..449、500..649、700..849，各150点，中间留50列间隔。
每种颜色共450点，三色共1350点；不同颜色按物理位置分开时刻发送，不能直接在同列写0x2A代替补偿。

默认候选几何为每列42.33um、X方向+1，route输出延迟：

| 颜色 | 数据位 | 延迟列数 | 第一段输出列 | 最后有效列 |
|---|---|---|---|---|
| 青 C | 0x02 | 1 | 301..450 | 850 |
| 品红 M | 0x08 | 22 | 322..471 | 871 |
| 黄 Y | 0x20 | 43 | 343..492 | 892 |

同一输出列可能有多个有效色位，这是各自逻辑点经过位置补偿后的正常叠加。
所有有效点仍在中央200..999范围，行程不增加；喷头返回前关闭，失败下电不盲目整段返回。
候选补偿来自喷嘴规格书坐标，实际每列距离及方向仍需通过纸面验证，不能先认定正确。

## 预期日志和观察

BK应出现三条 `[ALIGN_ROUTE]`，delay分别1、22、43；随后：

```text
[ALIGN_PATTERN] source=GENERATED mask=7 bars=300..449,500..649,700..849 dots_per_color=450 waveform_unchanged
[LINE_PLAN] result=0 ... mask=7 dots_CMY=450,450,450 first=301 last=892 ...
[CYAN_LINE_END] result=0 columns=1180/1180 forward=7480 reverse=7480 ...
```

HC32首次非零可能只记录0x02（最早开始的青色），不要求首次日志就是0x2A。
CYAN_LINE是共用执行器历史日志名；实际输出看ALIGN_PATTERN/ALIGN_ROUTE。

预期纸面出现三段短线，三色尽量在同一位置叠合，通常会比单色深；不承诺得到标准黑色。
重点看每段两端是否有彩色边、是否分成多条平行彩线、间隔是否清楚。
若不重合，保留样张和方向，先记录偏差，不调整脉宽或同时更改多个参数。
请提供完整纸面照片、三段端点近照、两个MCU完整日志，以及实际往返情况。
仍卡顿或撞边立即断电，不用手推动通电字车。

## 验证边界

主机测试逐字节验证三段图案、所有颜色组合、正反方向、每色450点、范围外为零，
并用独立规格书CSV坐标检查候选物理重合误差。覆盖越界和非法参数拒绝、接收失败不运动、
一次打印限制和原运动/电源/通信失败回归。未完整构建SDK固件或实机验证。
