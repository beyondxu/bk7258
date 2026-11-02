# 如何解决编译warning等级的报错

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[TOC]  

## 概述

SDK 默认编译选项，对变量定义未使用、函数没声明就使用、类型不符等waining等级的报错，在编译时以编译报错提示出来，正常开发者是需要逐个解决这些编译报错，特别是类型不符的error，这样可提前通过编译报错解决软件bug。 但是在移植一些开源库或者明确需要忽略这些warning等级的报错时，就可参考此章节进行修改。

本示例列举了变量没声明如何忽略这个编译报错的解决办法。

## 如何解决

### 找到报错原因

如下图所示，编译报错为：-Werror=unused-variable。

![bk_vfs](/compile/ignore_error/docs/compile_error.png)

### 修改代码对应的CMakeLists.txt

在.c代码对应的CMakeLists.txt中添加如下代码，即在上面编译报错"unused-variable"前面加上"-Wno-"，其它warning等级的报错也可以类似处理。

``` CMake
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-variable)
```

## 操作说明
### 编译

运行以下命令进行编译构建

``` Shell
export SDK_DIR=~/armino/bk_avdk_smp  //设置smp sdk路径到环境变量
cd sample_project/compile/ignore_error
make clean
make bk7258
```

### 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录sample_project/compile/ignore_error/bk7258/ignore_error/package/all-app.bin；
- 上电运行后，板子直接运行并打印PASS;

## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。
