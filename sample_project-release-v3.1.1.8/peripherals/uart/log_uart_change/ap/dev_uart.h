#pragma once

/**
 * @brief 打开UART设备
 * @param uart_id UART端口号，指定要打开的UART设备
 * @param config UART配置参数指针，包含波特率、数据位、校验位等配置信息
 * @param rx_isr UART接收中断回调函数指针，当UART有数据可读时会被调用
 * @param tx_isr UART发送中断回调函数指针，当UART可以发送数据时会被调用
 * @return BK_OK 表示成功，其他值表示失败
 * @note 该函数完成UART设备的初始化配置，包括：
 *       1. 检查配置参数有效性
 *       2. 根据配置使能接收/发送DMA
 *       3. 使能接收中断
 *       4. 注册接收和发送中断回调函数
 *       5. 初始化UART设备
 */
s32 dev_uart_open(uart_id_t uart_id, uart_config_t *config, uart_isr_t rx_isr, uart_isr_t tx_isr);

/**
 * @brief 关闭UART设备
 * @param uart_id UART端口号，指定要关闭的UART设备
 * @return BK_OK 表示成功，其他值表示失败
 * @note 该函数完成UART设备的反初始化，释放相关资源
 */
s32 dev_uart_close(uart_id_t uart_id);
