# TODO

## Hardware Bring-up

- [ ] `[HW_VERIFY]` MCU 具体型号和目标 SDK
- [ ] `[HW_VERIFY]` LCD 分辨率
- [ ] `[HW_VERIFY]` LCD X/Y offset
- [ ] `[HW_VERIFY]` RGB/BGR 顺序与 RGB565 字节序
- [ ] `[HW_VERIFY]` SPI mode 和最高稳定频率
- [ ] `[HW_VERIFY]` 8080 数据宽度与时序
- [ ] `[HW_VERIFY]` GPIO mapping
- [ ] `[HW_VERIFY]` 背光有效电平与 PWM 频率
- [ ] `[HW_VERIFY]` ST7789 初始化序列、reset timing 和 rotation
- [ ] `[HW_VERIFY]` PSRAM 容量、速度及 DMA 能力
- [ ] `[HW_VERIFY]` framebuffer 是否放 PSRAM
- [ ] `[HW_VERIFY]` 全帧、双缓冲或分块刷新策略
- [ ] `[HW_VERIFY]` Flash 容量、分区和资源预算
- [ ] `[HW_VERIFY]` 电源与 LCD/backlight sleep/wake 顺序

## Software V0.2

- [ ] 选择并验证交互式 PC 窗口方案（候选 SDL2）
- [ ] 定义位图资源格式和 PC 文件型 asset provider
- [ ] 增加 Renderer/display backend 契约测试
- [ ] 定义事件队列满时的产品级丢弃、计数或告警策略
- [ ] 增加确定性行为调度配置，避免未来随机行为难以测试

详细硬件检查顺序见 `docs/hardware.md`，版本计划见 `docs/roadmap.md`。
