# 第一轮只验证 APP → BK7258 的数据

用户确认的原始现象：APP中的一条黑线，打印后成为多条不同颜色的线，不是多条黑线。
本分支 `codex/printer-rx-audit` 先检查接收数据，不调试喷头/电机，不要求导出APK私有目录。

## 两层判断，不能混淆

1. 传输：开始命令声明的BIN长度/CRC32，与BK实际保存数据的长度/CRC32比较。APK确实对发送的整个BIN计算标准CRC32，字段不是预留帧尾CRC。匹配是强校验证据，但不是逐字节比较的数学证明。
2. 转换：从BK串口完整导出BIN，在电脑上解压、按颜色解析、还原逻辑位图，观察黑点/黑线在CMY通道的像素坐标。如果日后能取得APK原生BIN，可用工具的`--apk-bin`做严格逐字节比较；目前不以此为前提。

源图一个黑像素，不保证输出恰好三个同坐标墨点：APK还会等比缩放、加白边、JPEG重编码和颜色处理/抖动。不要把源图像素、逻辑墨点、物理喷嘴三者混为一谈。先记录转换输出，再建立几何预期。

## 如何更新及测试

保存本地诊断宏修改后，切换BK到新分支。两种方式任选：

```sh
# 本地已经有当前BK仓库，且已保存/暂存本地修改时：
git fetch origin
git switch --track origin/codex/printer-rx-audit
```

或者直接从GitHub下载新分支ZIP；不要把整个SDK的新旧文件混合覆盖。

`printer_diagnostics.h`默认`PRINTER_DATA_AUDIT=1`；它优先于启动诊断0/1/2/3/4。
此模式关闭BK升压使能，跳过HC自检、SPI初始化和运动/喷墨启动测试；接收后的工作线程仅做CRC和导出，不调用旧打印引擎。
HC固件不需要改，也可以不参与本轮。使用既有SDK_DIR命令重新编译BK，烧录后重启。

预期：`[AUDIT] receive-only; HC32 link test, motor and ink diagnostics disabled`。
先用原APP连接设备，确认能查询到空闲状态。**不要在本轮关闭审计宏来打印。**

第一张只发送 `tests/audit-images/01-white.png`，保持默认相纸、SDK printTimes=1参数路径，不选滤镜/文字/贴纸。
手机通过GitHub下载原PNG再选图，不用聊天截图或缩略图。原APP可能会再次缩放和JPEG编码，这一影响需要记录。
完整保存BK串口文本，从`AUDIT_START`一直到`AUDIT_RESULT`，首次只发这一个任务的日志给我们。
白图确认之后再发单像素黑点，随后单像素横线、竖线，逐张分析。

测试图源像素：白图0；点(590,738)共1个；横线y=738、x=200..979共780个；竖线x=590、y=400..1075共676个。
PNG尺寸1180×1476，生成器为`tools/make_audit_images.py`，源文件自身没有JPEG损失。

## 日志与PC工具

```text
[AUDIT_START] task=... expected_len=... expected_crc32=...
[AUDIT_RX] ... received=... expected=... crc32=... expected_crc32=... PASS
[AUDIT_BEGIN] len=... crc32=... dump_len=...
[AUDIT_HEX] 00000000 ...
...
[AUDIT_END] ... COMPLETE
[AUDIT_RESULT] ... PASS (NO PHYSICAL PRINT)
```

`AUDIT_RX`校验成功保存的流，`AUDIT_RESULT`重新对PSRAM实际保存的缓冲区计算CRC。
worker释放缓冲后才允许下一任务，避免下一页/断连释放正在导出的数据。
APP可能显示“打印完成”，这里只表示接收审计完成，**没有实际打印**。

在电脑上运行（Python3标准库即可）：

```sh
python3 tools/audit_bin.py bk-log.txt --log --out audit-white
# 如果将来能取得APK原始BIN：
python3 tools/audit_bin.py bk-log.txt --log --apk-bin app.bin --out audit-white
```

工具重建`bk_received.bin`，生成`summary.json`、`C.pbm/M.pbm/Y.pbm`、`composite.png`（另保留PPM）。
合成PNG可直接查看，PBM可用支持该格式的图片工具查看；也可以直接把日志发给我们分析，不要求你运行Python。
逻辑预览没有物理喷嘴地址/颜色排布补偿，因此能用于区分“收到的逻辑位图就已分色”与“后续映射/喷头链路引入分色”。

串口导出默认最多65536字节；更大BIN仍校验完整CRC，但日志标记TRUNCATED，PC工具拒绝把截断内容当完整BIN。
若首张日志出现TRUNCATED，请先发摘要，不要反复重打。可按文件实际长度适当提高PRINTER_AUDIT_DUMP_MAX_BYTES，注意UART时间和APP120秒结果等待。
本轮完整BIN最多4MiB，超出会拒绝开始，不会缩短数据冒充完整接收。

## 本次软件修正与限制

- 控制帧支持跨BLE回调重组（包括20+13字节的33字节开始帧），也支持同一回调内连续帧。
- 严格检查控制帧和TLV边界，要求任务ID、长度及CRC三个参数齐全。
- 二进制阶段按声明长度收取，不扫描BIN中的控制帧或ESC字节。
- 拒绝接收计数与真实写入不一致、错误CRC和不匹配的结束任务ID。
- 正在导出时新任务被拒绝；发送“正在处理”后才唤醒worker，完成后回复最终审计结果。
- 若传输中途丢包，提前到来的结束帧也会被当成尚欠缺的BIN字节，最终可能等到APP超时；这由无转义、长度定界协议决定。此时断开重连后再测，不能通过扫描帧头猜测补包。
- PC解析器仅支持已用原生库单像素实验验证的相纸Color1/192行/水平参数0格式，以及该原生库的TIFF式压缩流。未知命令/非零水平参数会明确报错，不输出猜测的预览。
- 若底层格式尚不支持，已重建的BIN仍保留；继续分析格式，不代表传输一定失败。

测试：实际协议源码的主机模拟通过分片/粘包、BIN含控制帧字节、CRC错误、TLV越界、缓存写失败与worker所有权检查；实际worker代码通过CRC/完整导出/不进入物理打印引擎检查；PC工具通过白图、单黑点、横/竖线、压缩、ESC载荷、坏日志与截断拒绝检查。
云端没有完成BK整机固件构建或真机BLE传输验证，仍需本地编译和第一张审计日志。
