# 软件架构

## 总体结构

V0.1 采用依赖倒置：业务状态不认识 LCD、SPI、GPIO 或 PWM；平台差异通过 HAL backend
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
        PC framebuffer       Embedded display
                                    |
                           SPI or 8080 backend
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

Idle 根据 timer 累计值依次触发眨眼和左右看。Button/Message 触发开心；Sleep/Wake 控制
睡眠。传感器、网络和系统事件已有类型，但 V0.1 不定义其业务语义。

## 动画与资源

播放器依赖 `pet_animation_catalog_t::get_clip`，clip 只包含逻辑 `asset_id`、逐帧时长和循环
标志。资源提供方以后可以把 `asset_id` 解析到 Flash、PSRAM、文件系统或 PC 文件。

V0.1 Renderer 用逻辑 asset ID 绘制简单矢量占位表情，以便验证流程。加入真实位图时应新增
资源解析接口，不应让动画播放器读取文件或 Flash 地址。

## Display HAL

`pet_display_t` 是 context + ops table，提供 init、clear、pixel、bitmap、region、flush、
rotation、width/height 和 brightness。当前 framebuffer backend 可运行；SPI、8080、ST7789
factory 返回 `PET_STATUS_NOT_SUPPORTED`，因为目标 SDK 和硬件参数尚未确认。

ST7789 实现时应分成两层：ST7789 命令/窗口逻辑依赖 LCD bus transport，transport 再分别由
SPI 和 8080 实现。Renderer 永远只看 Display API。Display 中 brightness 用于具备统一设备
控制能力的 backend；独立背光策略（fade/sleep/wake）使用 Backlight HAL。

## 事件架构

事件是值类型，固定容量队列避免动态分配。事件生产者只投递 `pet_event_t`；App 在单一执行
上下文消费事件。V0.1 队列满时 `push` 返回 false，由调用者决定丢弃或记录。未来接入 RTOS
时可替换队列实现，但保留事件结构和 Core handler。

## PC 与真机复用

PC 和真机只在入口、时钟/事件源、Display backend、Backlight backend 和资源 provider 上
不同。以下模块必须原样复用：`core/`、`app/pet/`、`animation/`、`ui/` 的平台无关部分。

```text
                    shared pet_core + animation + renderer
                         /                       \
          PC event loop + framebuffer    SDK task + ST7789 backend
```
