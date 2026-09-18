# 硬件记录与 Bring-up Checklist

## 已验证硬件基线

- MCU：ESP32-S3 revision v0.2，双核
- SDK：ESP-IDF 6.1
- Flash：16 MB
- PSRAM：8 MB Octal PSRAM，频率 80 MHz，初始化成功
- PSRAM 基础读写：显式申请 1 MB `MALLOC_CAP_SPIRAM` 内存成功，1 MB 全量写入/读回通过
- USB 接口：USB-Serial/JTAG
- 工具链：ESP-IDF build/flash/monitor 流程已通过实机验证

实测输出：

```text
=== Desktop Pet Hardware Test ===
Chip model: esp32s3, revision v0.2, cores: 2
Flash size: 16777216 bytes
PSRAM initialized: YES
PSRAM size: 8388608 bytes
PSRAM free: 8386308 bytes
PSRAM 1MB allocation: OK
PSRAM 1MB read/write test: PASS
=== Hardware Test Complete ===
```

## 已验证显示基线

- 显示类型：TFT LCD
- 控制器：ST7789
- 分辨率：240x240
- 像素格式：RGB565，RGB 顺序
- 接口：SPI2，mode 0
- 当前验证频率：40 MHz（polling 10/20/40/80 MHz 与 DMA 40 MHz 均已实测）
- rotation：0，MADCTL `0x00`
- X/Y offset：0/0
- inversion：off，使用 `INVOFF (0x20)`
- RGB565 wire order：MSB first
- reset timing：低电平 20 ms，释放后等待 120 ms
- 真机结果：共享 Renderer 静态画面内容、颜色和方向正确；V0.4 持续动画日志稳定

10 MHz 曾是稳定验证值；V0.5 实测并选择了 40 MHz。LCD 模组具体料号仍需补录。

## 已验证接线

| LCD | ESP32-S3 |
|---|---:|
| BL | GPIO7 |
| RESET | GPIO8 |
| DC/RS | GPIO9 |
| CS | GPIO10 |
| MOSI | GPIO11 |
| SCK | GPIO12 |
| GND | GND |
| 3V3 | 3V3 |

背光 GPIO7 高电平开启。V0.3 仅实现开关，不代表 PWM 参数已验证。

## 已验证初始化序列

```text
SWRESET 0x01, delay 150 ms
SLPOUT  0x11, delay 120 ms
COLMOD  0x3A 0x55
MADCTL  0x36 0x00
CASET   0x2A 0x0000..0x00EF
RASET   0x2B 0x0000..0x00EF
RAMWR   0x2C
INVOFF  0x20
NORON   0x13, delay 10 ms
DISPON  0x29, delay 100 ms
```

## 当前配置策略

通用 `pet_config_t` 保存平台无关硬件字段；ESP32 已验证参数集中在
`platform/esp32/main/pet_esp32_board_config.c`。PC development defaults 与 ESP32 board config
分别构造，业务代码和 Renderer 不包含 GPIO 数字。

V0.4 在 PSRAM 分配一个 115200-byte framebuffer，初始为 SPI 关闭 DMA、64-byte 内部 staging
的同步发送路径，实测约 5.2 FPS、flush 平均 149.6 ms。

V0.5 对显示链路做了单变量实测。ESP-IDF 拒绝 non-DMA 超过 64-byte 的单次 transaction，
因此 polling staging 只测试了 64 B；时钟从 10 MHz 逐档到 80 MHz（60 MHz 请求值与 40 MHz
落在同一 divider，已用 `spi_device_get_actual_freq` 确认）；随后在 40 MHz 上启用
`SPI_DMA_CH_AUTO`，staging 测试 64/512/1024/4096/8192 bytes，最后测试 1/4/8/16/32 行分块。
每个候选配置都在真机上运行动画演示至少 60 秒。

实测摘要（详见 `platform/esp32/README.md` 性能表）：

- polling：10 MHz flush 149.63 ms/5.2 FPS，20 MHz 103.28 ms/6.6 FPS，40 MHz 79.87 ms/8.3 FPS，
  80 MHz 68.60 ms/9.0 FPS
- DMA 40 MHz staging：64 B 85.34 ms，512 B 37.48 ms，1 KiB 34.04 ms，4 KiB 31.48 ms，
  8 KiB 31.07 ms
- DMA 40 MHz 行分块：1 行 37.91 ms，4 行 32.43 ms，8 行 31.51 ms，16 行 31.07 ms，
  32 行 30.85 ms

收益在 4 KiB 后明显递减：8 KiB 多花 4 KiB 内部 RAM 只再省 0.42 ms；最好的 32 行分块需要
15 KiB 内部 RAM，仅比 4 KiB 连续 staging 快约 2%。

