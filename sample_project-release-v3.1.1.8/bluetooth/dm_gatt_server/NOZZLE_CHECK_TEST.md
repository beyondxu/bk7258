# 当前分支模式

当前为原青色喷嘴正反序清单对照，见 [CYAN_INVENTORY_TEST.md](CYAN_INVENTORY_TEST.md)。
下文保留历史试验说明，不是本分支默认动作。

# 当前分支模式

当前为五条长线对照，见 [LONG_LINE_CONTROL_TEST.md](LONG_LINE_CONTROL_TEST.md)。
下文保留历史试验说明，不是本分支默认动作。

# 分色、分组逐喷嘴检查

BK分支 `codex/printer-nozzle-check`，仅更新BK。
HC32保持用户当前实测CE=1.97us的本地版本，不切换HC32分支、不覆盖手工修改。
电压、CE/S脉宽、字车起步/巡航速度、每列6脉冲及7480步行程不变。
尺寸放大本次不做。此诊断模式不打印APP图片本身，也不再打印原第5条带两份横线。

## 更新和操作

```sh
cd ~/armino/bk_avdk_smp
git stash push -m before-nozzle-check -- :/sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server/ap/app_bluetooth/gatt/printer/printer_diagnostics.h
git fetch origin
git switch codex/printer-nozzle-check
git pull --ff-only
git log -1 --oneline
cd sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server
make bk7258 SDK_DIR=~/armino/bk_avdk_smp
```

重新烧录BK并重启，启动标记 `APP_NOZZLE_CHECK`。不要恢复旧诊断头文件。
保持原有字车机械起点并准备足够纸张；一张检查图墨迹纵向名义跨度约41mm，另留正常边距。
APP发送一张现有、可成功转换的图片一次即可触发，仍先检查完整APP BIN及接收长度/CRC。
可以继续发F2，但图像内容仅用于触发；不要把本目录expected.png当待打印输入。
启动不自动打印。一次任务自动运行12次往返、4次3170脉冲进纸，约两分钟。
等待完成后再发下一张，不要重复点发送。头在每次组扫描完成后正常关闭。
先按当前不擦拭的状态打印一张，保留整张及分色近照和BK/HC32整段日志。
若需要比较重复性，下一张保持相同供电/喷头/擦拭条件，并记录两张之间等待时间。

## 图形与编号

- [预期图PNG](tests/nozzle-check/expected.png) / [可放大SVG](tests/nozzle-check/expected.svg)。
- [600行喷嘴对应表CSV](tests/nozzle-check/nozzle-map.csv)。

按C青、M品红、Y黄依次打印，每色4组，组间不进纸：

| 组 | 物理Y降序行号 | 对应S段 | 喷嘴数量 |
|---|---|---|---|
| G1 | 1..50 | S1 | 50 |
| G2 | 51..100 | S2 | 50 |
| G3 | 101..150 | S3 | 50 |
| G4 | 151..200 | S4 | 50 |

每组每个喷嘴画16点短线，横向错开排列，看起来是50个短横线组成的阶梯。
每次扫描1180列，每列最多一个选中位，严格不叠加两个喷嘴。
每色200个喷嘴合计3200个请求墨点，全图600个喷嘴、9600个请求墨点。
每两个颜色之间进纸两次3170脉冲，按现有标定合计约16.26mm，以分开完整200行带。
总进纸4次，不自动出纸；实际尺寸和方向仍以机械与纸面为准。
图中的颜色名、G1..G4和行号只在参考图中显示，不增加额外喷射标记。
预期图按逻辑X、物理Y降序绘制，纸面可能左右/上下翻转；先以出纸颜色顺序和组形状辨认。

这里行号1..200是规格表按物理Y排序后的索引，不是某个数据域的厂家喷嘴编号。
CSV同时列出address、section、domain、domain_nozzle_num、slot、mask及实际发送列范围，
可把缺失短线追踪到具体控制组合。短线名义长度约0.48mm，宜近拍或放大辨认。
普通图片路径只覆盖192行，本诊断通过合法的first_nozzle=8映射覆盖末尾8行，不修改普通图片路由。

## 日志核对

应有一个 `NOZZLE_CHECK_BEGIN`，明确 `APP_image_is_trigger_only`。
每个 `NOZZLE_CHECK_PASS` 包含color、group、section、物理行范围、800点和该组118000字节CRC。
同样输入重复任务时，同组CRC应相同；改变APP图片不改变本诊断点阵。
每个组有 `CYAN_LINE_END result=0 columns=1180/1180 forward=7480 reverse=7480`。
CYAN_LINE是原有扫描函数名，实际颜色以NOZZLE_CHECK_PASS为准，不代表只打印青色。
最后应有 `NOZZLE_CHECK_DONE scans=12/12 feeds=4/4 planned_unique_nozzles=600 requested_dots=9600`。

HC32应在每组关闭喷头后给出统计，共12组；正常每组columns=fired=1180，invalid=canceled=0。
其他墨点统计是计划输入位数，不是物理出墨。
固定参考喷嘴C95/M135/Y175仅在对应颜色和所在组中有16点，其余组为0属正常。
所有输送计数和CRC都不能证明内部加热或实际墨滴形成。

若通信、CRC或进纸计数异常则立即停止，不盲目继续运动，任务锁定到重启；恢复机械起点后再试。
发生故障时完整检查图可能没打印完，不能把未执行的组当成喷嘴全部失效。

## 观察方向

缺失短线位置固定：查对应地址/S/域与接触路径；整组缺失：重点核对对应S段。
组内隔一条缺一条：重点核对对应颜色两个数据域（奇偶行）的差异。
每次缺失位置变化或随时间衰减：继续查供墨/气泡/温度/带载驱动。
本次每列只选一个喷嘴，若此条件良好而原混色线很差，再设计增加同时喷嘴数的负载对照。
以上均是定位方向，不是单张纸面即可确定故障元件；分组扫描顺序也会影响喷头状态。

## 验证范围

20项主机测试通过。新增测试依据独立规格CSV逐位比较实际12组列缓冲区，
证明600个地址/S/域组合无重复遗漏、每喷嘴16点、每列最多1位，两个X方向及末尾8行均覆盖。
参考CSV也与实际映射比较。任务测试验证完整BIN/链路/OOM拒绝时无运动，12组扫描与4次进纸，
成功后可重复、首/中/末组通信错误及OFF失败/短进纸/缓冲区变化时停止和锁定。
任务控制器/点阵生成器通过Cortex-M33对象编译；原字车和进纸脉冲回归通过。
本环境未完成完整BK SDK链接、烧录或实机喷墨认证。

参考文件可运行 `python tools/nozzle_check_reference.py` 重新生成，需matplotlib。
