2.字体bin文件生成

使用记事本编辑RUN_Font-CRC32.BAT文件，第一个是字体版本号固定三个字符（比如：1.3），第一个是字体27文件名，第二个是字体39文件名，第三个是要生成的固件名,根据实际情况修改，完成后保存双击文件运行生成所需文件

合并后的文件结构（小端模式）

CRC32		---4Byte(计算范围从version到file2结束)
version		---4Byte（比如：1.3）
file1_size	---4Byte
file1_offset	---4Byte
file2_size	---4Byte
file2_offset	---4Byte
file1		---file1_size Byte
file2		---file2_size Byte
-----------------------------------------------------