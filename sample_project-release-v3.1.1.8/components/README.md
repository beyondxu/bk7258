# sample_project通用组件

[TOC]  

## 概述

以下文件夹是作为独立的组件，被sample_project不同的示例工程调用。


## 使用说明

- 工程根目录下的 CMakeLists.txt，加入需要调用的组件文件夹路径，让其参与编译，如jpeg硬解组件app_jpeg_hw_decode

``` cmake
set(EXTRA_COMPONENTS_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../components/app_jpeg_hw_decode
)
```

- 工程**ap/CMakeLists.txt** PRIV_REQUIRES中需加入依赖，这样可直接调用组件的头文件：

``` cmake
set(priv_req  media_service multimedia bk_network_provisioning bk_jpeg_decoder app_jpeg_hw_decode)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
```


