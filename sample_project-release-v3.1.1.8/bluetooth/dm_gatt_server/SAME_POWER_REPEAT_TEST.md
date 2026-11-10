# 原第5条带：同一缓冲区、同一次上电，连续打印两份

BK分支 `codex/printer-repeat-same-power`，基于 `ddd932e` 的第5条带隔离试验。
只更新BK。HC32继续使用 `3e7e161`；不更新CE、不调VL/VP、不增加预喷。
使用原来的 [F2上下横线图片](tests/page-images/frame-parts/F2-horizontal-edges.png)。

## 操作

```sh
cd ~/armino/bk_avdk_smp
git stash push -m before-same-power-repeat -- :/sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server/ap/app_bluetooth/gatt/printer/printer_diagnostics.h
git fetch origin
git switch codex/printer-repeat-same-power
git pull --ff-only
git log -1 --oneline
cd sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server
make bk7258 SDK_DIR=~/armino/bk_avdk_smp
```

烧录新生成的BK固件并重启，字车在原有起点，准备足够纸张。不要恢复旧诊断头文件。
启动标记应为 `APP_REPEAT_SWATH5`。
APP只发送F2一次，不要发送F5上移图。固件自动完成两份，约20秒。
两份都打印原第5条带的下横线；不会打印F2的上横线。
两次间走纸3170脉冲，按现有标定约8.13mm。实际距离仍需看纸。
不接示波器，保持同一喷头、供电和擦拭条件；不要两份之间人工干预。

## 软件实际流程

一次接收并校验BIN → 解析/缩放一次 → 只渲染原第5条带一次 →
保留同一个118000字节列缓冲区，不变更喷嘴行、列位置或CMY位 →
头OFF → 初始化电机/升压 → 头ON → 静止等2秒、不发列 →
第一次原速去程打印1180列 → 空白回程 → 只进纸3170步 →
第二次原速去程打印同一缓冲区1180列 → 空白回程 → 头OFF → 关闭升压。

头ON后至两份完成，中间不发送头OFF/ON，不重新初始化HC32。
走纸函数不能调用普通电机GPIO初始化（该初始化会拉低升压使能），
因此新增保留电源的走纸入口，仅停放U4并运行U5，再只唤醒U4。
相同缓冲区在每次扫描前后计算CRC，不在脉冲循环中添加CRC计算。
每次发送1180列；回程及走纸不发列SPI。
出现通信失败、短进纸、计数不完整或缓冲区CRC变化则下电并停止，
不再盲目回程或进纸；任务锁定，需要重启并恢复机械起点后再试。

## 留下什么结果

请保留一整组两条线的照片和这一任务的BK、HC32日志。
BK应有：

- `REPEAT_COPY_BEGIN copy=1/2` 与 `copy=2/2`，bytes均118000，crc32相同。
- 两个 `REPEAT_COPY_END` 的crc32也相同，各columns=1180，forward=7480、reverse=7480。
- 中间一个 `REPEAT_FEED pulses=3170/3170 result=0`。
- `REPEAT_POWER_END result=0 completed=2/2`、`PAGE_REPEAT_DONE`。

HC32在这两份之间不再输出power=OFF；应只在末尾下电并打印一次汇总：
`ROW_AUDIT columns=2360 fired=2360 invalid=0 canceled=0`。
固定参考喷嘴统计是两份合计，不能直接拿它的first/last当单条坐标。
HC32首次非零COL调试日志只打印一次是现有行为，不表示第二份没发数据。

若两份明显不同，即可确认在同一BK点阵、同一组喷嘴及连续上电命令下仍有不稳定出墨，
下一步集中查接触/驱动/时序/带载供电，而不是重新猜图片缩放。
若两份一致，仍不能证明全部喷嘴或驱动正常；需结合下一组任务是否又变化继续判断。
CRC仅验证BK内存缓冲区，列数仅验证程序执行，不能证明HC32接收逐字节一致或墨滴实际形成。
本测试也改变了两份之间的时间和电机负载，不能单独定位某个硬件元件。

## 验证范围

18项主机测试通过，包括真实映射的第5条带逐字节比较、重复控制器、实际字车步进循环、
正常/保留电源走纸入口及错误停止。重复控制器和字车循环另经Cortex-M33 ARM编译检查。
此环境没有完整BK SDK链接/烧录条件，最终SDK构建和纸面结果仍需在实机确认。
