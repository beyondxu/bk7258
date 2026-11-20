> Current default: automatic boot cleaning is temporarily disabled
> (`PRINTER_BOOT_WASTE_CLEAN=0`) at the user's request because of startup delay
> and no clear improvement. Head-OFF boot homing, normal print preparation,
> S1–S4 interlacing and explicit maintenance commands remain available.
> The instructions below document the optional enabled profile.

# V36 开机自动执行一次废墨垫清洗

用户当前APP没有清洗按钮，要求直接在开机流程执行。现在不依赖APP：在HC32通信自检成功、原字车无喷墨定位成功之后，直接调用既有废墨清洗任务一次，完成后才启动BLE／打印worker。没有在BLE回调里执行维护，也没有为开机动作伪造APP任务或BIN。

## 第一次测试

更新并烧录BK后，断电重启一次。观察：先定位，随后在海绵上方停稳吐墨，再慢速擦拭并回原密封槽。此动作不进纸，不需要手机操作。保存`BOOT_WASTE_BEGIN`、`WASTE_POSITION`、`WASTE_DONE`和`BOOT_WASTE_DONE`日志。正常结束应有`BOOT_WASTE_DONE code=0`，之后蓝牙才开放，连接APP打印同一张图比较出墨。

HC32保持79ba462，无需再次修改。普通图片打印的速度、行程和进纸参数不变。

## 范围

- 每次开机在满足自检条件后仅尝试一次；失败也消耗此次尝试，不自动重试。
- 先保留原来的开机无喷墨往返定位，再执行V35清洗定位／吐墨／回位流程；因此开机动作会比纯定位版多。
- 沿用V35的OC3定位、内退374步到PRINT_RIGHT、静止吐墨、每喷嘴32次／600喷嘴／12组、673us列周期、慢速擦拭和密封槽停靠。没有改变电气驱动脉宽、VP、剂量或纸张位置。
- 自检／初次定位失败不启动清洗。清洗失败关闭后续打印权限，BLE仍可启动用于观察；故障不通过重喷或追加运动掩盖。
- 关闭`PRINTER_BOOT_WASTE_CLEAN`可禁用此开机动作；现有维护命令仍保留。禁用整个维护／废墨功能时，此开机功能默认也禁用。
- 这不是每10分钟自动清洗，也不宣称解决密封或供墨根因；先验证这一次维护能否改善闲置后出墨。

## 验证

实际ap_main启动调用片段和开机包装函数的主机测试覆盖：定位在清洗之前、清洗在BLE之前、成功／失败都只尝试一次、通信或定位失败不清洗、清洗失败禁止后续打印、禁用配置不动作。既有废墨任务与准备顺序测试通过，BK六个SDK ARM对象编译通过。海绵接墨、机械回位和出墨恢复仍需此次实机观察。
