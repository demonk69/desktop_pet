# 开发指南

## 编码规范

- 使用 C11，公共 API 放在 `include/`，实现细节留在模块目录。
- 模块拥有自己的状态结构，避免可变全局变量。
- 返回 `pet_status_t` 或明确的 bool；调用边界必须处理失败。
- 业务代码不得包含目标 SDK、GPIO、SPI、8080 或 PWM 头文件。
- 未经实物确认的参数或路径标记 `HW_VERIFY`，并同步 `TODO.md`。
- 注释解释约束和原因，不复述代码。
- 大功能同时更新 `README.md`、`docs/features.md` 和受影响专题文档。

## 构建检查

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/pet_simulator build/final-frame.ppm
./build/desktop_pet_simulator --scale 3
```

测试不得依赖真实 LCD。新增平台 backend 时，核心测试仍应可在普通 PC 上运行。

## ESP32 构建与烧录

ESP-IDF target 与根 host CMake 相互独立：

```sh
source /home/lab_726/.espressif/tools/activate_idf_v6.1.sh
cd platform/esp32
idf.py build
idf.py -p /dev/ttyACM0 flash
idf.py -p /dev/ttyACM0 monitor
```

端口按实际系统调整。正常开发不需要 `erase-flash`，禁止修改 eFuse。`sdkconfig.defaults`
保存已验证的 ESP32-S3、16 MB Flash 和 8 MB Octal PSRAM 基线；生成的 `sdkconfig` 和
`platform/esp32/build/` 不提交。

LCD 显示链路参数是 `platform/esp32/CMakeLists.txt` 中的 cache 变量，可用 `-D` 覆盖进行
benchmark：

```sh
idf.py -D PET_LCD_SPI_FREQUENCY_HZ=40000000 \
       -D PET_LCD_STAGING_BUFFER_SIZE=4096 \
       -D PET_LCD_DMA_ENABLED=1 \
       -D PET_LCD_BLOCK_HEIGHT=0 build flash -p /dev/ttyACM0
