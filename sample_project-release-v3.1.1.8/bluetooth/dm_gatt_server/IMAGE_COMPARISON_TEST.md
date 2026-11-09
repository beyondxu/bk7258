# 换真实APP图片对照，不换固件

适用当前BK c60eafc（完整图、CMY同扫、上电静置2秒）与HC32 3e7e161。
本提交只有PNG、生成脚本及说明，板上不需拉代码/重编译/烧录。
图片由APP正常转换并传输，固件不内置替代图，不改变电压/时序/运动参数。

先测02、03各一张，04留待结果需要时再测，不要求示波器或外部机器。
保持同一喷头、纸种、纸张初始位置、清洁方式及擦拭至打印间隔，并记录有无擦拭。
APP选原图、不加滤镜、不镜像、不旋转、质量中；裁剪框覆盖整张PNG，不截屏替代原文件。
等上一任务PAGE_RESULT完成，再换纸发下一张，不同时更换任何打印参数。

## 图片

1. tests/page-images/comparison/02-bold-grid.png
   BOLD粗黑框、6条横线与斜线，线宽12源像素；粗线经缩放后由多行喷嘴共同组成。
   对照旧TOP/END细线：若明显完整，支持细线对弱喷嘴/喷射状态敏感，不能直接断言原APP丢点；
   面积增大、负载和喷嘴冗余均有变化。粗黑色在当前三色头上由CMY形成，不是额外黑色域。
2. tests/page-images/comparison/03-cmy-same-shapes.png
   C/M/Y三列各有相同Y位置的边框、6横线和斜线，采用纯青/品红/黄RGB源色。
   APP转换和半色调可能改变具体点数，不能当作原生单域命令。
   若三色同一位置都缺，优先比较公共分段/条带拼接/几何；若某色独有缺口，优先查该色域/供墨，
   都只是定位线索，不能单凭一张归因。不要用黄的手机照片亮度直接比青色浓度。
3. tests/page-images/comparison/04-cmy-shift-down.png
   将03整体下移72个源像素，画布/尺寸/形状不变，没有剪掉任何非白像素。
   当前992/1476缩放下约移动48～49输出行，约2.05mm，会使图案改用不同的喷嘴行相位。
   仅在03结果需要定位时再打印，不先连打三张。
   保持相同纸张初始定位与APP裁剪/缩放，比较缺口是否跟着具体图形移动、是否仍按条带重复；
   清洁和喷头状态变化、插纸误差、APP裁剪均会影响解释，不凭此单独证明软件或堵塞。

PNG均1180x1476、RGB无透明。03/04通过逐像素位移检查，底部白边足够，不裁非白内容。
生成工具tools/make_compare_images.py使用Pillow/DejaVu字体，仅主机生成资产，不修改固件。
本次未在实机验证新的纸面图。原完整图和运动回归未因这些资产重新运行。

手机原图链接：
https://raw.githubusercontent.com/beyondxu/bk7258/codex/printer-full-page-settle/sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server/tests/page-images/comparison/02-bold-grid.png
https://raw.githubusercontent.com/beyondxu/bk7258/codex/printer-full-page-settle/sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server/tests/page-images/comparison/03-cmy-same-shapes.png
https://raw.githubusercontent.com/beyondxu/bk7258/codex/printer-full-page-settle/sample_project-release-v3.1.1.8/bluetooth/dm_gatt_server/tests/page-images/comparison/04-cmy-shift-down.png

请把02纸标G1、03纸标G2，提供整张及清晰缺口近照、两端日志。
先确认PAGE_DOTS/PAGE_SWATH随新图片变化，并把每段HC32全部有效位与BK计划点数核对。
不是继续用日志计数宣布所有喷嘴都喷射，也不是声称这组新图片本身能修复供墨。
