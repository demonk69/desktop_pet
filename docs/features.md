# 功能列表

状态以本文件为准；较大功能合入时必须同步更新。

## 已完成

- Pet Core 基础状态机：启动、Idle、眨眼、左右看、开心、睡眠
- timer/button/message/sleep/wake/animation-done 事件处理
- 固定容量事件队列
- 多帧动画、逐帧时长、循环/非循环、切换、完成通知
- 可替换 animation catalog
- Display HAL 和 Backlight HAL 公共 API
- SPI、8080、ST7789、PWM backend 扩展入口和显式 unsupported stub
- 平台无关 Renderer 与简单占位表情
- PC framebuffer 模拟器、状态日志和 PPM 输出
- 状态机、动画、事件队列和配置测试
- 集中配置及基础校验

## 开发中

- V0.1 文档与接口稳定化
- 开发板到货前的行为和边界测试补充

## 计划中

- SDL2 或等价的交互式 PC 模拟窗口
- 文件型/生成型动画资源 provider
- ST7789 真机显示
- SPI backend
- 8080 backend（是否实现由实测性能和接线决定）
- PWM 背光与淡入淡出
- PSRAM framebuffer 策略
- Wi-Fi
- 网络与时间同步
- 天气
- PC 通信
- AI 交互
- 按键与传感器
- 持久化 storage

## 暂不实现

- 在线 AI 和云端推理
- 天气 API 接入
- OTA
- 大型 GUI 框架和复杂窗口系统
- 复杂运行时配置系统
- 未确认 SDK 上的硬件驱动
