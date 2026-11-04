# 字车 + 全零 SPI 列对比

基准为 `xqj/carriage-working` / `7f89a27`，并同步用户实机验证的
`xqj/motor-fixed` / `fd60189` 起步延时修正：1000改为3000。
新分支 `codex/printer-blank-spi`，不修改用户基准分支。
只更新 BK7258。HC32 保持 `codex/printer-bounded-line` / `f87a450`。

## 操作

```sh
cd ~/armino/bk_avdk_smp
git fetch origin
git switch codex/printer-blank-spi
git pull --ff-only
git log -1 --oneline
cd sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server
make bk7258 SDK_DIR=~/armino/bk_avdk_smp
```

保持默认参数：PRINTER_CARRIAGE_TEST=1、PRINTER_BOUNDED_PRINT=0，不用改诊断头文件。
如切分支因本地修改被拒绝，先保存改动，不使用 reset --hard。
烧录后把字车恢复到已验证的起始位置，两个 MCU 断电重启。不用 APP、不需要纸。
开机自检成功后自动进行一次往返。若持续卡住或碰到边界，立即断电，不反复运行。

## 与已验证基准的差异

原来的两个 shuttle 函数保留，并同步起步延时3000，新加单独的空白 SPI 测试路径并替换开机调用。
沿用基准的 GPIO 配置、升压等待60ms、细分111、SLEEP/RESET/ENABLE顺序，
单程7480步、250步加减速、3000→260→3000的延时参数和 STEP 高电平参数4。
基准里 delay_us 实际已调用 bk_delay_us，本分支保留其实现，不再替换延时。

唯一新增的前进节奏：先走200步，之后在第200+6*k步前（k=0..1179）
通过现有SPI握手发100字节全零列，再继续走步，最后走完余下行程。
200+1180*6+200=7480，没有增加步数。250步速度斜坡保留，SPI窗口会覆盖部分斜坡。
SPI握手会增加步间停顿，这是本次对比的对象；不保证它与纯运动速度相同。
返回仍为原来的无SPI梯形运动。

开始运动前显式发送 PCTLv1 OFF，HC32 喷头保持关闭，不发 ON；纸张电机不发步进脉冲。
本次验证 SPI 收包和握手对运动的影响，不验证实际喷墨函数耗时或带负载出墨。
不读取 nFAULT，不更改驱动芯片的硬件保护。
SPI握手失败即停止并下电，不执行整段盲目返回。

## 日志和反馈

正常 BK 日志包含：

```text
[BLANK_SPI_BEGIN] base=7f89a27 motor_fix=fd60189 start_delay_us=3000 columns=1180 bytes_each=100 zero_only=1 head=OFF feed=0 cycles=1
[BLANK_SPI_END] result=0 columns=1180/1180 forward=7480 reverse=7480 ...
```

HC32 应出现 PCTLv1 power=OFF，不应出现 power=ON。
确认实际字车是否平稳前进、返回、回到原位；应无出墨、无走纸。
请提供 BK/HC32 的完整开机日志和运动描述。软件脉冲计数不代表实际位移。
如果纯运动正常而本版卡顿，则重点比较新增SPI停顿；先不启用青色打印。

## 验证

主机测试直接运行实际原函数及新函数，比较两程全部延时参数和步数；
检查1180列均为零、列间6步、关喷头命令、通信失败停止且不整段返回。
原协议/解析/映射/桥接测试通过。未在此环境完成完整SDK固件构建或实机测试。
