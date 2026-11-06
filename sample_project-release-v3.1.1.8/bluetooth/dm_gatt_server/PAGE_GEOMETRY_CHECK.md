# 完整图几何独立核对（主机工具，无固件修改）

基线：BK 7e642a4；HC32回退3e7e161，用户确认恢复出墨，CE约2.3us。
不重新启用分段独立CE实验，不改变驱动、进纸或图像参数。

工具verify_page_geometry.py先运行实际C解析/缩放/列生成，再从100字节列中按
CSV的地址、段、域和物理X/Y反向还原，不使用C行查找表做逆映射。
当前脚本固定核对默认3000列距、方向-1、first_nozzle=0、reverse_y=0。

测试输入为仓库结构PNG合成的未压缩BIN，不是APP任务563的真实BIN。
阈值转换与APP半色调不同，不能拿93016点去匹配用户日志79422等实际点数。
CSV来自规格书转录，因此本核对也不能独立证明CSV本身或板上接线完全正确。

结果：1119x992，6个条带，93016点，无漏点/多点/重复，反向还原图的
竖框、斜线、数字1..6、TOP/END完整。图和JSON保存在tests/page-images/geometry-check。
该结果支持软件几何一致性，不能证明喷嘴实际喷墨，也不证明物理拼接无缝。

每条带192行，理论进纸192*42.33um=8.12736mm，5次40.6368mm。
用户量得总40..42mm，仅在假定5次平均一致的条件下，每段误差范围约
-0.127..+0.273mm，相当于-3..+6.4行；不能据此继续盲调3170步。

运行需主机cc和Pillow，无需切换/编译板上固件：

```sh
python3 tools/verify_page_geometry.py --fixture tests/page-images/01-full-page-grid.png --out /tmp/page-geometry
# 若取得APP实际BIN，也可：
python3 tools/verify_page_geometry.py --bin received.bin --out /tmp/real-page-geometry
```

下一份物理证据应来自回退后同一结构图的近照，包含完整边框和毫米尺；
重点比较缺口在条带内部还是接缝处、左右竖框的缺口是否处于相同高度。
不能依据传输点数正常就把全部缺线归为堵塞或宣布图片已正确。
