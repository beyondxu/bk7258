# FFmpeg 图片预解码工具

[toc]

## 概述

本工具是在 Windows 平台上利用 FFmpeg 将 JPEG 或 PNG 图片提前预解码为二进制文件的实用工具。通过将图片预先解码为目标格式，可以减少嵌入式系统运行时的解码开销，提高显示性能。

## 功能特性

- **JPEG 转 RGB565**：将 JPEG 图片解码为 RGB565 格式（小端序）的二进制文件
- **PNG 转 ARGB8888**：将 PNG 图片解码为 ARGB8888 格式的二进制文件
- 批量处理：支持一次性处理目录下的所有图片文件

## 目录结构

```
ffmpeg_bat/
├── bin/
│   └── ffmpeg.exe              # FFmpeg 可执行文件
├── jpeg2rgb565/                # JPEG 转 RGB565 工具目录
│   ├── run.bat                 # JPEG 转 RGB565 批处理脚本
│   ├── example.jpg             # 示例 JPEG 图片
│   └── example_output_rgb565.rgb  # 示例输出文件
└── png2argb/                   # PNG 转 ARGB8888 工具目录
    ├── run.bat                 # PNG 转 ARGB8888 批处理脚本
    ├── example.png             # 示例 PNG 图片
    └── example_output_argb.rgb   # 示例输出文件
```

## 使用方法

### JPEG 转 RGB565

1. 将需要转换的 JPEG 图片（.jpg）放入 `jpeg2rgb565` 目录
2. 双击运行 `run.bat` 脚本
3. 脚本会自动处理目录下所有的 JPEG 图片
4. 处理完成后，会在同一目录下生成对应的 `.rgb` 文件，文件名格式为：`原文件名_output_rgb565.rgb`

### PNG 转 ARGB8888

1. 将需要转换的 PNG 图片（.png）放入 `png2argb` 目录
2. 双击运行 `run.bat` 脚本
3. 脚本会自动处理目录下所有的 PNG 图片
4. 处理完成后，会在同一目录下生成对应的 `.rgb` 文件，文件名格式为：`原文件名_output_argb.rgb`

## 输出文件处理

生成的 `.rgb` 二进制文件可以通过以下工具转换为 C 语言数组：

### HxD 工具

1. 下载并安装 HxD 十六进制编辑器
2. 使用 HxD 打开生成的 `.rgb` 文件
3. 选择"文件" → "导出" → "C 源代码"
4. 设置数组名称和格式，导出为 C 语言数组
5. 将生成的数组代码复制到项目中使用

### 其他工具

也可以使用其他十六进制编辑器或自定义脚本将二进制文件转换为 C 语言数组。

## 注意事项

1. **FFmpeg 依赖**：本工具依赖 FFmpeg，FFmpeg 可执行文件位于 `bin/ffmpeg.exe`
2. **Windows 平台**：本工具仅适用于 Windows 操作系统
3. **文件覆盖**：每次运行脚本会先删除已存在的输出文件，然后重新生成
4. **格式说明**：
   - JPEG 转换输出格式为 RGB565（小端序，rgb565le）
   - PNG 转换输出格式为 RGBA（ARGB8888）

## 示例

### JPEG 转换示例

输入文件：`example.jpg`

运行 `jpeg2rgb565/run.bat`

输出文件：`example_output_rgb565.rgb`

### PNG 转换示例

输入文件：`example.png`

运行 `png2argb/run.bat`

输出文件：`example_output_argb.rgb`

## 技术说明

### RGB565 格式

RGB565 是一种 16 位颜色格式，每个像素占用 2 个字节：
- 红色（R）：5 位
- 绿色（G）：6 位
- 蓝色（B）：5 位

本工具使用小端序（Little Endian）存储。

### ARGB8888 格式

ARGB8888 是一种 32 位颜色格式，每个像素占用 4 个字节：
- Alpha（A）：8 位
- 红色（R）：8 位
- 绿色（G）：8 位
- 蓝色（B）：8 位

## 常见问题

**Q: 转换后的文件大小是多少？**

A: RGB565 格式每个像素占用 2 字节，ARGB8888 格式每个像素占用 4 字节。例如，一张 320x240 的图片：
- RGB565 格式：320 × 240 × 2 = 153,600 字节
- ARGB8888 格式：320 × 240 × 4 = 307,200 字节

**Q: 支持哪些图片格式？**

A: 
- `jpeg2rgb565` 目录：仅支持 JPEG 格式（.jpg）
- `png2argb` 目录：仅支持 PNG 格式（.png）

**Q: 可以自定义输出文件名吗？**

A: 当前版本的脚本会自动生成固定格式的输出文件名。如需自定义，可以修改 `run.bat` 脚本。

## 参考资源

- [FFmpeg 官方网站](https://ffmpeg.org/)
- [HxD 十六进制编辑器](https://mh-nexus.de/en/hxd/)
