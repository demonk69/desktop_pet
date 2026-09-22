# TODO

## Hardware Bring-up

- [x] `[已验证]` MCU：ESP32-S3 revision v0.2、双核；SDK：ESP-IDF 6.1
- [x] `[已验证]` LCD：ST7789，240x240，SPI
- [x] `[已验证]` LCD X/Y offset：0/0
- [x] `[已验证]` RGB565 RGB 顺序，SPI wire byte order 为 MSB-first
- [x] `[已验证]` SPI2 mode 0；polling 10/20/40/80 MHz 与 DMA 40 MHz 串口稳定
- [x] `[已验证]` 40 MHz + SPI DMA + 4096-byte 内部 staging 为当前固件默认
- [ ] `[HW_VERIFY]` SPI 80 MHz 及更大 staging 的目视确认和长期稳定性
- [ ] `[HW_VERIFY]` 8080 数据宽度与时序
- [x] `[已验证]` GPIO：BL=7、RESET=8、DC=9、CS=10、MOSI=11、SCLK=12
- [x] `[已验证]` 背光 GPIO7，高电平开启
- [ ] `[HW_VERIFY]` 背光 PWM 频率、分辨率、default/sleep 亮度和 LEDC fade
- [x] `[已验证]` Rotary VCC = 3.3V；模块内部 3.3k 上拉，ESP32 内部 pull disabled
- [x] `[已验证]` GA=GPIO4（ADC1 CH3），BB=GPIO5；idle AB=11
- [x] `[已验证]` transitions per detent=4；CW `11→10→00→01→11`、CCW `11→01→00→10→11`
- [x] `[已验证]` 机械 bounce 约 5~146 µs，±1 交替由 accumulator 自然抵消；invalid=0
- [x] `[已验证]` press 复用 GA：idle ≈3.1V / press ≈1.50V / A 触点闭合 ≈0V
- [x] `[已验证]` press ADC 阈值 LOW<600 / 1000..2200 / HIGH>2600 mV，实测余量充分
- [x] `[已验证]` press debounce 15 ms
- [x] `[已验证]` 按住同时旋转：按压期间导航暂停，release resync 正常
- [x] `[已验证]` ST7789 初始化序列、20/120 ms reset timing、rotation 0、MADCTL 0x00
- [x] `[已验证]` PSRAM：8 MB Octal、80 MHz，初始化及 1 MB 全量写入/读回通过
- [ ] `[HW_VERIFY]` PSRAM 缓存一致性及 DMA 能力
- [x] `[已验证]` 115200-byte 单 framebuffer 可分配到 PSRAM 并完成静态 Renderer 显示
- [x] `[已验证]` 非 DMA、64-byte 内部 staging 的同步全帧刷新可用
- [x] `[已验证]` SPI DMA 经内部 staging 同步发送可用，PSRAM framebuffer 不作为 DMA source
- [x] `[已验证]` 当前同步全帧动画基线：约 5.2 FPS，render 37.1 ms，flush 149.6 ms
- [x] `[已验证]` 优化后基线：10.0 FPS（pacing 受限），flush 31.5 ms，整帧约 68.6 ms
- [ ] `[HW_VERIFY]` 动画画面目视确认（花屏、错色、坏帧、撕裂）
- [ ] `[HW_VERIFY]` 屏幕撕裂信号 TE 是否引出和是否使用
- [x] `[已验证]` Flash 容量：16 MB
- [ ] `[HW_VERIFY]` Flash 分区和资源预算
- [ ] `[HW_VERIFY]` 电源与 LCD/backlight sleep/wake 顺序

## Software Follow-ups

- [ ] 定义事件队列满时的产品级丢弃、计数或告警策略
- [ ] 增加 Display HAL clipping、rotation 和失败路径契约测试
- [ ] 为 P3 provider 增加损坏文件和超大资源的独立测试样本
- [ ] 决定生产资源是否继续使用 PPM，或增加离线转换为嵌入式格式的工具
- [ ] dirty rectangle：先设计极小的平台无关 dirty-bounds 接口，再决定是否实现
- [x] 增加平台无关 `NAV_NEXT` / `NAV_PREV` 和 PC LEFT/RIGHT 映射
- [x] 在 shared App 层增加 15 秒 inactivity sleep 测试逻辑
- [x] 建立 ESP32 rotary backend：GA/BB GPIO ISR → quadrature decoder → NAV 事件（transitions/detent=4）
- [x] 建立 ESP32 LEDC PWM Backlight HAL backend
- [x] 完成 rotary hardware diagnostic（ISR 微秒采样）并回填实测硬件参数
- [x] press 接入：GA ADC LOW/MID/HIGH classifier + press 状态机 → PET_EVENT_BUTTON（INTERACT）
- [x] press 真机验收：单击 50/长按 2s/residual accumulator 专项/press+rotate ×10/10 分钟稳定性
- [x] ADC margin diagnostic：HIGH 3064~3123、MID 1504~1540、LOW 0 mV，阈值余量确认
- [x] inactivity timeout 从测试常量改为集中配置项（`pet_core_config_t.inactivity_sleep_ms`，默认 15s）
- [x] 平台无关 Time Service（snapshot + system backend + epoch 有效性）与 Network provider
- [x] ESP32 Wi-Fi STA backend：后台连接、backoff 重连、GOT_IP 后启动 ESP-NETIF SNTP
- [x] Renderer HH:MM 时钟 overlay（未同步 `--:--`），runtime 每秒缓存 snapshot
- [x] `PET_TIMEZONE` POSIX TZ 配置与 `wifi_config.local.h` 本地凭据机制
- [ ] V0.7 真机验收：正常 AP / 离线启动 / AP 中断恢复 / sleep-wake 回归 / 30 分钟稳定性
- [x] 实现 compiled asset provider 和受控真机动画循环
- [x] 连续运行 305 秒，确认 internal heap/PSRAM 无持续下降
- [x] 完成 LCD 性能矩阵实测并固化为 40 MHz + DMA + 4 KiB staging 默认值

详细硬件检查顺序见 `docs/hardware.md`，版本计划见 `docs/roadmap.md`。
