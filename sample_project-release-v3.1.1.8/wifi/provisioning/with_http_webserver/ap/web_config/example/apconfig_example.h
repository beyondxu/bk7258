#ifndef  _AP_CONFIG_EXAMPLE_H
#define  _AP_CONFIG_EXAMPLE_H


#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/netif.h"
#include "lwip/dns.h"
#include "lwip/prot/dns.h"
#include "lwip/udp.h"

#define TAG "web_config"

#define BK_EXAMPLE_LOG            BK_LOGI

#define TEST_PASS() 	BK_EXAMPLE_LOG(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "PASS")
#define TEST_FAIL() 	BK_EXAMPLE_LOG(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "FAIL")

#define MAX_DOMAIN_LEN      253
#define DNS_TTL             60
#define DNS_HEADER_LEN      12
#define DNS_FLAG1_QR        0x80 /* Query/Response */
#define DNS_FLAG1_RD        0x01 /* Recursion Desired */
#define DNS_FLAG1_AA        0x04 /* Authoritative Answer */
#define DNS_PORT            53

#define HEADER_GET      "<!doctype html><html lang='Zh'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1, shrink-to-fit=no'><title>WIFI配置</title><style type='text/css'>*,*::before,*::after {box-sizing: border-box;}html {line-height: 1.15;-webkit-text-size-adjust: 100%;}body {margin: 0;font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI',Arial;line-height: 1.5;background-color: #f5f5f5;padding-bottom: 200px;align-items: center;-ms-flex-align: center;display: flex;}p {margin-top: 0;margin-bottom: 1rem;}button{cursor: pointer;}.h3 {margin-top: 0;margin-bottom: 0.5rem;font-weight: 500;line-height: 1.2;font-size: 1.75rem;}.form-control {width: 100%;height: calc(1.5em + 0.75rem + 2px);padding: 0.375rem 0.75rem;font-size: 1rem;font-weight: 400;line-height: 1.5;color: #495057;background-color: #fff;background-clip: padding-box;border: 1px solid #ced4da;border-radius: 0.25rem;transition: border-color 0.15s ease-in-out, box-shadow 0.15s ease-in-out;}.form-control::-ms-expand {background-color: transparent;border: 0;}.form-control:focus {color: #495057;background-color: #fff;border-color: #80bdff;outline: 0;box-shadow: 0 0 0 0.2rem rgba(0, 123, 255, 0.25);}.form-control::-webkit-input-placeholder {color: #6c757d;opacity: 1;}.form-control::-moz-placeholder {color: #6c757d;opacity: 1;}.form-control:-ms-input-placeholder {color: #6c757d;opacity: 1;}.form-control::-ms-input-placeholder {color: #6c757d;opacity: 1;}.form-control:disabled {background-color: #e9ecef;opacity: 1;}.btn {color: #fff;background-color: #007bff;border-color: #007bff;padding: 0.5rem 1rem;font-size: 1.25rem;line-height: 1.5;border-radius: 0.3rem;display: block;width: 100%;}.mb-3 {margin-bottom: 1rem !important;}.mb-4 {margin-bottom: 1.5rem !important;}.text-center {text-align: center !important;}.font-weight-normal {font-weight: 400 !important;}html,body {height: 100%;}.form-signin {width: 100%;max-width: 420px;padding: 15px;margin: auto;}.form-label-group {position: relative;margin-bottom: 1rem;}.form-label-group > input,.form-label-group > label {height: 3.125rem;padding: .75rem;}.form-label-group > label {position: absolute;top: 0;left: 0;display: block;width: 100%;margin-bottom: 0;line-height: 1.5;color: #495057;pointer-events: none;cursor: text;border: 1px solid transparent;border-radius: .25rem;transition: all .1s ease-in-out;}.form-label-group input::-webkit-input-placeholder {color: transparent;}.form-label-group input:-ms-input-placeholder {color: transparent;}.form-label-group input::-ms-input-placeholder {color: transparent;}.form-label-group input::-moz-placeholder {color: transparent;}.form-label-group input::placeholder {color: transparent;}.form-label-group input:not(:placeholder-shown) {padding-top: 1.25rem;padding-bottom: .25rem;}.form-label-group input:not(:placeholder-shown) ~ label {padding-top: .25rem;padding-bottom: .25rem;font-size: 12px;color: #777;}@supports (-ms-ime-align: auto) {.form-label-group > label {display: none;}.form-label-group input::-ms-input-placeholder {color: #777;}}@media all and (-ms-high-contrast: none), (-ms-high-contrast: active) {.form-label-group > label {display: none;}.form-label-group input:-ms-input-placeholder {color: #777;}}.connection-status{display:flex;align-items:center;justify-content:center;margin:20px 0;padding:15px;background:#f8f9fa;border-radius:8px;border-left:4px solid #007bff;}.connection-status.success{border-left-color:#28a745;background:#d4edda;}.connection-status.error{border-left-color:#dc3545;background:#f8d7da;}.connection-status.warning{border-left-color:#ffc107;background:#fff3cd;}.status-icon{font-size:24px;margin-right:10px;}.status-text{font-size:16px;font-weight:500;}.progress-container{width:100%;height:8px;background:#e9ecef;border-radius:4px;overflow:hidden;margin:15px 0;}.progress-bar{height:100%;background:linear-gradient(90deg,#007bff,#0056b3);border-radius:4px;transition:width 0.3s ease;width:0%;}.progress-bar.animated{animation:progress-animation 2s ease-in-out infinite;}@keyframes progress-animation{0%{width:0%;}50%{width:70%;}100%{width:100%;}}.loading-spinner{display:inline-block;width:20px;height:20px;border:3px solid #f3f3f3;border-top:3px solid #007bff;border-radius:50%;animation:spin 1s linear infinite;margin-right:10px;}@keyframes spin{0%{transform:rotate(0deg);}100%{transform:rotate(360deg);}}.status-steps{display:flex;justify-content:space-between;margin:20px 0;}.step{flex:1;text-align:center;padding:10px;position:relative;}.step.active .step-number{background:#007bff;color:white;}.step.completed .step-number{background:#28a745;color:white;}.step-number{width:30px;height:30px;border-radius:50%;background:#e9ecef;display:flex;align-items:center;justify-content:center;margin:0 auto 5px;font-weight:bold;}.step-label{font-size:12px;color:#6c757d;}.step.active .step-label{color:#007bff;font-weight:bold;}.step.completed .step-label{color:#28a745;}</style></head>"
#define BODY_POST       "<body><form method='post' action='/' class='form-signin'><div class='text-center mb-4'><h1 class='h3 mb-3 font-weight-normal'>WIFI配置</h1></div><div class='connection-status' id='connectionStatus'><div class='loading-spinner' id='loadingSpinner'></div><span class='status-text' id='statusText'>正在提交Wi-Fi配置...</span></div><div class='status-steps' id='statusSteps'><div class='step active' id='step1'><div class='step-number'>1</div><div class='step-label'>提交配置</div></div><div class='step' id='step2'><div class='step-number'>2</div><div class='step-label'>连接网络</div></div><div class='step' id='step3'><div class='step-number'>3</div><div class='step-label'>连接成功</div></div></div><div class='progress-container'><div class='progress-bar' id='progressBar'></div></div><p align='center' id='statusMessage'>正在处理Wi-Fi配置，请稍候...</p><div class='form-label-group'></div><div class='form-label-group'></div></form><script>var statusText=document.getElementById('statusText');var statusMessage=document.getElementById('statusMessage');var step1=document.getElementById('step1');var step2=document.getElementById('step2');var step3=document.getElementById('step3');var connectionStatus=document.getElementById('connectionStatus');var progressBar=document.getElementById('progressBar');var loadingSpinner=document.getElementById('loadingSpinner');var checkCount=0;var maxChecks=500;var isConnected=false;function checkConnectionStatus(){if(isConnected){return;}try{fetch('/status').then(function(response){if(!response.ok){throw new Error('状态检查失败');}return response.json();}).then(function(status){checkCount++;progressBar.style.width=Math.min(checkCount/maxChecks*100,100)+'%';if(status.connect_state===2){statusText.textContent='Wi-Fi连接成功！';statusMessage.textContent='设备已成功连接到网络'+(status.ssid?':'+status.ssid:'');connectionStatus.classList.add('success');loadingSpinner.style.display='none';step1.classList.add('completed');step2.classList.add('completed');step3.classList.add('active');progressBar.style.width='100%';isConnected=true;clearInterval(statusInterval);}else if(status.connect_state===1){statusText.textContent='正在连接Wi-Fi网络...';statusMessage.textContent='正在连接Wi-Fi网络，请稍候...';step1.classList.add('completed');step2.classList.add('active');}else if(checkCount<=1){statusText.textContent='正在提交Wi-Fi配置...';statusMessage.textContent='正在处理Wi-Fi配置，请稍候...';}else{statusText.textContent='正在验证连接状态...';statusMessage.textContent='正在验证Wi-Fi连接状态，请稍候...';step2.classList.add('completed');step3.classList.add('active');}if(checkCount>=maxChecks){statusText.textContent='连接超时';statusMessage.textContent='Wi-Fi连接超时，请检查网络设置并重试。';connectionStatus.classList.add('error');loadingSpinner.style.display='none';clearInterval(statusInterval);}}).catch(function(error){if(!isConnected){statusText.textContent='状态检查错误';statusMessage.textContent='无法获取设备状态：'+error.message;connectionStatus.classList.add('error');loadingSpinner.style.display='none';clearInterval(statusInterval);}});}catch(error){if(!isConnected){statusText.textContent='状态检查错误';statusMessage.textContent='无法获取设备状态：'+error.message;connectionStatus.classList.add('error');loadingSpinner.style.display='none';clearInterval(statusInterval);}}}var statusInterval=setInterval(checkConnectionStatus,100);</script></body></html>\r\n"


