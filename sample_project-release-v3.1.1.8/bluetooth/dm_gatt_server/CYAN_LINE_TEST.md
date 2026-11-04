> `codex/printer-solid-line` 当前使用固件生成的连续单色线，步骤以 [SOLID_LINE_TEST.md](SOLID_LINE_TEST.md) 为准；下文为原APP单行测试说明。

# APP 黑横线 → 单色青线：沿用实机通过的运动

分支 `codex/printer-cyan-line`，运动基准 `29a973b`（空白SPI实机往返通过），
包含用户起步修正 `fd60189`。只更新 BK7258；HC32 保持 `f87a450` / PCTLv1 配套固件。

## 获取、编译和操作

```sh
cd ~/armino/bk_avdk_smp
git fetch origin
git switch codex/printer-cyan-line
git pull --ff-only
git log -1 --oneline
cd sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server
make bk7258 SDK_DIR=~/armino/bk_avdk_smp
```

如果切分支因本地修改失败，先保留改动，不强制覆盖。
无需修改参数：本分支默认 CARRIAGE_TEST=0、BOUNDED_PRINT=1、CHANNEL_MASK=1、STARTUP_DIAGNOSTIC=0。
不要把之前的诊断参数 stash pop 回来。保留空白SPI分支作对照。

只烧录 BK7258。放好纸张、把字车恢复到上次通过测试的起点，两个MCU断电重启。
开机只进行SPI自检，不自动运动、不打开喷头。应看到 `[SYS] APP_CYAN_LINE ... no_startup_motion`。
APP连接后发送原来同一张 `tests/audit-images/03-black-horizontal-1px.png`，
质量中、不镜像、方向正向，与先前接收审计保持一致。

只发送一次。它会先打印BIN审计和计划日志，可能等待几秒，之后才开启喷头执行。
一个开机周期最多尝试一次实际打印；再试需两个MCU重新启动，先恢复字车起点。
若出现持续卡顿或碰到边界，立即断电，不用手推动通电中的字车。

## 实际行为

完整接收和CRC校验后，严格解析BIN，选择条带起点y=576、条带内第175行（整图y=751）
的青色点；不打印品红和黄色，不执行整页走纸。
需要所选点至少64个，映射后所有有效点均在200..999列；否则拒绝运动。
喷嘴映射仍采用候选几何参数，尚未完成全部喷嘴或颜色标定。

与已经通过的空白SPI测试共用同一个运动循环、U4上电和下电函数：
起步延时3000、巡航260、斜坡各250步、单程7480步、前进每6步发送一列共1180列，
返回不发列；只唤醒字车，不发走纸脉冲，不读取nFAULT。
本次差异为列数据从全零变为已验证的单色数据，并在升压稳定后发送喷头ON命令。
前进完成后先发送OFF，再暂停200ms并返回；返回后暂停300ms，关闭字车和升压。

HC32保持原喷墨波形。喷头ON后，HC32处理列时会执行喷墨函数，
因此真实列处理时间可能比喷头OFF的空白测试长；本次需要同时验证运动和墨迹。
SPI/电源控制失败时停止下电，不盲目执行整段返回。

## 预期与反馈

BK日志顺序：

```text
[AUDIT_RX] ... PASS
[LINE_PLAN] result=0 ... mask=1 dots_CMY=非零,0,0 ...
[CYAN_LINE_BEGIN] motion_base=29a973b start=3000 cruise=260 ...
[LINE_DONE] sweep_and_return=1 feed=0 ink_unverified
[CYAN_LINE_END] result=0 columns=1180/1180 forward=7480 reverse=7480 ...
[LINE_RESULT] ... code=0 motion_transport=PASS ink_unverified
```

HC32应出现 PCTLv1 power=OFF、power=ON、首次有效列、power=OFF；
默认映射首次有效列应为 slot=15、data=0x02、addr=4、section=4。
预期为一条青色点线、字车平稳往返、不走纸。源图是黑线，软件此时只选其青色分量。
受APP抖动影响，点数不要求固定为此前的370等值，线也可能由离散点组成。
软件PASS不证明实际出墨正确。请提供两个MCU完整日志、纸张照片和实际运动情况；白纸也如实反馈。

## 验证与未完成项

主机测试直接比较原基准和共用运动循环的全部延时参数和步数，检查青色列的逐字节传输、
OFF/ON/OFF顺序、返回前关喷头、SPI或电源命令失败不盲目返回、数据范围验证和每次开机只尝试一次。
BIN解析、CRC、接收协议、喷嘴映射等回归测试通过。未在此环境完整构建SDK或实机验证。
本版仍不是完整照片打印；喷墨波形、全部喷嘴对应、走纸和CMY重合需要后续逐项确认。
