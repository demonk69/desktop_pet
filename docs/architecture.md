# 软件架构

## 总体结构

V0.4 继续采用依赖倒置：业务状态不认识 SDL、ESP-IDF、LCD、SPI、GPIO 或 PWM；平台差异通过 HAL backend
注入。App 层是唯一负责把事件、Pet Core 和动画播放器编排在一起的模块。

```text
 Event Producers                 Asset Provider
 timer/button/service/...        Flash / PSRAM / FS / PC
          |                              |
          v                              v
   +-------------+     intent     +-------------+
   | Event Queue | ---> Pet Core ->|  Animation  |
   +-------------+                 +-------------+
          ^                              |
          | animation done               | current frame
          +------------------------------+
                         |
                         v
                 +---------------+
                 | UI / Renderer |
                 +---------------+
                         |
                         v
                   Display API
                         |
              +----------+----------+
              |                     |
     PC framebuffer/SDL       ESP32 Display
                                    |
                              ESP-IDF SPI
                                    |
                                  ST7789
```

`pet_app_t` 拥有 Core、Animation Player 和 Event Queue，没有进程级可变全局对象，因此未来
可以创建测试实例或多个独立实例。

## 模块职责

| 模块 | 职责 | 不负责 |
|---|---|---|
| `core/` | 状态迁移、事件语义、行为意图 | 绘制、GPIO、资源读取 |
| `app/pet/` | 事件分发、动画完成回送、模块生命周期 | 决定像素和总线时序 |
| `animation/` | 帧时间轴、循环、切换、完成通知 | 资源存放位置、显示输出 |
| `ui/` | 将 App 快照转换为 Display API 调用 | 改变 Pet 状态、调用 ST7789 |
| `hal/` | 稳定的平台无关设备 API | MCU SDK 细节 |
| `drivers/` | 平台/器件 backend | 宠物业务逻辑 |
| `config/` | 集中默认值与有效性检查 | 隐式探测未知硬件 |
| `services/` | 未来网络、AI、时间等事件生产者 | 直接修改 Core 内部状态 |
| `simulator/` | PC 显示 backend 与宿主入口 | 复制另一套 Pet Core |
| `platform/esp32/` | ESP-IDF target、board config 和真机 backend | 平台无关业务规则 |

## 允许依赖

```text
simulator/main or embedded/main -> app + ui + selected HAL backends
app                            -> core + animation + event queue
ui                             -> app read-only snapshot + Display HAL
drivers                        -> HAL contracts + config
core                           -> event and animation identifiers only
services                       -> event publishing contract
```

禁止的依赖包括 `core -> ui`、`core -> drivers`、`ui -> ST7789/SPI/8080`、
`services -> core internals` 和任何业务模块直接访问 GPIO/PWM。

## 状态机

当前状态为 `BOOT`、`IDLE`、`BLINK`、`LOOK_LEFT`、`LOOK_RIGHT`、`HAPPY`、`SLEEP`。
状态进入时仅产生动画意图；App 层执行 `pet_play_animation()`。非循环动画完成后通过
`PET_EVENT_ANIMATION_DONE` 回到状态机，而不是由 UI 直接切换状态。

Idle 根据 timer 累计值依次触发眨眼和左右看。Button/Message/Happy 触发开心，Look 携带
左右方向，Sleep/Wake 控制睡眠。传感器、网络和系统事件已有类型，但 V0.2 不定义其业务语义。

## 动画与资源

播放器依赖 `pet_animation_catalog_t::get_clip`，clip 只包含逻辑 `asset_id`、逐帧时长和循环
标志。Renderer 通过 `pet_asset_provider_t` 把当前 `asset_id` 解析为只读 RGB565 bitmap。
catalog 管时间轴，asset provider 管像素存储，两者均不让 Animation Core 执行文件 IO。

PC file provider 从 manifest 懒加载 P3 PPM 并缓存为 RGB565；ESP32 compiled provider 在启动时
把编译进固件的 6x6 palette 数据展开到静态 RGB565 缓存。两者都向 Renderer 提供同一种只读
bitmap 视图，且真机每帧不分配资源。未注入 provider 时仍保留 V0.1 矢量 fallback。

## Display HAL

`pet_display_t` 是 context + ops table，提供 init、clear、pixel、bitmap、region、flush、
rotation、width/height 和 brightness。PC framebuffer、SDL 和 ESP32 ST7789 backend 均实现
同一 API；旧 stub 继续作为未选择 backend 的显式 unsupported 边界。