#define PAGE_HEADER  "<html>\
<head>\
<meta http-equiv='Content-Type' content='text/html; charset=UTF-8'>\
<meta name='viewport' content='width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no'>\
<title>设备配网</title>\
<style>\
    /* 全局样式重置 */\
    * { margin: 0; padding: 0; box-sizing: border-box; }\n    \
    /* 基础样式 */\
    body { \
        font-family: 'SF Pro Display', -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; \
        background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); \
        min-height: 100vh; \
        display: flex; \
        align-items: center; \
        justify-content: center; \
        padding: 20px; \
        overflow-x: hidden; \
    }\n    \
    /* 主容器样式 */\
    .container { \
        background: white; \
        border-radius: 24px; \
        box-shadow: 0 15px 40px rgba(0, 0, 0, 0.15); \
        width: 100%; \
        max-width: 480px; \
        overflow: hidden; \
        transition: transform 0.3s ease; \
    }\n    \
    .container:hover { transform: translateY(-5px); }\n    \
    /* 头部样式 */\
    .header { \
        background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); \
        color: white; \
        padding: 30px; \
        text-align: center; \
    }\n    \
    .header h1 { \
        font-size: 28px; \
        font-weight: 700; \
        margin-bottom: 10px; \
    }\n    \
    .header p { \
        font-size: 16px; \
        opacity: 0.9; \
    }\n    \
    /* 内容区域 */\
    .content { padding: 30px; }\n    \
    /* 标题样式 */\
    .step-title { \
        font-size: 22px; \
        font-weight: 600; \
        color: #333; \
        margin-bottom: 20px; \
        text-align: center; \
    }\n    \
    /* WiFi列表样式 */\
    .wifi-list { \
        max-height: 300px; \
        overflow-y: auto; \
        border-radius: 12px; \
        border: 1px solid #eee; \
    }\n    \
    /* WiFi项样式 */\
    .wifi-item { \
        padding: 16px 20px; \
        border-bottom: 1px solid #f0f0f0; \
        cursor: pointer; \
        transition: all 0.2s ease; \
        position: relative; \
    }\n    \
    .wifi-item:last-child { border-bottom: none; }\n    .wifi-item:hover { background-color: #f8f9fa; }\n    .wifi-item:active { background-color: #e9ecef; }\n    \
    /* WiFi名称样式 */\
    .wifi-name { \
        font-size: 18px; \
        color: #333; \
        font-weight: 500; \
        display: block; \
        text-decoration: none; \
        width: 100%; \
    }\n    \
    /* 分隔线样式 */\
    .divider { \
        height: 1px; \
        background: linear-gradient(to right, transparent, #e0e0e0, transparent); \
        margin: 8px 0; \
    }\n    \
    /* 模态框样式 */\
    .modal { \
        display: none; \
        position: fixed; \
        top: 0; \
        left: 0; \
        width: 100%; \
        height: 100%; \
        background-color: rgba(0, 0, 0, 0.5); \
        z-index: 1000; \
        backdrop-filter: blur(5px); \
        animation: fadeIn 0.3s ease; \
    }\n    \
    .modal-content { \
        background: white; \
        margin: 10% auto; \
        border-radius: 20px; \
        width: 90%; \
        max-width: 450px; \
        overflow: hidden; \
        box-shadow: 0 20px 40px rgba(0, 0, 0, 0.2); \
        animation: slideIn 0.4s ease; \
    }\n    \
    @keyframes fadeIn { \
        from { opacity: 0; } \
        to { opacity: 1; } \
    }\n    \
    @keyframes slideIn { \
        from { transform: translateY(-50px); opacity: 0; } \
        to { transform: translateY(0); opacity: 1; } \
    }\n    \
    /* 关闭按钮样式 */\
    .close { \
        position: absolute; \
        right: 20px; \
        top: 20px; \
        font-size: 24px; \
        color: #666; \
        cursor: pointer; \
        width: 32px; \
        height: 32px; \
        display: flex; \
        align-items: center; \
        justify-content: center; \
        border-radius: 50%; \
        transition: all 0.2s ease; \
    }\n    \
    .close:hover { \
        background-color: #f0f0f0; \
        color: #333; \
    }\n    \n    /* 表单样式 */\n    .form-container { \
        padding: 25px; \
        position: relative; \
    }\n    \n    .form-group { \
        margin-bottom: 20px; \
    }\n    \n    .form-group label { \
        display: block; \
        margin-bottom: 8px; \
        font-weight: 600; \
        color: #555; \
    }\n    \n    .form-control { \
        width: 100%; \
        padding: 12px 16px; \
        border: 2px solid #e0e0e0; \
        border-radius: 12px; \
        font-size: 16px; \
        transition: all 0.3s ease; \
        background-color: #fafafa; \
    }\n    \n    .form-control:focus { \
        outline: none; \
        border-color: #667eea; \
        box-shadow: 0 0 0 4px rgba(102, 126, 234, 0.1); \
        background-color: white; \
    }\n    \n    .form-control:disabled { \
        background-color: #f5f5f5; \
        color: #999; \
    }\n    \n    /* 按钮样式 */\n    .btn { \
        background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); \
        color: white; \
        border: none; \
        padding: 14px 24px; \
        border-radius: 12px; \
        font-size: 18px; \
        font-weight: 600; \
        cursor: pointer; \
        width: 100%; \
        transition: all 0.3s ease; \
        box-shadow: 0 4px 15px rgba(102, 126, 234, 0.3); \
    }\n    \n    .btn:hover { \
        transform: translateY(-2px); \
        box-shadow: 0 6px 20px rgba(102, 126, 234, 0.4); \
    }\n    \n    .btn:active { \
        transform: translateY(0); \
    }\n    \n    /* 提示信息样式 */\n    .hint { \
        background-color: #e3f2fd; \
        color: #1976d2; \
        padding: 12px 16px; \
        border-radius: 8px; \
        margin-bottom: 20px; \
        font-size: 14px; \
        border-left: 4px solid #2196f3; \
    }\n    \n    /* 加载状态样式 */\n    .loading { \
        text-align: center; \
        padding: 40px 20px; \
    }\n    \n    .loading-spinner { \
        width: 40px; \
        height: 40px; \
        border: 3px solid #f3f3f3; \
        border-top: 3px solid #667eea; \
        border-radius: 50%; \
        animation: spin 1s linear infinite; \
        margin: 0 auto 20px; \
    }\n    \n    @keyframes spin { \
        0% { transform: rotate(0deg); } \
        100% { transform: rotate(360deg); } \
    }\n    \n    /* 错误信息样式 */\n    .error { \
        background-color: #ffebee; \
        color: #c62828; \
        padding: 16px; \
        border-radius: 12px; \
        text-align: center; \
        font-weight: 500; \
        border: 1px solid #ffcdd2; \
    }\n    \n    /* 响应式设计 */\n    @media (max-width: 480px) { \
        body { padding: 15px; } \
        .container { \
            margin: 0; \
            border-radius: 20px; \
        } \
        .header { padding: 25px 20px; } \
        .content { padding: 25px 20px; } \
        .form-container { padding: 20px; } \
    }\n</style>\
</head>"

#define PAGE_BODY_GET "<body> <div class='container'>\n    <div class='header'>\n        <h1>设备配网</h1>\n        <p>选择WiFi网络并输入密码</p>\n    </div>\n    \n    <div class='content'>\n        <h2 id='step_tile' class='step-title'>正在获取WiFi列表...</h2>\n        \n        <div id='loading' class='loading'>\n            <div class='loading-spinner'></div>\n            <p>正在搜索附近的WiFi网络</p>\n        </div>\n        \n        <div id='resultContainer' class='wifi-list'></div>\n    </div>\n</div>\n\n<!-- 模态对话框 -->\n<div id='myModal' class='modal'>\n    <div class='modal-content'>\n        <span class='close' id='close'>×</span>\n        <div class='form-container'>\n            <h3 style='text-align: center; margin-bottom: 20px; color: #333;'>WiFi配置</h3>\n            <div class='hint'>请输入WiFi密码，如果密码为空可以不填</div>\n            \n            <form method='post' action='/'>\n                <div class='form-group'>\n                    <label for='ssid'>WiFi名称</label>\n                    <input type='text' name='ssid' id='ssid' class='form-control' required readonly>\n                </div>\n                \n                <div class='form-group'>\n                    <label for='password'>WiFi密码</label>\n                    <input type='text' id='password' name='password' class='form-control' placeholder='请输入WiFi密码'>\n                </div>\n                \n                <button class='btn' type='submit'>连接</button>\n            </form>\n        </div>\n    </div>\n</div>\n\n<script>\n    const resultContainer = document.getElementById('resultContainer');\n    const API_URL = '/scan';\n    const loadingElement = document.getElementById('loading');\n    \n    function change_ssid(item) {\n        document.getElementById('myModal').style.display = 'block';\n        document.getElementById('ssid').value = item.textContent.trim();\n        document.getElementById('password').focus();\n    }\n    \n  // 加载WiFi列表\n    async function loadUserData() {\n        try {\n            // 显示加载状态\n            loadingElement.style.display = 'block';\n            document.getElementById('step_tile').textContent = '正在获取WiFi列表...';\n            \n            const response = await fetch(API_URL);\n            \n            if (!response.ok) {\n                throw new Error('获取WiFi列表失败');\n            }\n            \n            const networks = await response.json();\n            \n            // 隐藏加载状态\n            loadingElement.style.display = 'none';\n            document.getElementById('step_tile').textContent = '请选择您的WiFi网络';\n            \n            // 清空结果容器\n            resultContainer.innerHTML = '';\n            \n            if (networks && networks.length > 0) {\n                // 按信号强度排序\n                networks.sort((a, b) => (b.rssi || 0) - (a.rssi || 0));\n                \n                // 创建WiFi列表项\n                const wifiItems = networks.map(network => {\n                    const name = network.name || network.ssid || '未知网络';\n                    const rssi = network.rssi || -80;\n                    return `\n                    <div class=\"wifi-item\">\n                        <a href=\"javascript:void(0)\" class=\"wifi-name\" onclick=\"change_ssid(this)\">\n                            ${name}\n                        </a>\n                        <div class=\"divider\"></div>\n                    </div>`;\n                }).join('');\n                \n                resultContainer.innerHTML = wifiItems;\n            } else {\n                resultContainer.innerHTML = `\n                    <div class=\"error\">\n                        未找到任何WiFi网络<br>\n                        请检查设备WiFi是否开启\n                    </div>`;\n            }\n            \n        } catch (error) {\n            // 隐藏加载状态\n            loadingElement.style.display = 'none';\n            \n            // 显示错误信息\n            resultContainer.innerHTML = `\n                <div class=\"error\">\n                    扫描WiFi失败<br>\n                    请检查设备状态并重试\n                </div>`;\n            \n            // 添加重试按钮\n            const retryBtn = document.createElement('button');\n            retryBtn.className = 'btn';\n            retryBtn.style.marginTop = '20px';\n            retryBtn.textContent = '重新扫描';\n            retryBtn.onclick = loadUserData;\n            resultContainer.appendChild(retryBtn);\n            \n            console.error('获取数据失败:', error);\n        }\n    }\n    \n    // 关闭模态框\n    document.getElementById('close').addEventListener('click', function() {\n        document.getElementById('myModal').style.display = 'none';\n    });\n    \n    // 点击模态框外部关闭\n    window.onclick = function(event) {\n        if (event.target == document.getElementById('myModal')) {\n            document.getElementById('myModal').style.display = 'none';\n        }\n    };\n    \n    // ESC键关闭模态框\n    document.addEventListener('keydown', function(event) {\n        if (event.key === 'Escape' && document.getElementById('myModal').style.display === 'block') {\n            document.getElementById('myModal').style.display = 'none';\n        }\n    });\n    \n    // 页面加载完成后自动执行扫描\n    window.onload = loadUserData;\n</script>\n</body></html>\r\n"

#define MAX_WIFI_SCAN_AP_NUM             32
#define SCAN_BUFF_SIZE                   4096
#define SCAN_OVER_TIME                   10000

enum
{
    ACK_WIFI_CONNECT_START,
    ACK_WIFI_CONNECTED_CFG_FIN,
    ACK_WIFI_CFG_STOP,
};

typedef struct
{
    char ssid[33];
    char pwd[65];
} net_info_t;

typedef struct
{
    uint32_t event;
    uint32_t param;
} note_msg_t;


struct interface
{
    struct netif *netif;
    ip_addr_t ipaddr;
    ip_addr_t nmask;
    ip_addr_t gw;
};

typedef struct __attribute__((packed))
{
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
}
dns_header_t;

typedef struct __attribute__((packed))
{
    uint16_t name_ptr;
    uint16_t type;
    uint16_t class;
    uint32_t ttl;
    uint16_t rdlength;
    uint32_t addr;
}
dns_answer_t;

typedef struct __attribute__((packed))
{
    uint16_t type;
    uint16_t class;
}
dns_query_t;

void wifi_provisioning_web_config_start(void);
void wifi_provisioning_web_config_stop(void);
#endif
