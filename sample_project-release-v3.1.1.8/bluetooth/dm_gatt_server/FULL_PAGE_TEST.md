# 完整图片打印路径：首次整页与走纸拼接验证

分支 `codex/printer-full-page`。暂缓频繁擦拭/堵塞专项，不再改HC32波形或做单色线反复测试。
HC32保持当前 `codex/printer-latch-reset` / `f5d53d4`；本次只更新BK7258。
本分支真正读取APP收到的完整图片点阵，不再用固件生成三段线替换图片。

## 当前实现

- 沿用已验证的BLE控制帧重组、原始BIN长度/CRC检查及所有权管理。
  worker在运动前再次核对存储缓冲的长度和CRC。
- 使用严格native BIN/整流TIFF RLE解析，不在图片字节里搜索ESC命令。
- 当前支持已确认的APP中质量格式：1180x1476源画布、8组192行CMY条带、末尾补零到1536行。
  printTimes不是1、尺寸/格式不符、有效数据出现在已知补零区，均拒绝且不运动。
- 完整解码、检查、缩放和分配打印工作区都在首次运动前完成。
- 使用规格书600喷嘴坐标逐bit映射，不再采用旧代码“非零字节同时开两个喷嘴”的错误路径。
- 沿用起步3000、巡航260、250步斜坡、单程7480步、每6步一列1180列、前进喷墨返回关喷头。
- 依据候选列距30um、喷嘴行距42.33um，预留最大61列颜色补偿，不增加横向行程。
  源图等比例适配为1119x992逻辑像素，物理范围约33.57x41.99mm（依据候选几何），分6个条带。
  这是完整内容的缩小打印，不是裁掉右侧补偿尾列，也不是承诺当前能打印满45x60mm。
- 二值点阵采用面积并集缩小以保留细线；已有半色调照片可能变深，墨量/色彩尚未标定。
- 每条带可打印全部192行，空白条带跳过字车扫描，但仍按页面位置进纸。
  条带之间只唤醒U5，U4关闭；进纸不长时间关中断，不读取nFAULT，不调用旧的关中断进纸循环。
- 成功完成整页后允许下一张；物理动作中失败则锁定到重启，要求人工恢复起点。
  不在失败后盲目返回，不自动退纸/出纸/寻零。

## 重要未标定项

纸张进给使用旧代码候选值：每192喷嘴行2340脉冲，方向引脚LOW（旧motor_feed_paper(...,1)）。
这相当于目标8.128mm，但实际距离及方向未验证，不能仅凭计数声称正确。
第一张完整结构图用于检查页方向、全部条带、纵向间隙/重叠及颜色位置。
如果出纸反向、卡住或碰边，立即断电，不用手推通电字车。
修好FPC并放好纸；完整图片比先前单次扫描多了实际进纸，起点必须留够约42mm的可打印纸长。

## 更新与编译

本地有颜色/几何参数修改时先保存，不要stash pop：

```sh
cd ~/armino/bk_avdk_smp
git stash push -m before-full-page -- :/sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server/ap/app_bluetooth/gatt/printer/printer_diagnostics.h
git fetch origin
git switch codex/printer-full-page
git pull --ff-only
git log -1 --oneline
cd sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server
make bk7258 SDK_DIR=~/armino/bk_avdk_smp
```

参数保持默认：FULL_PAGE=1，列距3000，方向-1，PAGE_FEED_STEPS=2340，PAGE_FEED_DIRECTION=0，
PAGE_TRANSFORM=0。不需要再改CHANNEL_MASK，整图始终打印收到的全部CMY数据。
只烧录BK7258，HC32不改。主机验证已通过，但本环境未完成完整SDK构建，最终以本地编译/实机为准。

## 首张图和操作

先用本仓库 `tests/page-images/01-full-page-grid.png`，不是先前那张只有一条黑线的图片。
图中有外框、TOP/END、1..6编号、斜线及三色条，覆盖整幅内容且避免大面积实心填充。
GitHub源图：
https://raw.githubusercontent.com/beyondxu/bk7258/codex/printer-full-page/sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server/tests/page-images/01-full-page-grid.png

保存到手机，用创彩APP图片打印，质量中、方向正向、不镜像，与此前协议测试一致。
放好新纸、恢复字车起点、两端断电重启。APP发送这张图，而不是用任何图片只触发固定图案。
开机不自动运动，收齐数据后开始整页。页面可能需要几十秒，直到PAGE_DONE/APP完成再取纸。
最多6次非空扫描、5次条带间进纸；空白条带可不扫描，末尾不自动出纸，电机关闭后人工取纸。
下一张可成功打印，但必须人工换纸并恢复正确起点；若某次失败，先重启恢复起点。

## 日志与判断

```text
[PAGE_PLAN] source=1180x1476 output=1119x992 swaths=6 columns=1180 max_delay=61 ...
[PAGE_DOTS] source_CMY=... resized_CMY=...
[PAGE_SWATH] index=1/6 y=0 dots_CMY=...
[CYAN_LINE_END] ...
[PAGE_FEED] after=1 pulses=2340/2340 result=0 distance_unverified
...
[PAGE_DONE] swaths=6 no_auto_eject ...
[PAGE_RESULT] ... code=0 transport_motion=PASS ...
```

CYAN_LINE是复用运动器的历史日志名称，不代表只印青色。
HC32旧ROW统计只统计固定的三个诊断槽；整页使用全部喷嘴，bars/outside/other_bits不再按
旧三段线标准解释。每个非空条带会有一次ON/OFF和1180列调用，统计不能证明实际出墨。

请提供：整张纸照片（带毫米尺）、两个MCU完整日志、纸张前进方向和是否卡顿。
重点看TOP/END及数字是否完整/镜像/上下颠倒、外框/斜线是否在条带边界断开或重叠。
不要同时调整多个参数。若左右镜像或上下翻转，后续可只改PAGE_TRANSFORM：
bit0镜像X，bit1翻转整页Y（0正常，1镜像X，2翻转Y，3两者），不反转马达方向。
进纸距离校准目标每条带8.128mm；需根据首张实测再修改PAGE_FEED_STEPS，不能靠照片猜整数。

## 主机预览与验证

`python3 tools/page_preview.py saved.bin --out page-preview.png` 编译同一C代码，
导出按物理像素比例显示的预览和尺寸/点数。预览仅用于核对数据，不要再送入APP作输入二次缩放。
完整图测试覆盖四边、底部像素、跨192行边界、全部喷嘴路由、长宽比、镜像/翻转、非法补零区，
以及格式/内存失败不运动、条带顺序和进纸次数、成功后再打印、失败锁定、worker释放再应答。
主机测试全部通过，新增可移植模块通过M33对象编译；完整BK固件、进纸距离、方向、喷嘴全数
实际出墨、墨量和最终照片质量均未实机验证。当前目标是跑通完整图并校准，不声称首张已正确。
