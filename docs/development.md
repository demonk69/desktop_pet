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
```

测试不得依赖真实 LCD。新增平台 backend 时，核心测试仍应可在普通 PC 上运行。

## 添加模块

1. 定义单一职责和 owner 状态结构。
2. 在 `include/<module>/` 添加最小公共 API，内部函数不导出。
3. 只依赖 `docs/architecture.md` 允许的下层模块。
4. 添加无硬件测试，并在 CMake 中声明目标。
5. 更新功能和架构文档。

## 添加动画

1. 在 `pet_animation_id_t` 添加 ID。
2. 在资源 ID 表或未来资源清单中添加逻辑 asset ID。
3. 在 catalog provider 中提供 frames、duration 和 loop 标志。
4. 由状态进入逻辑请求动画，不允许 Renderer 改状态。
5. 测试帧边界、完成通知和循环行为。

未来若资源来自 Flash、PSRAM 或文件系统，只替换/新增 `pet_animation_catalog_t` provider；
不要在 `pet_animation_player_t` 中加入文件 IO 或固定地址。

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
3. 对 ST7789，将命令层与 SPI/8080 transport 分开。
4. 用纯色、像素、bitmap、裁剪 region、rotation 和 flush 测试验证契约。
5. 回填 `docs/hardware.md` 的实测值，移除已确认项的 `HW_VERIFY`。

Brightness 可由 Display backend 转发；fade、sleep、wake 策略放在独立 Backlight HAL。

## 添加 Service

1. 在 `services/<name>/` 建立接口与实现。
2. Service 通过事件向 App 报告状态或数据，不包含 Pet Core 内部头文件。
3. 网络类 service 不得阻塞主事件循环；未来采用 SDK task/callback 时保留事件边界。
4. 为协议解析和错误路径添加 PC 测试。
5. 更新功能、架构和配置文档。
