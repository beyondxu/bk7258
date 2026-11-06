# 当前分支入口

本分支为两批交错喷嘴对照，字车恢复原速，见[NOZZLE_BATCHES_TEST.md](NOZZLE_BATCHES_TEST.md)。
下文为历史版本说明，不作为当前分支的更新步骤。

# 低负载版本入口

本分支默认打印去程STEP低电平延时乘4，详细更新和对照步骤见[NOZZLE_LOW_LOAD_TEST.md](NOZZLE_LOW_LOAD_TEST.md)。
下文保留原速度检查说明；本版本耗时预计50～60秒，见NOZZLE_LOAD日志。

# 可见喷嘴短线检查（BK专用分支）

分支codex/printer-nozzle-lines，从7e642a4完整图分组版建立。只更新BK7258。
HC32固定回退后能出墨的3e7e161；不启用SECTION_PULSE实验、不改变CE/S时序。

## 实际打印内容

APP发送一次之前的TOP/END结构图作为任务触发；本分支完整校验接收和BIN语法，
随后忽略其图像像素，生成固定检查图。不要把APP预览当作本次实际图形。
启动不打印；每次任务只执行一次三色检查，不自动循环。

按C、M、Y顺序分别扫描并返回，在颜色之间各进纸一次3170步：
3次往返、2次进纸，无最终自动出纸。预计约半分钟，真实时间以日志为准。
同一任务每色192条短线，覆盖实际全图使用的192/200个喷嘴，不是全部600喷嘴。
每喷嘴96个连续列点（约2.88mm），原微小覆盖图每喷嘴12点，这次长度8倍。
只延长空间短线，不增加单点脉宽、电压或同点重复喷射。

每种颜色分为从左到右6列，每列32条短线：

| 列 | 从上到下的使用行号（1起） |
| --- | --- |
| 1 | 1,7,13,...,187 |
| 2 | 2,8,14,...,188 |
| 3 | 3,9,15,...,189 |
| 4 | 4,10,16,...,190 |
| 5 | 5,11,17,...,191 |
| 6 | 6,12,18,...,192 |

同列短线中心距约0.254mm，相邻列横向间隔留白以避免重叠。
这些“行号”是排序后的使用行索引，不是规格书Domain内喷嘴编号。
对应地址/分段/域见tests/page-images/nozzle-lines/row-legend.csv。
参考图reference.png包含屏幕参考标签并放大了Y；标签不在纸上打印。
图形朝向沿用已验证完整图的Y方向，不要更改镜像或调换纸面方向。
黄色观察受纸张/光照影响，近拍时避免反光，不以手机自动曝光后的颜色深浅作唯一证据。

## 更新

```sh
cd ~/armino/bk_avdk_smp
git stash push -m before-nozzle-lines -- :/sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server/ap/app_bluetooth/gatt/printer/printer_diagnostics.h
git fetch origin
git switch codex/printer-nozzle-lines
git pull --ff-only
git log -1 --oneline
cd sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server
make bk7258 SDK_DIR=~/armino/bk_avdk_smp
```

烧录后不要stash pop，默认NOZZLE_CHECK=1、FULL_PAGE=1、3170步。
该模式走完整任务应答，因此正常成功后可再次发送，不要求复位才能第二次测试。
失败后锁定到复位，不在位置不确定时盲目继续。

## 一次有用的对照

用同一个喷头、同一固件、同一纸种，按原方式清洁后立即打印A。
等APP完成，换纸并恢复纸张起始位置，不擦拭、不换喷头、不改参数，发送相同图打印B。
不要在运动时换纸或推字车。记录两次间隔，保留每张整图和能看清短线的近照。
把A/B及C/M/Y方向标在纸的空白处，便于比较同一列的同一行。
本次不需要再次量CE，也不要求连续反复试印。

预期日志：

```
[NOZZLE_LINES_BEGIN] ... used_rows=192 ... length_columns=96 ... dots_each_color=18432 ...
[NOZZLE_LINES_PASS] channel=0 rows=0..191 expected_dots=18432
[CYAN_LINE_END] result=0 columns=1180/1180 ...
[NOZZLE_LINES_FEED] after_color=0 pulses=3170/3170 result=0
... channel=1 ... feed after_color=1 ... channel=2 ...
[NOZZLE_LINES_END] code=0 locked=0 ink_unverified
[PAGE_RESULT] ... code=0 ...
```

HC32每次仍1180列，other_bits与固定三槽dots之和应18432；不是旧三段条形统计。
若某条线缺失，只能定位到该行喷嘴没有在本条件下正常出墨，不能单独区分堵塞、
供墨不足、驱动问题或损坏；若固定地址/Section成组缺失，还要查公共驱动路径。
若第二张相同喷嘴短线更短/缺失增加，支持出墨随使用变化，但仍不是堵塞的唯一证据。

## 检查与回到完整图

17个主机脚本通过；新模式验证了按规格CSV反查192行每行96点、无额外域位、
完整任务校验、成功允许第二次、内存失败不运动、扫描/进纸失败后停止并锁定。
便携mapper通过M33对象编译；本环境未完成BK完整SDK构建或板上出墨验证。
参考图可用tools/make_nozzle_reference.py重建（主机cc与Pillow）。

检查后需恢复完整图时，保存本地变动后切回codex/printer-full-page-groups（7e642a4），
重新编译烧录BK即可；HC32不改。本检查不替代完整图片路径。
