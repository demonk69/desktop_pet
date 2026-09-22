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
- 平台无关 `NAV_NEXT` / `NAV_PREV` 事件和 Core 行为
- PC `RIGHT` / `LEFT` 映射到 `NAV_NEXT` / `NAV_PREV`
- shared App 15 秒 inactivity sleep 计时与 `PET_EVENT_SLEEP` 投递
- ESP32 rotary hardware diagnostic：ISR 微秒级 AB 采样、ADC 电平记录、truth table 实测
- ESP32 rotary backend 正式接入：GA=GPIO4/BB=GPIO5，ISR → 16-entry lookup decoder →
  signed accumulator（4/detent）→ `NAV_NEXT`/`NAV_PREV`，无固定 debounce
- ESP32 rotary press：GA ADC oneshot LOW/MID/HIGH 分类 + press 状态机（15ms debounce）
  → 复用 `PET_EVENT_BUTTON`（INTERACT）；按压与 quadrature 隔离，按压期间导航暂停
- Core `SLEEP + INTERACT` 唤醒语义；PC SPACE 与 ESP32 PRESS 共享同一 interaction 事件
- ESP32 GPIO7 LEDC PWM Backlight HAL backend，支持 default/sleep 亮度
- V0.6 真机验收：press+rotate ×10、ADC margin、10 分钟混合压力稳定性全部通过
- 平台无关 Time Service：`pet_time_snapshot_t`（valid/hour/minute/second），system
  backend 为 `time()`+`localtime_r`，epoch 阈值判定有效性
- 平台无关 Network provider：`DISCONNECTED/CONNECTING/CONNECTED/ERROR` 状态快照
- ESP32 Wi-Fi STA backend：后台连接、断线 backoff 重连（1s→…→30s）、GOT_IP 后自动
  启动 ESP-NETIF SNTP（pool.ntp.org）
- Renderer 顶部 HH:MM 时钟 overlay（未同步显示 `--:--`），只消费 runtime 每秒缓存的
  snapshot；时区为 `PET_TIMEZONE` POSIX TZ 配置
- `wifi_config.local.h` 本地凭据机制（模板入库、真实凭据 .gitignore）

## 开发中

- V0.7 真机验收：离线启动 / AP 中断恢复 / 30 分钟稳定性待测

## 计划中

- 8080 backend（是否实现由实测性能和接线决定）
- Display HAL 边界和错误路径测试补充
- LEDC PWM 背光参数实机确认与硬件 fade
- 动画画面目视确认（花屏、错色、坏帧、撕裂）
- 更高 SPI 频率或更大 staging 的目视验证与选择
- dirty rectangle 最小接口设计与实现
- Wi-Fi
- 网络与时间同步
- 天气
- PC 通信
- AI 交互
- 传感器
- 持久化 storage

## 暂不实现

- 在线 AI 和云端推理
- 天气 API 接入
- OTA
- 大型 GUI 框架和复杂窗口系统
- 复杂运行时配置系统
- 复杂 LCD GUI 和未验证硬件 backend
