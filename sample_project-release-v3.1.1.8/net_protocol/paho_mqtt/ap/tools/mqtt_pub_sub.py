#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import paho.mqtt
import paho.mqtt.client as mqtt
import time
import threading

print("=== MQTT发布订阅客户端脚本 ===")
print("Python版本:", sys.version)
print("paho-mqtt版本:", paho.mqtt.__version__)

# MQTT服务器配置
MQTT_BROKER = "broker.emqx.io"  # 可以替换为本地MQTT服务器地址
MQTT_PORT = 1883

# 主题配置
SUBSCRIBE_TOPIC = "/mqtt/pub_test"  # 订阅的主题
PUBLISH_TOPIC = "/mqtt/sub_test"    # 发布的主题

# 客户端ID（确保唯一）
CLIENT_ID = "python_mqtt_pub_sub"

# 发布消息配置
PUBLISH_INTERVAL = 5  # 发布消息间隔（秒）
PUBLISH_COUNT = 100    # 发布消息数量，0表示无限发布


class MqttPubSubClient:
    """MQTT发布订阅客户端类"""
    
    def __init__(self):
        """初始化MQTT客户端"""
        self.client = None
        self.is_connected = False
        self.publish_thread = None
        self.stop_event = threading.Event()
        self.message_count = 0
    
    def on_connect(self, client, userdata, flags, rc, properties=None):
        """连接成功回调函数"""
        print(f"=== 连接回调触发 ===")
        print(f"参数: client={client}, userdata={userdata}, flags={flags}, rc={rc}, properties={properties}")
        
        if rc == 0:
            self.is_connected = True
            print(f"✅ 成功连接到MQTT服务器: {MQTT_BROKER}")
            # 连接成功后订阅主题
            client.subscribe(SUBSCRIBE_TOPIC, qos=0)
            print(f"📩 已订阅主题: {SUBSCRIBE_TOPIC}")
            print(f"🔍 等待接收消息...")
        else:
            print(f"❌ 连接失败，错误码: {rc}")
            print(f"错误码说明: {mqtt.connack_string(rc)}")
    
    def on_message(self, client, userdata, msg):
        """消息接收回调函数"""
        try:
            payload = msg.payload.decode('utf-8')
            print(f"📨 收到消息 - 主题: {msg.topic}, 负载: {payload}")
        except Exception as e:
            print(f"❌ 解析消息失败: {e}")
    
    def on_disconnect(self, client, userdata, rc, properties=None):
        """连接断开回调函数"""
        print(f"=== 断开连接回调触发 ===")
        print(f"参数: client={client}, userdata={userdata}, rc={rc}, properties={properties}")
        
        self.is_connected = False
        if rc != 0:
            print(f"⚠️  意外断开连接，错误码: {rc}")
            print(f"错误码说明: {mqtt.connack_string(rc)}")
        print("🔌 已断开与MQTT服务器的连接")
    
    def on_publish(self, client, userdata, mid, properties=None):
        """消息发布成功回调函数"""
        print(f"📤 消息发布成功，消息ID: {mid}")
    
    def publish_message(self):
        """发布消息线程函数"""
        print(f"=== 发布消息线程启动 ===")
        count = 0
        while not self.stop_event.is_set():
            # 检查是否达到发布数量限制
            if PUBLISH_COUNT > 0 and count >= PUBLISH_COUNT:
                print(f"✅ 已完成所有消息发布（共 {count} 条）")
                break
            
            # 生成消息内容
            message = f"Hello MQTT! This is message #{count+1} from {CLIENT_ID}"
            
            # 发布消息
            if self.is_connected:
                result = self.client.publish(PUBLISH_TOPIC, message, qos=0)
                if result.rc == mqtt.MQTT_ERR_SUCCESS:
                    print(f"📤 正在发布消息 #{count+1}: {message}")
                    count += 1
                else:
                    print(f"❌ 发布消息失败，错误码: {result.rc}")
            else:
                print("⚠️  未连接到MQTT服务器，跳过发布")
            
            # 等待指定间隔
            self.stop_event.wait(PUBLISH_INTERVAL)
    
    def start(self):
        """启动MQTT客户端"""
        print("🚀 MQTT发布订阅客户端启动")
        print(f"📋 配置信息:")
        print(f"   服务器: {MQTT_BROKER}:{MQTT_PORT}")
        print(f"   订阅主题: {SUBSCRIBE_TOPIC}")
        print(f"   发布主题: {PUBLISH_TOPIC}")
        print(f"   客户端ID: {CLIENT_ID}")
        print(f"   发布间隔: {PUBLISH_INTERVAL}秒")
        print(f"   发布数量: {'无限' if PUBLISH_COUNT == 0 else PUBLISH_COUNT}")
        
        # 创建MQTT客户端实例
        print("=== 创建MQTT客户端实例 ===")
        self.client = mqtt.Client(
            client_id=CLIENT_ID, 
            clean_session=True
        )
        
        # 设置回调函数
        print("=== 设置回调函数 ===")
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        self.client.on_disconnect = self.on_disconnect
        self.client.on_publish = self.on_publish
        
        try:
            # 连接到MQTT服务器
            print(f"\n🔄 正在连接到MQTT服务器: {MQTT_BROKER}:{MQTT_PORT}")
            self.client.connect(MQTT_BROKER, MQTT_PORT, keepalive=60)
            print("=== 连接命令已发送 ===")
            
            # 启动发布消息线程
            print("=== 启动发布消息线程 ===")
            self.publish_thread = threading.Thread(target=self.publish_message, daemon=True)
            self.publish_thread.start()
            
            # 启动MQTT消息循环
            print("🔄 启动MQTT消息循环...")
            self.client.loop_forever()
            
        except KeyboardInterrupt:
            print("\n⏹️  用户中断，正在停止服务...")
            self.stop()
        except Exception as e:
            print(f"\n❌ 发生错误: {e}")
            import traceback
            traceback.print_exc()
            self.stop()
    
    def stop(self):
        """停止MQTT客户端"""
        print("=== 停止MQTT客户端 ===")
        # 设置停止事件
        self.stop_event.set()
        
        # 等待发布线程结束
        if self.publish_thread and self.publish_thread.is_alive():
            self.publish_thread.join(timeout=2.0)
        
        # 断开MQTT连接
        if self.client:
            self.client.disconnect()
            self.client.loop_stop()
        
        print("✅ MQTT客户端已停止")


def main():
    """主函数"""
    print("=== 进入main函数 ===")
    client = MqttPubSubClient()
    print("=== 创建客户端实例完成 ===")
    client.start()


if __name__ == "__main__":
    print("=== 脚本开始执行 ===")
    main()