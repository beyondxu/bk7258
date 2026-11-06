# 纸面分段/地址对照：不用示波器

分支codex/printer-section-paper。只更新BK7258；HC32保持能出墨的3e7e161。
不抓波形，不延长CE、不改变加热时序或电压。由纸面分布缩小原因范围，不直接确认堵塞。

## 实际打印内容

APP仍发送原TOP/END图一次作为合法任务触发。此固件完整校验BIN，但生成固定纸面对照图，
不打印APP预览：只有青色D2，S1～S4各25个地址，共100个喷嘴，每个喷嘴96个连续点。
不同于之前192行图，本次包含D2全部100喷嘴（四个Section各25），不检查D1或M/Y。
每段在纸面分成5列、每列5条短线，按喷嘴物理Y排序后交错摆放。
短线约2.88mm，同列中心距约0.423mm，四段总体高度约8.38mm，图宽约24.5mm。
每段一次往返，4次往返、不进纸，预计约26秒；启动不自动打印。
最多一列5个有效点且只属于当前段，不同时使能同地址的其他段。
每段25*96=2400点，整图9600点。HC32的采样/清零/地址轮询方式保持。

纸面不打印文字编号，避免缺墨喷嘴让编号本身失读。每条线的地址/段编号见
`tests/page-images/section-paper/reference.png`与`address-legend.csv`。
参考图为便于阅读放大了Y并在Section间留白；实际纸面没有这些文字和额外留白。
从上往下为S1、S2、S3、S4，沿用已验证完整图的Y方向。
例如同段内有正常线也有缺线，说明该段并非完全失效，但不能区分局部堵塞、损伤或地址驱动。

## 更新与打印

```sh
cd ~/armino/bk_avdk_smp
git stash push -m before-section-paper -- :/sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server/ap/app_bluetooth/gatt/printer/printer_diagnostics.h
git fetch origin
git switch codex/printer-section-paper
git pull --ff-only
git log -1 --oneline
cd sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server
make bk7258 SDK_DIR=~/armino/bk_avdk_smp
```

烧录后不stash pop；默认SECTION_PAPER=1、SECTION_SCOPE=0、NOZZLE_TIME_SCALE=1。
装好喷头，不接示波器。用同一个喷头与相同纸种，按之前相同方式擦拭后，
尽快发送一次APP任务。先只打印一张，标记T，并记擦拭至开始打印的间隔。
等待APP完成后取纸，提供整图和短线清晰近照；毫米尺靠近图案可帮助判定缺行位置。
暂时不要为了“补全”而反复打印或改变参数。

```
[SECTION_PAPER_BEGIN] ... C_D2 addr=1..25 sections=1..4 nozzles=100 dots_each_nozzle=96 sweeps=4 feed=0 ...
[SECTION_PAPER_PASS] section=1 addresses=25 expected_dots=2400 max_bits_per_column=5 feed=0
... section=2/3/4 ...
[SECTION_PAPER_END] code=0 locked=0 ink_unverified
[PAGE_RESULT] ... code=0 ...
```

每次HC32仍1180列；other_bits加固定统计之和应2400，不能用ROW_C=0判断没有青色数据。
传输/调用统计不等于实际墨滴。某段失败停止后续段，锁定到重启；成功允许下一任务。

## 结果如何使用

- 同段内固定少数地址缺失、其他地址正常：优先定位对应喷嘴和地址路径。
- 同段内大多数地址一起偏弱或缺失：保留公共S/域驱动、区域供墨、喷头状态等因素。
- 不同段同地址反复缺失：查地址公共路径，同时保留该地址对应喷嘴状态。
- 清洁或重复打印后位置明显变化：支持状态相关的出墨问题，但不单独证明堵塞。

本图只固定D2，不把结论外推到所有CMY域，不根据单张纸面延长CE。
若缺失过多不能可靠辨认行号，明确记录不能定位，不凭可见线从头顺数编号。
回完整图可切回codex/printer-full-page-groups/7e642a4重新编译烧录BK，HC32不改。

## 验证范围

20项主机脚本通过；新增按规格CSV独立核对全部100个D2喷嘴，地址/分段不重不漏，
每个喷嘴96点、每段2400点、每列上限5点；实际任务控制器校验4段顺序、
输入/内存失败无运动、物理失败停止锁定、成功允许下一任务。
映射模块通过M33对象编译。未完成完整BK SDK构建或板上喷墨验证。