Renderer 永远只看 Display API。V0.3 ESP32 backend 是经实机验证的 ST7789-over-SPI 实现；
未实现的 8080 不进入该 backend。独立背光策略使用 Backlight HAL，当前 GPIO backend 只提供
开关语义，LEDC fade 留待确有调光需求时实现。

## ESP32 Platform

```text
                         Shared
                            |
              +-------------+-------------+
              |                           |
         PC Platform                 ESP32 Platform
              |                           |
             SDL2                    ESP-IDF SPI2
                                          |
                                        ST7789
```

ESP-IDF 工程位于 `platform/esp32/`，其 `pet_shared` component 直接引用仓库中的 Core、App、
Animation、Event、Renderer 和通用 HAL 源文件。没有 ESP32 专用 Core 或 Renderer。

ESP32 Display backend 在 8 MB PSRAM 中分配一个 240x240 RGB565 framebuffer。Renderer 的
逻辑颜色仍是 `RRRRRGGGGGGBBBBB`；`flush` 在平台边界转换为 ST7789 要求的 MSB-first 字节流。
V0.5 使用 40 MHz SPI、`SPI_DMA_CH_AUTO` 和 4096-byte 内部 DMA-capable staging：每次
transaction 先把该块 PSRAM 像素打包进内部 buffer，再同步等待 DMA 发送。PSRAM framebuffer
不作为 DMA transaction 的直接 source，也不搬进内部 SRAM。

V0.4 `app_main` 只组装 board、HAL、App、compiled provider 和 Renderer。ESP32 runtime 用
`esp_timer_get_time()` 计算真实 `delta_ms`，每帧依次调用共享 `pet_app_update()`、读取快照和
调用 Renderer。Core 自带 Blink/Look idle 序列；平台演示器仅通过 Event Queue 注入 Happy、
Sleep、Wake，不直接设置状态。Renderer 仍拥有同步 flush。

当前目标周期为 100 ms。同步全帧处理超过预算时 runtime 至少 yield 一个 RTOS tick；实测约
10.0 FPS（flush 平均 31.5 ms、整帧约 68.6 ms，受 100 ms pacing 限制）。Display backend
记录最近一次 flush 耗时，runtime 用 Renderer 调用总耗时减去 flush 得到纯绘制时间，并每
五秒报告 App、绘制、flush、整帧和 heap 指标。

LCD 链路参数（SPI 频率、staging 大小、DMA 开关、分块高度）集中在
`platform/esp32/CMakeLists.txt` 的 cache 变量中，编译为 `pet_esp32_board_config_t` 字段。
ESP-IDF 拒绝 non-DMA 超过 64-byte 的单次 transaction，该约束在 display init 中提前校验。
Dirty rectangle 暂不实现：Renderer 没有 previous frame、changed region 或动画帧边界信息，
需要先增加极小的平台无关 dirty-bounds 接口。

## 事件架构

事件是值类型，固定容量队列避免动态分配。事件生产者只投递 `pet_event_t`；App 在单一执行
上下文消费事件。V0.1 队列满时 `push` 返回 false，由调用者决定丢弃或记录。未来接入 RTOS
时可替换队列实现，但保留事件结构和 Core handler。

## PC Simulator

```text
SDL2 keyboard/window
        |
        +--> Input Adapter --> pet_event_t --> App/Event Queue --> Pet Core
        |
        +--> SDL Display backend <-- Display HAL <-- Shared Renderer
                                         ^
P3 files --> PC File Asset Provider ------+
```

SDL 主循环只负责平台事件、单调时间差、渲染调用和 frame pacing。Core 接收 `delta_ms`，
不调用 `SDL_GetTicks64()`。SDL Display backend 拥有窗口、texture 和 RGB565 framebuffer，
`flush` 才把共享 Renderer 的输出提交到窗口。

## 平台归属

| 范围 | 模块 |
|---|---|
| Shared | `core/`、`app/pet/`、`animation/`、`ui/`、`hal/` 公共契约 |
| PC only | SDL/framebuffer backend、SDL input adapter、P3 file provider、PC main |
| ESP32 only | ESP-IDF main/runtime/time、compiled provider、board config、SPI/ST7789、GPIO backlight |
| Future embedded | LEDC、DMA/局部刷新优化、其他 backend |

## PC 与真机复用

PC 和真机只在入口、时钟/事件源、Display backend、Backlight backend 和资源 provider 上
不同。以下模块必须原样复用：`core/`、`app/pet/`、`animation/`、`ui/` 的平台无关部分。

```text
                    shared pet_core + animation + renderer
                         /                       \
        PC event loop + SDL/framebuffer   SDK task + ST7789 backend
```
