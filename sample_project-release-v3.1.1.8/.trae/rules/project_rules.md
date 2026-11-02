# 编程规则

## 头文件包含规则

在 sample project 中编写的所有 .c 文件，必须包含以下三个系统头文件：

```c
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
```

### 注意事项

1. 这三个头文件应放在文件开头，在其他自定义头文件之前
2. 保持头文件包含顺序的一致性
3. 这些头文件提供了操作系统、内存管理和字符串操作的基础功能

### 示例

```c
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include "app_display_draw.h"
#include "app_draw_sw.h"
```

## README文档规则

生成组件的README.md时，第一级标题应使用"概述"而不是"组件简介"。

### 示例

```markdown
# 组件名称

## 概述

组件的简要介绍...
```

### 注意事项

1. 保持文档标题的一致性
2. "概述"章节应包含组件的核心功能说明
3. 避免使用"组件简介"等非标准标题

### 目录生成规则

在 README.md 文件中，必须在第一级标题和第一个二级标题之间添加 `[toc]` 标记，用于生成文档目录。

### 示例

```markdown
# 组件名称

[toc]

## 概述

组件的简要介绍...
```

### 注意事项

1. `[toc]` 必须单独占一行
2. `[toc]` 应放在第一级标题之后、第一个二级标题之前
3. 后续生成的所有 README.md 文件都必须包含此标记

## 日志输出规则

所有日志输出必须包含当前函数名和行号，格式如下：

```c
LOGE("[%s][%d] 错误描述\r\n", __FUNCTION__, __LINE__);
LOGI("[%s][%d] 信息描述\r\n", __FUNCTION__, __LINE__);
LOGD("[%s][%d] 调试信息\r\n", __FUNCTION__, __LINE__);
LOGW("[%s][%d] 警告信息\r\n", __FUNCTION__, __LINE__);
```

### 示例

```c
// 正确的日志格式
LOGE("[%s][%d] DMA2D controller already initialized\r\n", __FUNCTION__, __LINE__);
LOGE("[%s][%d] bk_dma2d_new failed:%d\r\n", __FUNCTION__, __LINE__, ret);
LOGD("[%s][%d] init\r\n", __FUNCTION__, __LINE__);

// 错误的日志格式（缺少函数名和行号）
LOGE("DMA2D controller already initialized\n");
LOGE("bk_dma2d_new failed: %d\n", ret);
LOGD("[%s] init \n", __func__);
```

### 注意事项

1. 使用 `__FUNCTION__` 宏获取当前函数名
2. 使用 `__LINE__` 宏获取当前行号
3. 日志字符串末尾使用 `\r\n` 而不是 `\n`
4. 保持日志格式统一，便于调试和问题追踪

## 条件判断规则

所有 if 条件判断中，常量必须放在前面，变量放在后面。

### 示例

```c
// 正确的判断格式
if (NULL == ptr)
if (0 == value)
if (BK_OK == ret)
if (TWO_BYTES == pix_size)

// 错误的判断格式（变量在前）
if (ptr == NULL)
if (value == 0)
if (ret == BK_OK)
if (pix_size == TWO_BYTES)
```

### 注意事项

1. 常量放在前面可以避免误写为 `=` 赋值操作符
2. 保持代码风格统一，提高可读性
3. 适用于所有比较操作符（==、!=、>、<、>=、<=）

## frame_buffer_t合法性检查规则

当函数参数包含frame_buffer_t指针时，必须对该结构体进行合法性检查，包括以下成员：

1. **frame成员检查**：判断frame指针是否为NULL
2. **size成员检查**：判断size是否为0
3. **length成员检查**：判断length是否为0
4. **width成员检查**：判断width是否为0
5. **height成员检查**：判断height是否为0

### 示例

```c
// 正确的合法性检查
if (NULL == frame || NULL == frame->frame)
{
    LOGE("[%s][%d] frame or frame->frame is NULL\r\n", __FUNCTION__, __LINE__);
    return BK_ERR_PARAM;
}

if (0 == frame->size)
{
    LOGE("[%s][%d] frame->size is 0\r\n", __FUNCTION__, __LINE__);
    return BK_ERR_PARAM;
}

if (0 == frame->length)
{
    LOGE("[%s][%d] frame->length is 0\r\n", __FUNCTION__, __LINE__);
    return BK_ERR_PARAM;
}

if (0 == frame->width || 0 == frame->height)
{
    LOGE("[%s][%d] invalid width:%d or height:%d\r\n", __FUNCTION__, __LINE__, frame->width, frame->height);
    return BK_ERR_PARAM;
}
```

### 注意事项

1. 必须先检查frame指针本身是否为NULL，再检查frame->frame是否为NULL
2. 检查顺序建议：frame指针 -> frame成员 -> size -> length -> width/height
3. 所有检查失败时都应返回错误码（如BK_ERR_PARAM）
4. 每个检查都应该有相应的错误日志输出
5. 错误日志必须包含函数名和行号，遵循日志输出规则
