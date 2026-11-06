# 同一批喷嘴，交换短线左右列

分支codex/printer-paper-reverse-columns，基于fa114bc的T纸面对照版。
只更新BK，HC32保持3e7e161。不要求示波器，不改CE/S或电压。

## 这一次只改哪里

同一C/D2、100个喷嘴、原S1..S4顺序、相同Y高度、每喷嘴96点、相同原速字车。
仅把5列的位置反排：原第1列到第5列、原第2列到第4列、原第3列不变。
每条线保持左右朝向与长度；没有反转马达方向，也没有翻转Y、换喷嘴或重复喷射。
非空列窗口及每列总负载仍相同，每段2400点/总9600点，4次往返、不进纸约26秒。

| 新R纸左到右列 | 对应原T纸列 |
| --- | --- |
| 1 | 5 |
| 2 | 4 |
| 3 | 3 |
| 4 | 2 |
| 5 | 1 |

这会改变每组喷嘴在扫描过程中的打印时间/纸面X，但喷嘴及Y位置不变。
比较同一喷嘴需按上述列关系寻找相同高度，不能拿T与R的左列当成相同喷嘴。
纸上仍只有固定青色短线，不打印APP预览的TOP/END或文字编号。
R参考图/地址表位于tests/page-images/section-paper-reverse，参考图文字与分段留白不打印。

## 更新

```sh
cd ~/armino/bk_avdk_smp
git stash push -m before-paper-reverse -- :/sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server/ap/app_bluetooth/gatt/printer/printer_diagnostics.h
git fetch origin
git switch codex/printer-paper-reverse-columns
git pull --ff-only
git log -1 --oneline
cd sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server
make bk7258 SDK_DIR=~/armino/bk_avdk_smp
```

烧录后不stash pop；默认SECTION_PAPER=1、SECTION_SCOPE=0、REVERSE_COLUMNS=1、TIME_SCALE=1。
装同一喷头，用与T相同纸种、擦拭方式与纸面朝向，记录擦拭至打印时间。
APP发送原TOP/END图片一次作为触发，打印完成后标为R。保留T，不需要重新打印T。
先只打印一张R，近拍全部5列及上下边界，旁边放毫米尺。

关键日志：

```
[SECTION_PAPER_ORDER] reverse_columns=1 expected_column_mapping=5,4,3,2,1 nozzle_rows_unchanged waveform_unchanged
[SECTION_PAPER_PASS] section=1 addresses=25 expected_dots=2400 max_bits_per_column=5 feed=0
... section=2/3/4 ...
[SECTION_PAPER_END] code=0 locked=0 ink_unverified
[PAGE_RESULT] ... code=0 ...
```

每段HC32的other_bits与固定槽计数之和仍2400。
ROW_C的固定喷嘴位于第3列，未移动，计数96与列520..615应保持。
头没有实际出墨也可能统计正常，不能拿日志代替纸面判断。

## 怎样解释

- T左列缺失的相同喷嘴在R右列仍缺失：支持固定喷嘴/地址/喷头区域因素，不能直接断言堵塞。
- 换喷嘴后仍纸面左侧更差：支持打印起始状态、随位置变化的连接或其他过程因素，
  不单凭此断言供墨不足或软件错误。
- 原T/R第3列也明显变化：说明喷头状态/清洁/重复打印等变化影响对照，降低结论强度。
- 缺线过多无法对齐时明确记录，不按可见线顺数强行编号。

## 验证

21项主机脚本通过。新增逐bit对照：全部100个喷嘴只平移到反向列，
每喷嘴的槽/域/点数/Y顺序不变，负载窗口一致，中列不变，非法排序不改缓冲。
实际调度器检查原地址1/2已分别移至右/左、4次顺序及失败停止。
映射模块通过M33对象编译。未完成完整BK SDK构建或板上出墨验证。
