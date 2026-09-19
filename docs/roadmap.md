# Roadmap

## V0.1 软件骨架（已完成）

- 模块边界、状态机、事件、动画、HAL、配置
- 无硬件测试
- 依赖最少的 PC framebuffer demo
- 硬件未知项和 Bring-up 文档

退出条件：工程可编译，基础测试通过，模拟器复用正式 Pet Core。

## V0.2 PC 模拟桌宠（已完成）

- SDL2 交互窗口、缩放、稳定主循环和键盘事件
- P3 PPM file asset provider 和基础像素动画资源
- Message/Happy/Look 完整事件链路
- App、Renderer、资源和输入适配集成测试

退出条件：SDL backend 仅实现 Display HAL，Core/Renderer 无 SDL 依赖，无 SDL 时仍可构建
共享模块和 framebuffer simulator。当前条件已满足。

## V0.3 ESP32-S3 真机静态集成（已完成）

- ESP32-S3、ESP-IDF 6.1、引脚和内存基线确认
- 经验证的 SPI2/ST7789 backend
- 共享 Renderer 固定 IDLE 画面真机显示
- PSRAM 单 framebuffer 和内部非 DMA staging
- GPIO 背光开关

退出条件：host 测试继续通过，ESP-IDF build/flash 成功，启动日志无错误，真屏静态桌宠的
内容、颜色和方向正确。当前条件已满足。

## V0.4 真机动画（已完成）

- 编译进固件的静态 RGB565 asset provider
- 共享 App/Animation/Event/Renderer 真机持续运行
- Core Idle 行为和 Event Queue 自动演示
- FPS、render、flush、frame、internal heap、PSRAM 低频统计
- 305 秒真机稳定性验证；同步全帧基线约 5.2 FPS

退出条件：host 7/7 测试通过，ESP-IDF build/flash 成功，Blink/Look/Happy/Sleep/Wake 循环，
连续五分钟无崩溃且 heap 不持续下降。当前条件已满足。DMA、局部刷新和撕裂优化留待后续。

## V0.5 LCD 性能优化（已完成）

- baseline 与单变量实测：polling staging、SPI 时钟、SPI DMA、行分块
- 集中可切换的 LCD 编译参数，不修改共享模块
- 最终配置：40 MHz + `SPI_DMA_CH_AUTO` + 4096-byte 内部 staging + 单一 PSRAM framebuffer
- flush 从 149.6 ms 降至 31.5 ms，整帧约 68.6 ms，10.0 FPS（pacing 受限）
- 每个候选配置至少 60 秒真机运行，heap 全程稳定
- dirty rectangle 分析：需先增加极小的平台无关 dirty-bounds 接口，本阶段不实现

退出条件：host 7/7 测试通过，PC simulator 正常，最终配置 60 秒真机回归无 SPI 错误、
无 watchdog、无重启、heap 不下降。当前条件已满足（基于日志）；花屏/错色/撕裂仍待人工
目视确认。

## V0.6 Rotary Input / Backlight（已完成）

- 平台无关 `NAV_NEXT` / `NAV_PREV` 事件
- PC `LEFT` / `RIGHT` 与 ESP32 rotary 共用 Core 行为
- shared App inactivity sleep，测试阈值 15 秒
- ESP32 GA/BB rotary backend skeleton 和 quadrature decoder
- GPIO7 LEDC PWM Backlight HAL backend
- rotary hardware diagnostic：ISR 微秒级采样，实测 GA/BB 电气参数与方向表
- rotary 正式接入：ISR → decoder → NAV 事件，CW/CCW 20 格与快速旋转真机验收
- press 接入：GA ADC LOW/MID/HIGH classifier + press 状态机 → 复用 `PET_EVENT_BUTTON`
  （INTERACT）；press 与 quadrature accumulator 隔离，按压期间导航暂停
- 真机验收：单击 50 次、长按 2s、residual accumulator 专项、press+rotate ×10、
  ADC margin、10 分钟混合压力稳定性

退出条件：host tests 通过；ESP-IDF build 通过；CW/CCW/PRESS 计数与事件一一对应；
10 分钟无 crash/watchdog/drop、heap 恒定；press ADC 阈值余量充分。当前条件已满足。

## V0.7 Wi-Fi / 时间

- Wi-Fi service
- 网络状态事件
- 时间同步和基础时间显示

## V0.8 PC 通信

- 传输协议和版本协商
- 调试、资源或消息通道

## V0.9 AI

- AI service 接口和异步消息流
- 对话状态、错误处理和隐私策略

## V1.0 第一版完整桌宠

- 稳定硬件显示和交互
- 核心在线服务
- 可恢复错误、功耗和发布流程
