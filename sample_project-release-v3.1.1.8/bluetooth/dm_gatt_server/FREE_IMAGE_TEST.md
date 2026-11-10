# APP完整图片自由测试

分支 `codex/printer-free-image`，仅更新BK。HC32保留已确认带ROW_AUDIT的3e7e161。

APP可选择自己的图片，使用现有图片转换和发送流程。仍接受原1180x1476及1536填充的BIN格式，
不是新增任意BIN尺寸支持。打印实际APP像素，不替换成固定线条或诊断图。
关闭第5条带隔离、关闭同一条带重复打印；保留原缩放、颜色、喷嘴映射和完整页走纸。
每个非空条带：上电静止等待2秒、原速去程打印、头OFF、空白回程；条带间按原方向进纸3170步。
不调整CE或供电，不增加预喷；启动不自动打印或运动。

## 更新

```sh
cd ~/armino/bk_avdk_smp
git stash push -m before-free-image -- :/sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server/ap/app_bluetooth/gatt/printer/printer_diagnostics.h
git fetch origin
git switch codex/printer-free-image
git pull --ff-only
git log -1 --oneline
cd sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server
make bk7258 SDK_DIR=~/armino/bk_avdk_smp
```

烧录新生成的固件并重启，启动标记应为 `APP_FREE_IMAGE`。
不要恢复旧诊断头文件。APP发送一次即打印该完整图一次，不自动重复整页。
发生通信/运动错误仍按原逻辑停止并锁定，重启并恢复机械起点后再试。
打印质量问题尚未解决，这次仅恢复自由换图能力。
