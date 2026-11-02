# lvgl 图片解码.c如何保存为bin文件

[TOC]  

## 相关章节

- [lvgl V8.3.9 如何显示解码图片bin](/multimedia/lvgl/V8.X/image_bin_display/README.md)

## 概述

lvgl图片解码显示，一般常用方法是将图片放到文件系统，然后解码显示。这种方式在png图片多，并且图片大时，解码耗费的时间比较长，会导致开机时间久（png一张480X480图片实时解码需要几百ms），这种方式可以考虑将png图片解码为bin文件，省去运行时解码的耗时。

本工具是将lvgl在线图片解码的.c文件，保存为bin文件。

操作步骤大致如下：
1. lvgl官网将图片解码为.c文件；
2. 将.c文件稍微修改，放到本工具下；
3. main.c引用第二步.c里的图片变量；
4. common.h 确认是否修改；
5. 重新编译，运行可执行bin，得到解码的bin文件；

## 步骤1：lvgl官网图片解码.c文件

> **备注** ：png图片选择CF_TRUE_COLOR_ALPHA, jpeg图片选择CF_TRUE_COLOR；

访问 [https://lvgl.io/tools/imageconverter](https://lvgl.io/tools/imageconverter)，选择LVGL v8，然后按照下面图片操作，依次选择图片、颜色格式、输出格式，然后点转换按钮。

![imageconverter](/tools/lvgl_C_file_to_image_bin/lvgl_online_decoce.jpg)

## 步骤2：解码.c放到工具下修改

> **备注** ：可参考原始icon.c文件；

1. 解码.c 改名为icon.c, 去掉解码const 数组前面的定义，增加#include "common.h"。
2. 去掉解码const 数组名前的“LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_ICON”；

## 步骤3：main.c修改

将上面解码.c最下面的变量名修改到main如下地方：

``` c
    extern const lv_img_dsc_t icon;   //图片数组说明

    int main(void)
    {
        FILE *fp = fopen("ram_image.bin", "wb");
        
        fwrite(icon.data, icon.data_size, 1, fp);
``` 

## 步骤4：common.h 确认是否修改

确认以下两个宏是否需修改：

``` c
    #pragma once

    #define LV_COLOR_DEPTH      16                 //lvgl的颜色深度，需要让lvgl输出rgb565填16，输出agb888填32
    #define LV_COLOR_16_SWAP    0                  //一般不修改，spi或qspi屏，需调整为1

``` 

## 步骤5：编译并运行

输出的ram_image.bin文件为解码的bin，可放入文件系统给lvgl使用。
``` c
    make clean
    make
    ./image_to_bin
``` 

## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。
