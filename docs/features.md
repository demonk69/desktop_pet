# 功能列表

状态以本文件为准；较大功能合入时必须同步更新。

## 已完成

- Pet Core 基础状态机：启动、Idle、眨眼、左右看、开心、睡眠
- timer/button/message/sleep/wake/animation-done 事件处理
- 固定容量事件队列
- 多帧动画、逐帧时长、循环/非循环、切换、完成通知
- 可替换 animation catalog
- 通用 bitmap asset provider 和 PC P3 PPM file provider
- Display HAL 和 Backlight HAL 公共 API
- SPI、8080、ST7789、PWM backend 扩展入口和显式 unsupported stub
- 平台无关 Renderer 与简单占位表情
- PC framebuffer 模拟器、状态日志和 PPM 输出
- SDL2 实时窗口、nearest-neighbor 缩放和稳定帧循环
- SPACE/H/S/W/M/LEFT/RIGHT/ESC 键盘输入适配
- Message、Happy 和 Look 事件链路
- boot、idle、blink、look、happy、sleep 文件资源
- 状态机、动画、事件队列、配置、App/Renderer 集成和输入适配测试
- 集中配置及基础校验
- ESP-IDF 6.1 独立构建目标和共享源码 component
- ESP32-S3 SPI2/ST7789 240x240 Display HAL backend
- PSRAM 全 framebuffer 和同步 RGB565 MSB-first flush
- GPIO7 基础背光开关 backend
- 真机固定 IDLE Renderer 画面，构建、烧录、启动和显示验证
- ESP32 compiled RGB565 asset provider，无文件系统和每帧动态分配
- ESP32 单调时钟 runtime、目标 frame pacing 和过载 yield
- Core Blink/Look 与 Event Queue Happy/Sleep/Wake 自动演示
- 五秒 FPS/render/flush/frame/heap 统计和 305 秒真机稳定性验证
- LCD 显示链路集中编译参数（SPI 频率、staging、DMA、分块高度）
- 单变量性能实测：polling staging、10/20/40/80 MHz、SPI DMA、1/4/8/16/32 行分块
- 最终显示配置：40 MHz + SPI DMA + 4096-byte 内部 staging，10.0 FPS，flush 约 31.5 ms

## 开发中

- 暂无；V0.5 已完成 LCD 性能测量与最终配置真机回归

## 计划中

- 8080 backend（是否实现由实测性能和接线决定）
- Display HAL 边界和错误路径测试补充
- PWM 背光与淡入淡出
- 动画画面目视确认（花屏、错色、坏帧、撕裂）
- 更高 SPI 频率或更大 staging 的目视验证与选择
- dirty rectangle 最小接口设计与实现
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
- 复杂 LCD GUI 和未验证硬件 backend
