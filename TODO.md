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
- [ ] `[HW_VERIFY]` 背光 PWM 频率、分辨率和 LEDC fade
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
- [x] 实现 compiled asset provider 和受控真机动画循环
- [x] 连续运行 305 秒，确认 internal heap/PSRAM 无持续下降
- [x] 完成 LCD 性能矩阵实测并固化为 40 MHz + DMA + 4 KiB staging 默认值

详细硬件检查顺序见 `docs/hardware.md`，版本计划见 `docs/roadmap.md`。
