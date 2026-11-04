# 连续单喷嘴黄色测试

分支 `codex/printer-solid-line`，沿用已验证的运动分支 `codex/printer-cyan-line`。
默认只测黄色。HC32仍为PCTLv1配套固件，不更改波形、电压或脉宽。
目的：此前黄色335个稀疏点未观察到墨迹；本次用同一个物理喷嘴连续600列发点，
排除APP抖动和图片中黄色分量稀疏的影响。尚未确认黄色实际出墨。

## 更新

此前本地可能把CHANNEL_MASK改为4，先保存旧参数再切新分支，不要stash pop：

```sh
cd ~/armino/bk_avdk_smp
git stash push -m before-solid-yellow -- sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server/ap/app_bluetooth/gatt/printer/printer_diagnostics.h
git fetch origin
git switch codex/printer-solid-line
git pull --ff-only
git log -1 --oneline
cd sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server
make bk7258 SDK_DIR=~/armino/bk_avdk_smp
```

默认 SOLID_LINE=1、CHANNEL_MASK=4、BOUNDED_PRINT=1、CARRIAGE_TEST=0。
不用改参数，只烧录BK7258，HC32不改。放好纸张、恢复字车起点，两端断电重启。
开机不运动，APP发送之前同一张黑横线即可触发；不依赖该图片的颜色点阵。
仍先检查接收CRC和完整BIN格式。格式不支持、传输不完整或CRC错误不会运动。

## 本次具体数据

全长1180列，只有第300..899列非零；每个非零列只有slot15的0x20，其他99字节为零。
即D6、地址4、分区4、一个黄色喷嘴，连续600列、总600个点；不同时打开多个喷嘴。
按候选几何表逻辑row175选取，与先前黄色测试相同。
不做额外重复喷射、加长加热脉宽、提高电压或多次叠印。
单程7480步，起步3000、巡航260、加减速各250步；只往返一次、不走纸，不读取nFAULT。
使用此前相同的喷头OFF/ON/OFF控制和失败清理；一个开机周期最多尝试一次打印。

## 日志与观察

BK应有：

```text
[SOLID_LINE] source=GENERATED channel=2 row=175 slot=15 data=0x20 first=300 last=899 dots=600 waveform_unchanged
[LINE_PLAN] result=0 ... mask=4 dots_CMY=0,0,600 first=300 last=899 ...
[CYAN_LINE_END] result=0 columns=1180/1180 forward=7480 reverse=7480 ...
```

CYAN_LINE仍是共用执行器的历史日志名字，颜色由mask及实际data决定。
MAP_DOTS统计的是收到的APP图；SOLID_LINE/LINE_PLAN统计本次固件生成的测试点，两者不要求相等。
HC32应有power=ON、slot15/data0x20喷墨函数记录、power=OFF。
请在白光下观察纸面，反馈照片、两个MCU完整日志和字车是否平稳往返。
如果依然看不到黄色，不判定为成功，也不继续盲目增加能量；下一步核对D6/S4/CE实际波形及供墨。
若卡住或碰到边界立即断电，不用手推动正在通电的字车。

## 验证边界

主机测试逐字节验证1180列中恰好600个单bit、范围外全零、各颜色的同一逻辑喷嘴路由；
覆盖非法边界、完整解析失败不运动、一次打印限制、原运动与电源/失败清理回归。
未完整构建BK SDK固件、未实机验证。默认测试不依赖APP点阵，不是完整图片打印版本。
