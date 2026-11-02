# 第三方库如何编译以及使用

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[TOC]  

## 概述

> **备注** ：编译应用工程时，应用工程根目录下的ap/components下的文件夹，只要文件夹下有CMakeLists.txt文件，都会以模块方式自动编译，打包成lib库。


本示例主要介绍第三方库如何编译，以及如何使用，大致步骤如下：

- **创建第三方库源码文件夹**：在应用工程根目录下建立ap/components文件夹，然后将第三方库源码文件夹（如文件夹名为moduleA)放到ap/components文件夹下，并建立一个CMakeLists.txt文件，用于编译第三方库源码；
- **第三方库库生成**：编译工程，会在build文件夹下生成第三方库的lib文件（如libmoduleA.a)；
- **创建第三方库库文件夹**：拷贝ap/components为ap/components--lib文件夹，删除里面的所有.c文件，将第二步的lib文件拷贝进来，修改CMakeLists.txt采用lib库的方式编译；
- **第三方库库文件夹修改名称参与编译**：将ap/components重命名为ap/components--src，ap/components--lib重命名为ap/components（**sdk 只识别ap/components**，所以这里需改名为components，这样才会被sdk识别并编译）；
- **应用代码使用**：应用工程CMakeLists.txt中加入第三方库的依赖，应用代码就可直接调用第三方库的头文件以及函数；

## 示例相关文件说明

用户可自行将下面其中一个文件夹改名为components，用于编译库源码以及lib库的使用验证。

- 第三方库源码文件夹：sample_project/compile/compile_lib/ap/components，目录下面的moduleA是第三方库的示例源码；
- 第三方库lib库文件夹：sample_project/compile/compile_lib/ap/components--lib，目录下面的moduleA是第三方库的如何以lib库访问的示例；

``` c
    set(incs)
    set(srcs)

    set(incs ./inc)

    armino_component_register(
        INCLUDE_DIRS "${incs}"
        )
        
    get_filename_component(lib_name ${CMAKE_CURRENT_SOURCE_DIR}/libmoduleA.a NAME)
    add_prebuilt_library(${lib_name} "${CMAKE_CURRENT_SOURCE_DIR}/libmoduleA.a")
    target_link_libraries(${COMPONENT_LIB} INTERFACE ${lib_name})
```
## 应用代码使用

应用代码加入第三方库moduleA依赖，接着可调用moduleA文件夹下CMakeLists.txt导出的inc路径的头文件。

``` c
    set(incs .)
    set(srcs ap_main.c)

    set(priv_req  media_service multimedia bk_network_provisioning moduleA)    // 依赖第三方库moduleA
```
## 编译

运行以下命令进行编译构建

``` Shell
export SDK_DIR=~/armino/bk_avdk_smp  //设置smp sdk路径到环境变量
cd sample_project/compile/compile_lib
make clean
make bk7258
```

## 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录工程build里的all-app.bin；
- 上电运行后，板子会自动运行，打印调用moduleA里面接口的log信息;

以下是上电运行后的串口log 输出：

```
 ap1:D(291):[moduleA][5]new    //ap_main.c调用moduleA模块的moduleA接口
 ap1:main:I(291):/homeb/honghui.xiao/7258/avdk_smp/sample_project/compile/compile_lib/ap/ap_main.c:29:main:PASS

```

## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。
