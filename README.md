# Desktop Pet

面向 TFT LCD 桌面智能桌宠的软件工程。当前处于 **V0.1 软件骨架阶段**：目标是在开发板
到货前验证状态机、事件、动画时间轴和渲染边界，硬件到货后主要替换 HAL/backend，而不是
重写桌宠业务逻辑。

## 当前实现

- C11 Pet Core 状态机：启动、Idle、眨眼、左右看、开心、睡眠
- 固定容量轻量事件队列和事件驱动的 App 编排层
- 支持逐帧时长、循环、切换和完成通知的动画播放器
- 动画 catalog 抽象，资源位置不与播放器绑定
- Display HAL、Backlight HAL，以及 SPI/8080/ST7789/PWM 的明确 stub
- 与硬件无关的 Renderer
- 无外部 GUI 依赖的 PC framebuffer 模拟器，可导出 PPM 图像
- 状态机、事件队列、动画和配置基础测试

未实现真实 ST7789 初始化、SPI/8080 传输、PWM 背光和在线服务。原因是 MCU、引脚、
LCD 模组参数及目标 SDK 尚未确认，相关位置均标记为 `HW_VERIFY`。

## 构建与测试

要求 CMake 3.16+ 和支持 C11 的编译器。

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

关闭可选目标：

```sh
cmake -S . -B build -DPET_BUILD_SIMULATOR=OFF -DPET_BUILD_TESTS=OFF
```

## 运行模拟器

```sh
./build/pet_simulator
```

程序会打印状态切换，并在当前目录生成 `pet_simulator.ppm`。也可以指定输出路径：

```sh
./build/pet_simulator build/final-frame.ppm
```

V0.1 选择 PPM framebuffer 而不是 SDL2，以保持零额外依赖。V0.2 可在 `simulator/` 增加
交互窗口，仍复用同一个 `pet_app_t`、状态机、动画播放器和 Renderer。

## 目录

```text
app/pet/             App 编排层
animation/           动画播放器与内置 catalog
assets/              资源约定；后续放置资源或生成工具
config/              集中配置与校验
core/                状态机与事件队列
drivers/display/     SPI、8080、ST7789 backend/stub
drivers/backlight/   PWM 背光 backend/stub
hal/                 平台无关 Display/Backlight API 包装
include/             公共 API
main/                未来目标 SDK 入口
services/            未来服务边界
simulator/           PC framebuffer backend 与 demo
tests/               无硬件单元测试
ui/                  Renderer
docs/                架构、硬件、功能和开发文档
```

## 硬件状态

目前只知道显示控制器方向为 ST7789、屏幕支持 SPI/8080、开发板约有 8 MB 外置 PSRAM
和约 16 MB Flash。具体 MCU、分辨率、偏移、GPIO、总线时序、色序、背光和 DMA 能力均
未确认。模拟器使用的 240x240 是开发用逻辑画布，不是物理屏规格结论。

完整 Bring-up 清单见 [`docs/hardware.md`](docs/hardware.md) 和 [`TODO.md`](TODO.md)。

## 下一步

1. V0.2 增加可交互 PC 窗口和更真实的资源加载器。
2. 开发板到货后确认 MCU/SDK、原理图、LCD 模组和 GPIO。
3. 按确认结果实现 SPI 或 8080 backend、ST7789 与 PWM 背光。
4. 在真机上验证内存策略、刷新路径和动画帧率。

架构约束和扩展步骤分别见 [`docs/architecture.md`](docs/architecture.md) 与
[`docs/development.md`](docs/development.md)。
