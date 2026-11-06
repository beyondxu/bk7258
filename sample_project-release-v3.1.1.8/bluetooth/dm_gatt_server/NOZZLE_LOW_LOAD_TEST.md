# 当前分支入口

本分支为固定地址逐段波形对照，实际4条青色线、0进纸，见[SECTION_SCOPE_TEST.md](SECTION_SCOPE_TEST.md)。
下文仅保留历史版本说明。

# 当前分支入口

本分支为两批交错喷嘴对照，字车恢复原速，见[NOZZLE_BATCHES_TEST.md](NOZZLE_BATCHES_TEST.md)。
下文为历史版本说明，不作为当前分支的更新步骤。

# 喷嘴检查：降平均喷射频率对照

分支codex/printer-nozzle-low-load，基于14cd8b1可见短线检查版。
仅更新BK7258。HC32保持回退能出墨的3e7e161，不更改电压或CE/S时序。

## 唯一物理调度变化

打印去程且发送实际列数据时，将原梯形曲线的STEP低电平等待时间乘4：
起步3000→12000us，巡航260→1040us。STEP高电平仍4us。
原7480步、1180列、每6步一列、200步边距、250步加减速段均保持。
回程仍原速，进纸仍3170步且不降速；纯空白SPI测试/原始独立马达测试保持。

三色列数据逐字节保持原检查图：每色192条，每条96点，18432点/色。
不修改点阵加浓，不增加同点重复喷射，不增加单点加热宽度或电压。
本次降低的是平均喷射频率，同一列同时驱动的喷嘴数未减少。
实际列间隔包含SPI/握手耗时，不能称为精确四倍；源低电平总等待量从
2632780us变为10531120us，回程仍2632780us。
原CYAN_LINE_BEGIN的base_start/base_cruise表示乘倍率前的基线，实际倍率见NOZZLE_LOAD。

## 更新与打印

```sh
cd ~/armino/bk_avdk_smp
git stash push -m before-nozzle-low-load -- :/sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server/ap/app_bluetooth/gatt/printer/printer_diagnostics.h
git fetch origin
git switch codex/printer-nozzle-low-load
git pull --ff-only
git log -1 --oneline
cd sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server
make bk7258 SDK_DIR=~/armino/bk_avdk_smp
```

烧录后不要stash pop，默认NOZZLE_CHECK=1、NOZZLE_TIME_SCALE=4。
用与A/B相同喷头、同纸种，按A开始时相同方式擦拭，准备好纸后发送之前的TOP/END图一次。
实际仍打印固定三色喷嘴检查图（APP图片仅作为合法任务触发）。
保持一次任务3次往返/2次进纸，预计50～60秒，比原约27秒慢是预期行为。
不要因为去程变慢重复发送；正常回程应比去程快。最终仍等PAGE_RESULT=0。
先只打印一张慢速L，与原A/B比较，记录喷头是否相同、擦拭方式、擦拭至打印的间隔。
固件更新与清洁状态会影响可比性，不据单张变化断定唯一原因。

日志应见：

```
[NOZZLE_LOAD] forward_motor_delay_scale=4 start_low_us=12000 cruise_low_us=1040 return_scale=1 ...
... 3次NOZZLE_LINES_PASS与CYAN_LINE_END ...
... 2次NOZZLE_LINES_FEED ...
[NOZZLE_LINES_END] code=0 locked=0 ink_unverified
[PAGE_RESULT] ... code=0 ...
```

每次HC32接收仍1180列，other_bits加固定槽统计应18432点/色。
点数是传输/调用审计，不是实际墨滴数。
拍L的整张和三色短线近照，附两端完整日志，无需再量CE即可完成本次负载比较。

## 怎么解释

- L比A/B相同位置短线恢复：支持与频率/负载相关的供墨、气泡、温度或电源恢复因素，
  不单凭此断言一定补墨不足或一定脉宽不足。
- 固定行仍缺失：保留局部堵塞、弱喷嘴及公共分段/域驱动可能，不能排除时序问题。
- L更差：记录结果，退回14cd8b1原速；不延长脉宽弥补。

这是出墨原因对照，不是宣称防堵修复或完成图片质量校准。
检查结束后仍可回codex/printer-full-page-groups / 7e642a4恢复完整图片。

## 验证范围

18项主机脚本通过。新增检查直接执行实际运动函数，分别验证倍率1/4、
两种速度逐列同数据/同位置/同脉冲数、4us高脉冲与50us方向建立不变、
原速回程/空白测试不变、SPI失败立即停止。
完整图、检查图生成、第二任务应答与异常锁定的原回归通过。
尚未在本环境完整构建BK SDK或验证实际墨滴与真实喷射间隔。
