# 硬件记录与 Bring-up Checklist

## 当前已知

- 产品方向：桌面智能桌宠
- 显示类型：TFT LCD
- 显示控制器方向：ST7789
- LCD 接口能力：SPI / 8080 并口
- 外置 PSRAM：约 8 MB
- Flash：约 16 MB
- 开发板和屏幕实物：尚未到货

以上容量为当前输入信息，仍需通过具体料号、原理图和启动日志确认。

## 当前配置策略

`pet_config_t` 集中保存显示、总线、GPIO、背光和 framebuffer 参数。未知数字使用
`PET_VALUE_HW_VERIFY`（0）或 `PET_GPIO_HW_VERIFY`（-1）。开发默认的 240x240 只用于 PC
模拟器逻辑画布，不能直接用于真机配置。

## HW_VERIFY 清单

- [ ] `[HW_VERIFY]` MCU 具体型号、核心架构和目标 SDK
- [ ] `[HW_VERIFY]` 开发板原理图、供电电压和 IO 电平
- [ ] `[HW_VERIFY]` LCD 模组具体料号和物理分辨率
- [ ] `[HW_VERIFY]` LCD 可视区域、GRAM 尺寸和 X/Y offset
- [ ] `[HW_VERIFY]` ST7789 具体变体及初始化序列
- [ ] `[HW_VERIFY]` RGB/BGR 顺序、RGB565 字节序和反色设置
- [ ] `[HW_VERIFY]` MADCTL rotation 对应关系
- [ ] `[HW_VERIFY]` 实际采用 SPI 还是 8080 接口
- [ ] `[HW_VERIFY]` SPI mode、最高稳定频率和 DMA 限制
- [ ] `[HW_VERIFY]` 8080 数据宽度、WR/RD 时序和 DMA 支持
- [ ] `[HW_VERIFY]` MOSI/SCLK/CS/DC/RESET GPIO mapping
- [ ] `[HW_VERIFY]` 8080 D0..Dn、WR、RD、CS、DC GPIO mapping
- [ ] `[HW_VERIFY]` RESET 和上电延时要求
- [ ] `[HW_VERIFY]` 背光 GPIO、有效电平和驱动电路
- [ ] `[HW_VERIFY]` 背光 PWM 外设、频率、分辨率和是否可硬件 fade
- [ ] `[HW_VERIFY]` 电源域、睡眠唤醒顺序和 LCD 休眠命令
- [ ] `[HW_VERIFY]` Flash 实际容量、分区和可用于资源的空间
- [ ] `[HW_VERIFY]` PSRAM 实际容量、速度、缓存一致性和 DMA 能力
- [ ] `[HW_VERIFY]` framebuffer 是否放 PSRAM
- [ ] `[HW_VERIFY]` 全帧、双缓冲或分块刷新的内存/性能选择
- [ ] `[HW_VERIFY]` 屏幕撕裂信号 TE 是否引出和是否使用
- [ ] `[HW_VERIFY]` 按键、传感器和其他外设 GPIO/总线

## 到货后验证顺序

1. 根据料号和原理图建立目标 SDK 工程，确认 Flash/PSRAM 和日志串口。
2. 仅实现 GPIO、延时和目标 LCD bus backend，使用逻辑分析仪检查时序。
3. 验证 reset、read ID（若可用）和最小 ST7789 初始化序列。
4. 输出纯色和定位图，确认分辨率、offset、rotation、色序和字节序。
5. 测试 SPI/8080 不同速率及 DMA，记录稳定上限。
6. 实现 PWM 背光并验证有效电平、频率、fade、sleep/wake。
7. 评估 framebuffer 所在内存及刷新策略，再启用真机 Renderer。

所有实测结论应回填本文件，并移除对应代码中的 `HW_VERIFY`，不能只修改魔法数字。
