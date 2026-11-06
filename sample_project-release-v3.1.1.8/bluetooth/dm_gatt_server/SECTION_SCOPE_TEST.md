# 当前分支入口

本分支改为纸面检查，不要求示波器，见[SECTION_PAPER_TEST.md](SECTION_PAPER_TEST.md)。
以下仅为历史波形版说明。

# 固定地址4、青色D2，逐段抓波形

分支codex/printer-section-scope。只更新BK；HC32固定3e7e161。
本版不再打印六列短线，也不打印APP的TOP/END：每次APP任务生成4条青色长线。
这不是之前导致空白的独立CE实验。HC32的CE/S/D产生方式完全未改。

## 精确选择

保持地址4、D2=0x02，仅分段不同，一次往返对应一个段：

| 往返次序 | 段 | 使用行号（1起） | 100字节槽（0起） | 有效列 | 点数 |
| --- | --- | --- | --- | --- | --- |
| 1 | S1 | 26 | 12 | 300..899 | 600 |
| 2 | S2 | 76 | 13 | 300..899 | 600 |
| 3 | S3 | 126 | 14 | 300..899 | 600 |
| 4 | S4 | 176 | 15 | 300..899 | 600 |

原始0起行25/75/125/175，已用规格CSV独立验证。使用行号不是Domain内喷嘴号。
S4行与早期row175单线测试一致，但不能保证当前喷头仍正常。
四条线的X范围相同，约18mm长，Y间距约2.12mm，总跨度约6.35mm。
只用这4个喷嘴，不对整片喷嘴或堵塞程度作结论。

每列最多只有一个有效点（一个地址、一个段、一个域），其余99字节为0。
地址轮询、四段采样与清零、CE门控保持原HC32实现；不是只给选中的S输出脉冲。
每个非空列仍由完整100字节传输。未增加电压、单点脉宽或重复同一纸面墨点。
字车原速7480步往返，4次往返、0次进纸，每段之前CE/电源关断并等3秒。
预计一次任务约38秒；图案不等于前一版检查图，不能拿浓度做单变量比较。

## 更新BK

```sh
cd ~/armino/bk_avdk_smp
git stash push -m before-section-scope -- :/sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server/ap/app_bluetooth/gatt/printer/printer_diagnostics.h
git fetch origin
git switch codex/printer-section-scope
git pull --ff-only
git log -1 --oneline
cd sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server
make bk7258 SDK_DIR=~/armino/bk_avdk_smp
```

烧录后不stash pop，放好纸，APP发送原TOP/END图一次作为任务触发。
接收CRC及完整BIN语法检查仍执行，再生成固定四条线。启动不自动打印。
成功允许新任务，物理执行失败后锁定，后续段不执行；没有自动重试或进纸。

## 示波器设置：先取S1和S4两张

HC32引脚已与当前头文件核对，尽量在靠近喷头连接端的同名逻辑信号测量。
探头地接逻辑GND，以下均为逻辑信号，不把示波器地夹接到电源输出或马达端。

| 通道 | 信号 | HC32引脚 |
| --- | --- | --- |
| CH1 | CE | PB15 |
| CH2 | D2 | PA2 |
| CH3 | S1 | PC15 |
| CH4 | S4 | PC14 |

**触发CH2(D2)上升沿，阈值约1.5V，单次，水平500ns/div或1us/div。**
用D2触发选择真正有数据的地址，不用CE触发那些全0地址。
S为低有效取样、高电平锁存，截图需保留电平刻度、时间刻度和触发位置。

开始前接好探头并按Single；第一段抓到后保存为S1。
查看串口，等出现`SECTION_SCOPE_ARM section=4`时再按Single，
有3秒停顿加到达有效列前的时间，抓到后保存为S4。
若错过，不需要为此改变脉冲参数，保留已抓图并说明对应日志段。
S2/S3暂不要求，后续需要时将CH3/CH4改到PH1/PH0。

需要看的是D2有效区、对应S的采样窗口以及CE高电平的先后关系，
不是只量一次CE总高脉宽；旧版CE约2.3us的测量仍保留，不将其当作各喷嘴tFire。
该测量只能比较外部逻辑，不能直接观测内部加热器或证明墨滴喷射。

## 日志和纸面

```
[SECTION_SCOPE_BEGIN] generated_C_D2_addr4 dots_each=600 sweeps=4 feed=0 ...
[SECTION_SCOPE_ARM] section=1 used_row=26 slot=12 data=0x02 D2_PA2 addr4 settle_ms=3000
[CYAN_LINE_END] result=0 columns=1180/1180 ...
[SECTION_SCOPE_PASS] section=1 code=0 columns=1180 expected_dots=600 ...
... section=2/3/4 ...
[SECTION_SCOPE_END] code=0 locked=0 ...
[PAGE_RESULT] ... code=0 ...
```

HC32每次other_bits加固定槽计数总和应600，非选中段的固定统计可能全0，不能当失败。
纸面4条线从使用行26→176排序；若方向与预期相反，先记录不改参数。
请发完整纸面照片、哪几条缺失/变淡、S1/S4波形截图及两端日志。
本次只需一张纸，不以成功的函数计数宣布4个喷嘴都出墨。

## 软件验证与回退

19项主机脚本通过。新增测试按CSV验证四个精确地址/段/域/行关系和600点唯一槽，
实际控制器验证完整解析、内存失败无动作、四段顺序/3秒等待、段失败后停止锁定、
电源命令失败停止、成功后允许下一任务。映射模块通过M33对象编译。
未在本环境完成完整BK SDK构建或板上逻辑时序/喷墨验证。
需要回原完整图：切回codex/printer-full-page-groups/7e642a4并重新编译烧录BK；HC32不改。