```

只允许修改 ESP32 Display backend 和这些集中参数，禁止为性能修改 Pet Core、Animation、
Event System 或 Renderer 的平台无关语义。每个候选配置必须在真机运行至少 60 秒并记录
FPS、flush、frame 和 heap；实测结论回填 `docs/hardware.md` 与 `platform/esp32/README.md`。

## 切换平台 backend

- PC：从仓库根目录运行普通 CMake，选择 framebuffer 或 SDL Display backend。
- ESP32：在 `platform/esp32/` 运行 `idf.py`，main component 构造 ESP32 Display/Backlight。
- ESP32 runtime：只提供时钟、frame pacing、统计和事件生产；状态迁移仍由共享 App/Core 完成。
- ESP32 rotary：GA/BB 与 press 参数已实测（GA=GPIO4/BB=GPIO5、pull 关闭、4 transitions/detent、
  ADC LOW<600/1000..2200/HIGH>2600 mV、debounce 15 ms）。新增/修改 rotary 参数前先看
  `docs/hardware.md` 的实测表，未实测的新参数保持 `HW_VERIFY` 并默认禁用对应功能。
- Shared：Core、App、Animation、Event、Renderer 和 HAL API 不使用平台条件编译。
- 新平台只新增入口、board config 和 HAL backend，不复制共享业务源文件。

## 添加模块

1. 定义单一职责和 owner 状态结构。
2. 在 `include/<module>/` 添加最小公共 API，内部函数不导出。
3. 只依赖 `docs/architecture.md` 允许的下层模块。
4. 添加无硬件测试，并在 CMake 中声明目标。
5. 更新功能和架构文档。

## 添加动画

1. 在 `pet_animation_id_t` 添加 ID。
2. 在 `pet_assets.h` 添加逻辑 asset ID。
3. 在 catalog provider 中提供 frames、duration 和 loop 标志。
4. 在 `assets/pet/manifest.txt` 映射 ID，并添加简单 P3 PPM 文件。
5. 由状态进入逻辑请求动画，不允许 Renderer 改状态。
6. 测试帧边界、资源解析、完成通知和循环行为。

动画时间轴通过 `pet_animation_catalog_t` 提供，像素通过 `pet_asset_provider_t` 提供。不要在
`pet_animation_player_t` 或 Renderer 中加入文件 IO、SDK API 或固定地址。

## 添加 Asset Provider

1. 实现 `pet_asset_provider_t::get_bitmap`，返回稳定的只读 RGB565 视图。
2. 需要临时资源时实现 `release_bitmap`；缓存资源可在 provider destroy 时统一释放。
3. provider 自己负责 Flash、PSRAM、文件系统或 SD 生命周期。
4. Animation Core、Pet Core 和 Renderer 不得包含存储平台头文件。
5. 添加无 GUI 解析测试和非法资源测试。

ESP32 compiled provider 使用静态 RGB565 缓存，不在帧循环中分配或释放内存。新增逻辑 asset
时必须同时更新 PC manifest/resource 与 `pet_compiled_assets.c`，保持两端 ID 一致。

## 添加状态

1. 在 `pet_state_t` 添加状态并更新名称和 state-to-animation 映射。
2. 在 `pet_core_handle_event()` 中定义唯一、可测试的进入和退出条件。
3. 为状态迁移和非法输入增加测试。
4. Renderer 只消费快照；若需要新视觉资源，按动画流程添加。
5. 更新 `docs/features.md` 和状态机说明。

## 添加事件

1. 在 `pet_event_type_t` 添加事件类型和最小 payload。
2. 事件生产者只调用 `pet_app_post_event()`，不能直接操作 Core。
3. 在 Core 定义处理语义，未知/不适用事件应安全处理。
4. 测试事件在关键状态下的行为及队列满策略。

## 添加 Display backend

1. 实现完整 `pet_display_ops_t`，context 保存设备实例，禁止 backend 全局单例。
2. 在 factory 中检查集中配置，不从业务模块读取 GPIO 宏。
3. 新总线或控制器应保持 transport 与 panel 参数边界清晰；不要修改 Renderer 适配字节序。
4. 用纯色、像素、bitmap、裁剪 region、rotation 和 flush 测试验证契约。
5. 回填 `docs/hardware.md` 的实测值，移除已确认项的 `HW_VERIFY`。

Brightness 可由 Display backend 转发；fade、sleep、wake 策略放在独立 Backlight HAL。

ESP32 board 参数集中在 `platform/esp32/main/pet_esp32_board_config.c`。RGB565 wire byte swap、
offset 和 ST7789 window 属于 ESP32 Display backend，不能放入 Renderer。修改已验证参数时必须
同步 `docs/hardware.md` 并重新执行真机纯色、坐标和 Renderer 验证。

## 添加 Simulator 输入

1. 在 `simulator_input.c` 将 SDL key 转成现有 `pet_event_t`。
2. 若现有事件语义不匹配，先添加平台无关领域事件及 Core 测试。
3. 输入适配器只返回事件或 quit 请求，禁止调用 `pet_set_state` 一类直接状态接口。
4. 在 `test_simulator_input` 用合成 SDL event 验证映射，不创建 GUI 窗口。
5. 更新 README 键盘表和帮助文本。

## 添加 Rotary 输入

1. 确认模块只有 `GND`、`VCC`、`BB`、`GA` 时，不得假设存在 `SW`、`KEY` 或 `PRESS` GPIO。
2. GA/BB GPIO、VCC、电气特性、idle level、pull、每档 transition 数和方向表未实测前保持
   `HW_VERIFY`，不自动选择未知 GPIO。
3. ESP32 backend 只把 decoded direction 转成 `PET_EVENT_NAV_NEXT` / `PET_EVENT_NAV_PREV`，不得
   调用 Core 状态接口或 Renderer。
4. quadrature decoder 使用 previous AB + current AB 的 16-entry lookup table 和 signed accumulator；
   不用固定 50 ms 全局 debounce 过滤旋转通道。
5. ISR 只做最小采样或唤醒任务；禁止 malloc、printf、Renderer、Pet Core 调用和状态切换。
6. PC simulator 使用 `LEFT` / `RIGHT` 产生同样的 NAV 事件，不维护另一套 Core 规则。

## 添加 Service

1. 在 `services/<name>/` 建立接口与实现。
2. Service 通过事件向 App 报告状态或数据，不包含 Pet Core 内部头文件。
3. 网络类 service 不得阻塞主事件循环；未来采用 SDK task/callback 时保留事件边界。
4. 为协议解析和错误路径添加 PC 测试。
5. 更新功能、架构和配置文档。

## Wi-Fi 凭据与安全

1. 真实 SSID/password 只能放在 `platform/esp32/main/wifi_config.local.h`，该文件已被
   `.gitignore` 忽略；仓库只保留 `wifi_config.example.h` 模板。
2. 提交前用 `git status` / `git diff` 确认没有真实凭据或 token 被跟踪。
3. 日志不得打印 password 或真实 SSID。
4. 时区使用 `PET_TIMEZONE` POSIX TZ 字符串（`CST-8` 表示 UTC+8，POSIX offset
   符号方向与常见 `UTC+8` 写法相反），只在 platform 初始化阶段设置，Renderer 不得感知时区。
