# Desktop Pet

面向 TFT LCD 桌面智能桌宠的软件工程。当前已完成 **V0.5 ESP32-S3 LCD 性能优化**：
同一套 Pet Core、App、Animation 和 Renderer 可运行在 SDL2 PC 模拟器和 ESP-IDF 真机上。

## 当前实现

- C11 Pet Core 状态机：启动、Idle、眨眼、左右看、开心、睡眠
- 固定容量轻量事件队列和事件驱动的 App 编排层
- 支持逐帧时长、循环、切换和完成通知的动画播放器
- 动画 catalog 抽象，资源位置不与播放器绑定
- bitmap asset provider 抽象和 PC 文件资源 provider
- Display HAL、Backlight HAL，以及保留的未实现 backend stub
- 与硬件无关的 Renderer
- 无外部 GUI 依赖的 PC framebuffer 模拟器，可导出 PPM 图像
- 可选 SDL2 Display backend、实时主循环和键盘输入适配
- P3 PPM 像素动画资源：boot、idle、blink、look、happy、sleep
- 状态机、事件、动画、配置、App/Renderer 集成和输入映射测试
- 独立 ESP-IDF 6.1 target，直接编译共享业务和 Renderer 源码
- ESP32-S3 SPI/ST7789 Display backend 和 GPIO Backlight backend
- PSRAM 单 framebuffer 和同步 RGB565 MSB-first flush
- 无文件系统、无每帧分配的 ESP32 compiled RGB565 asset provider
- 真机 runtime、Event Queue 演示事件和五秒性能/内存统计
- LCD 显示链路集中编译参数和完整单变量性能矩阵
- 默认显示配置：40 MHz SPI + DMA + 4096-byte 内部 staging，10.0 FPS

V0.5 未实现 LEDC 调光、双缓冲、局部刷新、8080 或在线服务；动画画面（花屏、错色、
坏帧、撕裂）仍需人工目视确认。尚未确认的电气和性能上限继续标记为 `HW_VERIFY`。

## 构建与测试

要求 CMake 3.16+ 和支持 C11 的编译器。SDL2 仅用于可选 GUI simulator。

Ubuntu/Debian 安装 SDL2 开发包：

```sh
sudo apt install libsdl2-dev
```

Fedora 安装：

```sh
sudo dnf install SDL2-devel
```

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

没有 SDL2 时，CMake 会跳过 `desktop_pet_simulator`，Core、测试和 framebuffer simulator
仍可构建。也可显式关闭 SDL 目标：

```sh
cmake -S . -B build -DPET_BUILD_SDL_SIMULATOR=OFF
```

## 运行 SDL2 模拟器

```sh
./build/desktop_pet_simulator
```

默认逻辑分辨率为 240x240，窗口放大 3 倍并使用 nearest-neighbor 缩放。可指定 1 到 8 倍
窗口缩放或资源目录：

```sh
./build/desktop_pet_simulator --scale 2
./build/desktop_pet_simulator --assets ./assets/pet
```

键盘控制：

| 按键 | 事件/行为 |
|---|---|
| `SPACE` | 普通按键交互 |
| `H` | 开心 |
| `S` | 睡眠 |
| `W` | 唤醒 |
| `M` | 模拟消息 `Hello Pet` |
| `LEFT` | 向左看 |
| `RIGHT` | 向右看 |
| `ESC` | 退出 |

输入路径始终是 `SDL event -> simulator input adapter -> pet_event_t -> App queue -> Core`，
SDL 不会直接修改桌宠状态。

## 无 GUI 模拟器

```sh
./build/pet_simulator
```

程序会打印状态切换，并在当前目录生成 `pet_simulator.ppm`。也可以指定输出路径：

```sh
./build/pet_simulator build/final-frame.ppm
```

该目标保留用于无 SDL 环境、CI 和快速验证。

## ESP32-S3 固件

要求 ESP-IDF 6.1：

```sh
source /home/lab_726/.espressif/tools/activate_idf_v6.1.sh
cd platform/esp32
idf.py build
idf.py -p /dev/ttyACM0 flash
idf.py -p /dev/ttyACM0 monitor
```

`flash` 只写入构建产物，不需要 `erase-flash`。当前固件持续运行共享 App/Animation/Renderer，
使用 compiled assets 播放完整演示序列并定期输出性能统计。默认显示链路为 40 MHz SPI、
DMA 开启、4096-byte 内部 staging。LCD 参数可用编译变量覆盖用于 benchmark：

```sh
idf.py -D PET_LCD_SPI_FREQUENCY_HZ=40000000 \
       -D PET_LCD_STAGING_BUFFER_SIZE=4096 \
       -D PET_LCD_DMA_ENABLED=1 \
       -D PET_LCD_BLOCK_HEIGHT=0 build flash -p /dev/ttyACM0
```

详细说明见 [`platform/esp32/README.md`](platform/esp32/README.md)。

## 目录

```text
app/pet/             App 编排层
animation/           动画播放器与内置 catalog
assets/              PC P3 PPM 动画资源与 manifest
config/              集中配置与校验
core/                状态机与事件队列
drivers/display/     SPI、8080、ST7789 backend/stub
drivers/backlight/   PWM 背光 backend/stub
hal/                 平台无关 Display/Backlight API 包装
include/             公共 API
main/                嵌入式入口说明
platform/esp32/      独立 ESP-IDF target 和 ESP32 HAL backend
services/            未来服务边界
simulator/           framebuffer/SDL backend、输入适配和 PC file provider
tests/               无硬件测试及可选 SDL 输入适配测试
ui/                  Renderer
docs/                架构、硬件、功能和开发文档
```

## 硬件状态

已实机确认 ESP32-S3 revision v0.2、16 MB Flash、8 MB Octal PSRAM 80 MHz，以及 SPI
ST7789 240x240。当前默认使用 SPI2 mode 0、40 MHz、RGB565/RGB、rotation 0、offset 0/0、
SPI DMA 与 4096-byte 内部 staging，实测 10.0 FPS（pacing 受限），flush 平均约 31.5 ms，
整帧约 68.6 ms；候选配置矩阵和 60 秒稳定性数据见 `platform/esp32/README.md`。
动画画面目视确认、撕裂观感、PWM 调光和电气细节仍待验证。

完整 Bring-up 清单见 [`docs/hardware.md`](docs/hardware.md) 和 [`TODO.md`](TODO.md)。

## 下一步

1. 人工目视确认 40 MHz DMA 动画画面无花屏、错色、坏帧和影响体验的撕裂。
2. 若帧率仍不足，目视验证 80 MHz 与更大 staging，再决定是否调整默认配置。
3. 评估 dirty rectangle：先设计极小的平台无关 dirty-bounds 接口。
4. 需要可调亮度时实现 LEDC backend。

架构约束和扩展步骤分别见 [`docs/architecture.md`](docs/architecture.md) 与
[`docs/development.md`](docs/development.md)。