最终选择（V0.5 固件默认）：SPI 40 MHz、DMA on、4096-byte 内部 DMA-capable staging、
不分行分块、单一 PSRAM framebuffer。60 秒回归：10.0 FPS，flush 平均 31.48 ms / 最大
31.52 ms，纯绘制平均 37.10 ms，整帧平均 68.61 ms；free internal heap 固定 372315 bytes，
free PSRAM 固定 8269568 bytes。

稳定性判定目前基于串口日志：全程无 SPI 错误、无 watchdog、无重启、heap 不下降。
屏幕花屏、错色、坏帧和撕裂观感仍需人工目视确认；60/80 MHz polling 与 32 行分块虽日志
稳定，因缺目视确认且收益有限，未选为默认值。

## HW_VERIFY 清单

- [x] `[已验证]` MCU 为 ESP32-S3 revision v0.2、双核，目标 SDK 为 ESP-IDF 6.1
- [ ] `[HW_VERIFY]` 开发板原理图、供电电压和 IO 电平
- [ ] `[HW_VERIFY]` LCD 模组具体料号
- [x] `[已验证]` 物理分辨率 240x240、GRAM 可视区域、X/Y offset 0/0
- [x] `[已验证]` 当前 ST7789 初始化序列
- [x] `[已验证]` RGB 顺序、RGB565 MSB-first 和 INVOFF
- [x] `[已验证]` rotation 0、MADCTL `0x00`
- [x] `[已验证]` 当前接口采用 SPI2 mode 0
- [x] `[已验证]` SPI 10/20/40/80 MHz polling 与 40 MHz DMA 串口稳定，60 MHz 请求回落 40 MHz
- [ ] `[HW_VERIFY]` SPI 最高稳定频率（80 MHz 缺目视确认）和更长期 DMA 稳定性
- [ ] `[HW_VERIFY]` 8080 数据宽度、WR/RD 时序和 DMA 支持
- [x] `[已验证]` MOSI=11、SCLK=12、CS=10、DC=9、RESET=8
- [ ] `[HW_VERIFY]` 8080 D0..Dn、WR、RD、CS、DC GPIO mapping
- [x] `[已验证]` RESET 低 20 ms，释放后等待 120 ms
- [x] `[已验证]` 背光 GPIO7，高电平开启
- [ ] `[HW_VERIFY]` 背光驱动电路和电流能力
- [ ] `[HW_VERIFY]` 背光 PWM 外设、频率、分辨率和是否可硬件 fade
- [ ] `[HW_VERIFY]` 电源域、睡眠唤醒顺序和 LCD 休眠命令
- [x] `[已验证]` Flash 实际容量为 16 MB
- [ ] `[HW_VERIFY]` Flash 分区和可用于资源的空间
- [x] `[已验证]` PSRAM 为 8 MB Octal PSRAM、80 MHz，初始化及 1 MB 全量写入/读回通过
- [ ] `[HW_VERIFY]` PSRAM 缓存一致性和 DMA 能力
- [x] `[已验证]` 单 framebuffer 可放入 PSRAM 并完成静态 Renderer 显示
- [x] `[已验证]` 非 DMA、64-byte staging 的同步全帧刷新
- [x] `[已验证]` 真机同步单 framebuffer 动画性能和五分钟内存稳定性基线
- [x] `[已验证]` 40 MHz + DMA + 4 KiB staging 的 60 秒稳定性和性能基线
- [ ] `[HW_VERIFY]` 双缓冲、dirty rectangle 和 TE 同步
- [ ] `[HW_VERIFY]` 屏幕撕裂信号 TE 是否引出和是否使用
- [ ] `[HW_VERIFY]` 按键、传感器和其他外设 GPIO/总线

## 后续硬件验证顺序

1. 补录 LCD 模组和开发板具体料号、原理图、电压及背光驱动能力。
2. 人工目视确认 40 MHz DMA 画面与动画无花屏、错色、坏帧和影响体验的撕裂。
3. 需要更高帧率时，目视验证 80 MHz 与更大 staging/分块，再决定是否调整默认值。
4. 评估 dirty rectangle：Renderer 当前无 previous frame、changed region 或动画帧边界
   信息，6x6 资源整帧放大且每帧全量重绘；需要先增加极小的平台无关 dirty-bounds 接口。
5. 需要调光时实现 LEDC 并验证 PWM 频率、分辨率、fade、sleep/wake。
6. 仅在产品确实选择 8080 时验证其数据宽度、时序和 GPIO。

所有实测结论应回填本文件，并移除对应代码中的 `HW_VERIFY`，不能只修改魔法数字。
