# Vulkan 到 PBR 渲染完整流程拆解（代码定位增强版）

> 目标：让你从 `Instance` 到 `PBR 出图` 再到 `销毁`，每一步都能在源码里**精确找到对应代码**。  
> 主线样例：`examples/pbrtexture/pbrtexture.cpp`

---

## 0. 如何使用这份文档

1. 打开 IDE 的“转到行”（Go to Line）。
2. 按本文 `步骤编号` 查对应 `文件:行号`。
3. 每一步只回答 3 个问题：
   - 输入资源是什么？
   - 这一步创建/更新了什么 Vulkan 对象？
   - 输出给下一步的是什么？

---

## 1. 一张总流程图（先看全局）

```text
【A. 程序入口层】
VULKAN_EXAMPLE_MAIN
  -> new VulkanExample
  -> initVulkan()                         // 建立 Vulkan 运行基础
       -> createInstance()                // 实例 + 扩展 + layer
       -> 枚举并选择 PhysicalDevice       // 选 GPU
       -> createLogicalDevice()           // 创建设备句柄
       -> 获取 graphics queue             // 后续提交命令用
       -> 选择 depthFormat                // 深度/模板格式
       -> swapChain.setContext(...)       // 把 instance/device 上下文交给 swapchain

【B. 窗口与基础渲染层】
  -> setupWindow()                        // 创建 OS 窗口
  -> prepare()                            // 建“可渲染现场”
       -> VulkanExampleBase::prepare()
            -> createSurface()            // 窗口与 Vulkan 连接
            -> createCommandPool()
            -> createSwapChain()
            -> createCommandBuffers()
            -> createSynchronizationPrimitives()
            -> setupDepthStencil()
            -> setupRenderPass()          // 或 dynamic rendering 路径
            -> createPipelineCache()
            -> setupFrameBuffer()
       -> pbrtexture::prepare()           // PBR 专属资源
            -> loadAssets
            -> generateBRDFLUT
            -> generateIrradianceCube
            -> generatePrefilteredCube
            -> prepareUniformBuffers
            -> setupDescriptors
            -> preparePipelines
       -> prepared = true

【C. 每帧执行层】
  -> renderLoop()
       -> render() 每帧
            -> prepareFrame()             // 等 fence + acquire image
            -> updateUniformBuffers()     // 相机/灯光/材质参数更新
            -> buildCommandBuffer()       // 录制本帧绘制命令
            -> submitFrame()              // queue submit + present

【D. 退出清理层】
  -> 析构
       -> pbrtexture 析构                 // 先释放业务资源
       -> VulkanExampleBase 析构          // 再释放通用 Vulkan 资源
```

### 1.1 每部分是做什么的

- A 程序入口层：把“能调用 Vulkan 的最小运行环境”搭起来（实例、设备、队列、深度格式）。
- B 窗口与基础渲染层：把“能真正画到屏幕”的基础设施搭好（surface/swapchain/sync/depth/renderpass/framebuffer）。
- PBR 专属层：在基础设施之上准备 PBR 必需资源（IBL 贴图、描述符、管线）。
- C 每帧执行层：重复执行“拿图 -> 更新参数 -> 录命令 -> 提交显示”。
- D 退出清理层：按依赖反序释放资源，避免泄漏和悬挂句柄。

### 1.2 各部分关系（重点）

- 关系 1：`initVulkan` 是 `prepare` 的前置条件。没有 device/queue，就不能创建 swapchain 和命令池。
- 关系 2：`VulkanExampleBase::prepare` 是“地基”，`pbrtexture::prepare` 是“装修”。必须先地基后装修。
- 关系 3：每帧 `render()` 必须按 `prepareFrame -> update -> build -> submit` 顺序，顺序错了会同步冲突或画面错误。
- 关系 4：`submitFrame` 依赖前面创建的 semaphores/fences；`buildCommandBuffer` 依赖 renderpass/framebuffer/pipeline。
- 关系 5：窗口大小变化会触发 `windowResize` 重建 swapchain 及相关资源，然后再回到每帧循环。

### 1.3 每一步详细解释（做什么 / 依赖什么 / 产出给谁）

1. `VULKAN_EXAMPLE_MAIN`
- 做什么：程序总入口，组织启动顺序。
- 依赖：平台宏（Win32/Android/XCB...）。
- 产出给谁：按固定顺序调用 `new -> initVulkan -> setupWindow -> prepare -> renderLoop`。

2. `new VulkanExample`
- 做什么：创建示例对象，初始化成员变量和默认设置。
- 依赖：类构造函数、默认相机/参数。
- 产出给谁：后续所有 Vulkan 初始化函数都基于这个对象执行。

3. `initVulkan`
- 做什么：建立“可用 GPU 上下文”。
- 依赖：命令行参数、平台扩展可用性。
- 产出给谁：`instance/physicalDevice/device/queue/depthFormat`，并把上下文交给 swapchain。

4. `createInstance`
- 做什么：创建 Vulkan 实例，启用实例扩展与可选验证层。
- 依赖：平台 surface 扩展、debug utils、validation 设置。
- 产出给谁：`VkInstance`，供设备枚举和 surface 创建使用。

5. 枚举并选择 `PhysicalDevice`
- 做什么：找到机器上的 Vulkan GPU 并选定一张。
- 依赖：实例已创建。
- 产出给谁：`physicalDevice`，给逻辑设备创建、格式查询使用。

6. `createLogicalDevice`
- 做什么：按需求启用 feature/extension，创建 `VkDevice`。
- 依赖：物理设备能力读取结果。
- 产出给谁：后续所有资源创建都依赖 `device`。

7. 获取 `graphics queue`
- 做什么：拿到提交绘制命令的队列。
- 依赖：逻辑设备已创建、队列家族索引已确定。
- 产出给谁：`prepareFrame/submitFrame` 提交命令和 present。

8. 选择 `depthFormat`
- 做什么：挑 GPU 真正支持的深度或深度模板格式。
- 依赖：物理设备格式支持查询。
- 产出给谁：`setupDepthStencil` 和 `setupRenderPass`。

9. `swapChain.setContext`
- 做什么：把 `instance/physicalDevice/device` 保存到 swapchain 模块。
- 依赖：前三者都已准备好。
- 产出给谁：`createSurface/createSwapChain/acquireNextImage/present`。

10. `setupWindow`
- 做什么：创建系统窗口句柄。
- 依赖：平台窗口系统。
- 产出给谁：`createSurface` 用窗口句柄创建 `VkSurfaceKHR`。

11. `prepare`（总入口）
- 做什么：分两层准备资源：Base 层 + PBR 层。
- 依赖：`initVulkan` 和 `setupWindow` 已完成。
- 产出给谁：`renderLoop` 每帧可直接执行。

12. `VulkanExampleBase::prepare`
- 做什么：搭通用渲染地基。
- 依赖：窗口、device、swapchain 上下文。
- 产出给谁：surface/swapchain/cmd/sync/depth/renderpass/framebuffer。

13. `createSurface`
- 做什么：把窗口接到 Vulkan，创建 `VkSurfaceKHR`。
- 依赖：窗口句柄和实例。
- 产出给谁：`createSwapChain` 查询 present 支持与 surface 能力。

14. `createCommandPool`
- 做什么：创建命令池，供命令缓冲分配。
- 依赖：队列家族索引。
- 产出给谁：`createCommandBuffers`。

15. `createSwapChain`
- 做什么：创建交换链、拿 swapchain images、创建 image views。
- 依赖：surface + device + physicalDevice。
- 产出给谁：framebuffer、present、同步对象尺寸。

16. `createCommandBuffers`
- 做什么：分配每帧主命令缓冲。
- 依赖：命令池已就绪。
- 产出给谁：`buildCommandBuffer` 每帧录制命令。

17. `createSynchronizationPrimitives`
- 做什么：创建 fence/semaphore，协调 acquire/submit/present。
- 依赖：swapchain 图像数量、device。
- 产出给谁：`prepareFrame` 和 `submitFrame`。

18. `setupDepthStencil`
- 做什么：创建深度图像、分配内存、创建深度视图。
- 依赖：`depthFormat`、窗口尺寸。
- 产出给谁：renderpass/framebuffer 深度附件。

19. `setupRenderPass`
- 做什么：定义附件、子通道、依赖与布局规则（或 dynamic rendering 跳过）。
- 依赖：颜色格式 + 深度格式。
- 产出给谁：`setupFrameBuffer`、绘制命令 begin render pass。

20. `createPipelineCache`
- 做什么：创建管线缓存。
- 依赖：device。
- 产出给谁：后续 pipeline 创建可复用缓存。

21. `setupFrameBuffer`
- 做什么：给每张 swapchain image 绑定颜色+深度附件，生成 framebuffer。
- 依赖：swapchain image views + depth view + renderpass。
- 产出给谁：每帧 `vkCmdBeginRenderPass` 选择当前 framebuffer。

22. `pbrtexture::prepare`
- 做什么：创建 PBR 业务资源。
- 依赖：Base 地基已经完成。
- 产出给谁：每帧绘制 PBR 所需纹理、描述符、管线、UBO。

23. `loadAssets`
- 做什么：加载模型与贴图（模型、天空盒、材质图）。
- 依赖：资源路径、纹理加载器、device。
- 产出给谁：描述符绑定和绘制调用。

24. `generateBRDFLUT / generateIrradianceCube / generatePrefilteredCube`
- 做什么：离线/预计算 IBL 资源。
- 依赖：临时离屏渲染流程。
- 产出给谁：PBR 光照公式查表与环境反射采样。

25. `prepareUniformBuffers`
- 做什么：创建并映射 UBO。
- 依赖：device 与内存分配。
- 产出给谁：每帧 update 写入相机、灯光、参数。

26. `setupDescriptors`
- 做什么：声明资源绑定布局并写入 descriptor sets。
- 依赖：UBO + 纹理 + sampler 已就绪。
- 产出给谁：命令录制时 `vkCmdBindDescriptorSets`。

27. `preparePipelines`
- 做什么：创建 skybox/pbr 图形管线。
- 依赖：renderpass（或 dynamic rendering 配置）、pipeline layout、shader。
- 产出给谁：`buildCommandBuffer` 绑定后发起 draw。

28. `prepared = true`
- 做什么：标记初始化结束。
- 依赖：前述资源全部成功创建。
- 产出给谁：允许进入稳定每帧渲染。

29. `renderLoop`
- 做什么：主循环，持续驱动每帧渲染。
- 依赖：`prepared` 为 true。
- 产出给谁：持续更新画面。

30. `render` 每帧 4 步
- `prepareFrame`：等待 fence，获取可渲染图像索引。
- `updateUniformBuffers`：更新相机矩阵、灯光和材质参数。
- `buildCommandBuffer`：录制本帧 render pass、bind pipeline、bind descriptor、draw。
- `submitFrame`：提交 GPU 执行并 present 到屏幕。

31. 析构（先派生后基类）
- 做什么：先释放 PBR 专属资源，再释放通用 Vulkan 资源。
- 依赖：GPU 空闲或安全同步点。
- 产出给谁：程序干净退出，无资源泄漏。

### 1.4 为什么这个顺序不能随便换

- `initVulkan` 先于 `prepare`：没有 device 就无法创建后续任何 GPU 资源。
- `createSurface` 先于 `createSwapChain`：没有 surface 就没法查询 present 支持。
- `createCommandPool` 先于 `createCommandBuffers`：没有池子就无法分配命令缓冲。
- `createSwapChain` 先于 `setupFrameBuffer`：framebuffer 需要 swapchain image views。
- `setupDepthStencil` 与 `setupRenderPass` 早于绘制：否则 render pass 缺附件定义。
- `prepareFrame` 先于 `build/submit`：必须先拿到当前可写入的交换链图像。

---## 2. 全流程代码定位表（核心）

> **颜色图例（高对比）**
> - 🟥 [A] S01-S07 启动主流程
> - 🟧 [B] S08-S15 实例创建
> - 🟨 [C] S16-S22 设备初始化
> - 🟩 [D] S23-S41 Base Prepare 与 Swapchain
> - 🟦 [E] S42-S67 PBR 资源准备
> - 🟪 [F] S68-S80 每帧渲染提交
> - 🟫 [G] S81-S84 Resize 重建
> - ⬛ [H] S85-S86 UI 调参
> - ⬜ [I] S87-S90 资源销毁
>
| 步骤 | 作用 | 代码定位（文件:行） | 你要重点看什么 |
|---|---|---|---|
| 🟥 [A] S01 | 平台入口（Win32） | `base/Entrypoints.h:25` | `WinMain` 的启动顺序 |
| 🟥 [A] S02 | 创建样例对象 | `base/Entrypoints.h:28` | `new VulkanExample()` |
| 🟥 [A] S03 | 初始化 Vulkan | `base/Entrypoints.h:29` | 进入 `initVulkan()` |
| 🟥 [A] S04 | 创建窗口 | `base/Entrypoints.h:30` | `setupWindow(...)` |
| 🟥 [A] S05 | 初始化渲染资源 | `base/Entrypoints.h:31` | `prepare()` |
| 🟥 [A] S06 | 进入主循环 | `base/Entrypoints.h:32` | `renderLoop()` |
| 🟥 [A] S07 | 退出释放 | `base/Entrypoints.h:33` | `delete(vulkanExample)` |
| 🟧 [B] S08 | 命令行参数注册 | `base/vulkanexamplebase.cpp:766-783` | `-s/-v/-g/-gl/-rp` 等 |
| 🟧 [B] S09 | 命令行参数解析 | `base/vulkanexamplebase.cpp:785` | `commandLineParser.parse(args)` |
| 🟧 [B] S10 | createInstance 开始 | `base/vulkanexamplebase.cpp:26` | 实例扩展与 layer 组装 |
| 🟧 [B] S11 | 平台 surface 扩展追加 | `base/vulkanexamplebase.cpp:28-52` | Win32/Android/XCB/Wayland 等 |
| 🟧 [B] S12 | 查询实例扩展 | `base/vulkanexamplebase.cpp:57,61` | `vkEnumerateInstanceExtensionProperties` |
| 🟧 [B] S13 | debug utils 扩展 | `base/vulkanexamplebase.cpp:131-132` | `VK_EXT_DEBUG_UTILS_EXTENSION_NAME` |
| 🟧 [B] S14 | validation layer 名称 | `base/vulkanexamplebase.cpp:142` | `VK_LAYER_KHRONOS_validation` |
| 🟧 [B] S15 | 真正创建实例 | `base/vulkanexamplebase.cpp:175` | `vkCreateInstance` |
| 🟨 [C] S16 | initVulkan 主函数 | `base/vulkanexamplebase.cpp:997` | 设备初始化总入口 |
| 🟨 [C] S17 | 枚举物理设备 | `base/vulkanexamplebase.cpp:1029,1036` | `vkEnumeratePhysicalDevices` |
| 🟨 [C] S18 | 读取设备能力 | `base/vulkanexamplebase.cpp:1073-1075` | Properties/Features/Memory |
| 🟨 [C] S19 | 创建逻辑设备 | `base/vulkanexamplebase.cpp:1088` | `createLogicalDevice(...)` |
| 🟨 [C] S20 | 取得 graphics queue | `base/vulkanexamplebase.cpp:1096` | `vkGetDeviceQueue` |
| 🟨 [C] S21 | 选深度格式 | `base/vulkanexamplebase.cpp:1104` | `getSupportedDepthFormat` |
| 🟨 [C] S22 | 绑定 swapchain 上下文 | `base/vulkanexamplebase.cpp:1108` | `swapChain.setContext(...)` |
| 🟩 [D] S23 | Base prepare 开始 | `base/vulkanexamplebase.cpp:221` | 基类初始化流水线 |
| 🟩 [D] S24 | prepare 调用顺序 | `base/vulkanexamplebase.cpp:228-236` | surface/pool/swapchain/sync/depth/rp/fb |
| 🟩 [D] S25 | 创建 surface | `base/vulkanexamplebase.cpp:3278` | 转入 `swapChain.initSurface` |
| 🟩 [D] S26 | initSurface 主体 | `base/VulkanSwapChain.cpp:15` | 平台 `vkCreate*Surface*` |
| 🟩 [D] S27 | 创建 Win32 surface | `base/VulkanSwapChain.cpp:42` | `vkCreateWin32SurfaceKHR` |
| 🟩 [D] S28 | 查询 queue 家族 | `base/VulkanSwapChain.cpp:113,116` | `vkGetPhysicalDeviceQueueFamilyProperties` |
| 🟩 [D] S29 | 查询 present 支持 | `base/VulkanSwapChain.cpp:124` | `vkGetPhysicalDeviceSurfaceSupportKHR` |
| 🟩 [D] S30 | 选 surface format | `base/VulkanSwapChain.cpp:166,169,186` | `colorFormat/colorSpace` |
| 🟩 [D] S31 | 创建 swapchain | `base/vulkanexamplebase.cpp:3301` + `base/VulkanSwapChain.cpp:197` | create 总入口 |
| 🟩 [D] S32 | surface capabilities | `base/VulkanSwapChain.cpp:208` | extent/image count 边界 |
| 🟩 [D] S33 | present mode 选择 | `base/VulkanSwapChain.cpp:226,230,234,242,244` | FIFO/MAILBOX/IMMEDIATE |
| 🟩 [D] S34 | 创建 VkSwapchainKHR | `base/VulkanSwapChain.cpp:312` | `vkCreateSwapchainKHR` |
| 🟩 [D] S35 | 拿 swapchain images | `base/VulkanSwapChain.cpp:322,324` | `vkGetSwapchainImagesKHR` |
| 🟩 [D] S36 | 创建 image views | `base/VulkanSwapChain.cpp:344` | 每张交换链图像一个 view |
| 🟩 [D] S37 | 同步原语 | `base/vulkanexamplebase.cpp:2948` | fence + semaphores |
| 🟩 [D] S38 | 命令池 | `base/vulkanexamplebase.cpp:2968` | `vkCreateCommandPool` |
| 🟩 [D] S39 | 深度资源 | `base/vulkanexamplebase.cpp:2978` | depth image/memory/view |
| 🟩 [D] S40 | 默认 renderpass | `base/vulkanexamplebase.cpp:3047` | color/depth attachment + dependency |
| 🟩 [D] S41 | framebuffer | `base/vulkanexamplebase.cpp:3023` | 每个 swapchain image 对应一个 framebuffer |
| 🟦 [E] S42 | pbrtexture prepare 总流程 | `examples/pbrtexture/pbrtexture.cpp:1231-1240` | PBR 专属步骤顺序 |
| 🟦 [E] S43 | 加载模型与贴图 | `examples/pbrtexture/pbrtexture.cpp:113` | loadAssets 入口 |
| 🟦 [E] S44 | skybox/model 加载 | `examples/pbrtexture/pbrtexture.cpp:116-117` | `cube.gltf` / `cerberus.gltf` |
| 🟦 [E] S45 | 环境和材质贴图加载 | `examples/pbrtexture/pbrtexture.cpp:118-123` | environment/albedo/normal/ao/metallic/roughness |
| 🟦 [E] S46 | 描述符布局绑定声明 | `examples/pbrtexture/pbrtexture.cpp:138-147` | binding 0~9 对应资源槽 |
| 🟦 [E] S47 | scene descriptor 写入 | `examples/pbrtexture/pbrtexture.cpp:159-168` | UBO + IBL + 5 张材质图 |
| 🟦 [E] S48 | skybox descriptor 写入 | `examples/pbrtexture/pbrtexture.cpp:175-177` | skybox 用 environment cube |
| 🟦 [E] S49 | pipeline layout 创建 | `examples/pbrtexture/pbrtexture.cpp:197-198` | `vkCreatePipelineLayout` |
| 🟦 [E] S50 | skybox / pbr shader 装配 | `examples/pbrtexture/pbrtexture.cpp:215-222` | `loadShader(...)` |
| 🟦 [E] S51 | graphics pipeline 创建 | `examples/pbrtexture/pbrtexture.cpp:217,226` | `vkCreateGraphicsPipelines` |
| 🟦 [E] S52 | BRDF LUT 生成函数 | `examples/pbrtexture/pbrtexture.cpp:230` | 离屏生成 2D LUT |
| 🟦 [E] S53 | BRDF LUT 尺寸格式 | `examples/pbrtexture/pbrtexture.cpp:234-235` | `R16G16_SFLOAT`, `dim=512` |
| 🟦 [E] S54 | BRDF LUT 图像用法 | `examples/pbrtexture/pbrtexture.cpp:248` | COLOR_ATTACHMENT + SAMPLED |
| 🟦 [E] S55 | BRDF LUT 绘制提交 | `examples/pbrtexture/pbrtexture.cpp:409,414` | begin renderpass + bind pipeline + draw |
| 🟦 [E] S56 | Irradiance Cube 生成函数 | `examples/pbrtexture/pbrtexture.cpp:434` | 漫反射 IBL 预计算 |
| 🟦 [E] S57 | Irradiance 常量 | `examples/pbrtexture/pbrtexture.cpp:438-440` | `R32G32B32A32`, `dim=64`, `numMips` |
| 🟦 [E] S58 | Irradiance 图像用法 | `examples/pbrtexture/pbrtexture.cpp:454` | SAMPLED + TRANSFER_DST |
| 🟦 [E] S59 | Irradiance 转布局/拷贝 | `examples/pbrtexture/pbrtexture.cpp:715,743,769,779,788` | setImageLayout + vkCmdCopyImage |
| 🟦 [E] S60 | Prefiltered Cube 生成函数 | `examples/pbrtexture/pbrtexture.cpp:814` | 镜面 IBL 预过滤 |
| 🟦 [E] S61 | Prefiltered 常量 | `examples/pbrtexture/pbrtexture.cpp:818-820` | `R16G16B16A16`, `dim=512`, `numMips` |
| 🟦 [E] S62 | Prefiltered roughness 分层 | `examples/pbrtexture/pbrtexture.cpp:1101-1102` | 每个 mip 对应一个 roughness |
| 🟦 [E] S63 | Prefiltered 转布局/拷贝 | `examples/pbrtexture/pbrtexture.cpp:1094,1123,1149,1159,1168` | setImageLayout + vkCmdCopyImage |
| 🟦 [E] S64 | Uniform Buffer 创建 | `examples/pbrtexture/pbrtexture.cpp:1193` | scene/skybox/params 三套 UBO |
| 🟦 [E] S65 | 每帧更新矩阵 | `examples/pbrtexture/pbrtexture.cpp:1211-1215` | scene 的 projection/view/model/camPos |
| 🟦 [E] S66 | 每帧更新 skybox 矩阵 | `examples/pbrtexture/pbrtexture.cpp:1218-1219` | 去平移 view |
| 🟦 [E] S67 | 每帧更新灯光参数 | `examples/pbrtexture/pbrtexture.cpp:1223-1228` | 四个 lights + params |
| 🟪 [F] S68 | 每帧 render 入口 | `examples/pbrtexture/pbrtexture.cpp:1296` | prepareFrame -> update -> build -> submit |
| 🟪 [F] S69 | prepareFrame | `base/vulkanexamplebase.cpp:700` | 等 fence + acquire image |
| 🟪 [F] S70 | wait fence | `base/vulkanexamplebase.cpp:704` | `vkWaitForFences` |
| 🟪 [F] S71 | acquire next image | `base/vulkanexamplebase.cpp:709` + `base/VulkanSwapChain.cpp:348,352` | `vkAcquireNextImageKHR` |
| 🟪 [F] S72 | 记录主渲染命令 | `examples/pbrtexture/pbrtexture.cpp:1244` | `buildCommandBuffer` |
| 🟪 [F] S73 | begin renderpass | `examples/pbrtexture/pbrtexture.cpp:1266` | 主 pass 开始 |
| 🟪 [F] S74 | 绘制 skybox | `examples/pbrtexture/pbrtexture.cpp:1279-1281` | bind skybox set + pipeline + draw |
| 🟪 [F] S75 | 绘制主模型 | `examples/pbrtexture/pbrtexture.cpp:1285-1287` | bind scene set + pbr pipeline + draw |
| 🟪 [F] S76 | 叠加 UI | `examples/pbrtexture/pbrtexture.cpp:1289` | `drawUI` |
| 🟪 [F] S77 | submitFrame | `base/vulkanexamplebase.cpp:723` | queue submit + present |
| 🟪 [F] S78 | queue submit | `base/vulkanexamplebase.cpp:737` | `vkQueueSubmit` |
| 🟪 [F] S79 | queue present | `base/vulkanexamplebase.cpp:748` | `vkQueuePresentKHR` |
| 🟪 [F] S80 | 帧轮转 | `base/vulkanexamplebase.cpp:760` | `currentBuffer` 递增 |
| 🟫 [G] S81 | resize 总入口 | `base/vulkanexamplebase.cpp:3191` | swapchain & 依赖资源重建 |
| 🟫 [G] S82 | resize 等待空闲 | `base/vulkanexamplebase.cpp:3200,3234` | `vkDeviceWaitIdle` |
| 🟫 [G] S83 | resize 重建资源 | `base/vulkanexamplebase.cpp:3205,3211,3215,3232` | swapchain/depth/fb/sync |
| 🟫 [G] S84 | resize 更新相机比例 | `base/vulkanexamplebase.cpp:3237` | `camera.updateAspectRatio` |
| ⬛ [H] S85 | UI 参数入口 | `examples/pbrtexture/pbrtexture.cpp:1306` | OnUpdateUIOverlay |
| ⬛ [H] S86 | Exposure/Gamma UI | `examples/pbrtexture/pbrtexture.cpp:1309-1310` | 可视化调参 |
| ⬜ [I] S87 | 样例析构 | `examples/pbrtexture/pbrtexture.cpp:82` | 释放样例自己的资源 |
| ⬜ [I] S88 | 样例析构细节 | `examples/pbrtexture/pbrtexture.cpp:85-101` | pipeline/texture/UBO destroy |
| ⬜ [I] S89 | 基类析构 | `base/vulkanexamplebase.cpp:921` | 释放通用 Vulkan 资源 |
| ⬜ [I] S90 | 基类析构细节 | `base/vulkanexamplebase.cpp:924-959` | swapchain/pool/cmd/rp/fb/depth/sync/instance |

---

## 3. 按“文件”反向看（你在 IDE 里更好查）

## 3.1 `base/Entrypoints.h`

- 启动主链：`25 -> 33`
- 不同平台的主入口宏结构相同：
  - 创建对象
  - `initVulkan`
  - `setupWindow`
  - `prepare`
  - `renderLoop`
  - `delete`

## 3.2 `base/vulkanexamplebase.cpp`

### A. 实例与设备初始化

- `createInstance()`：`26`
- `initVulkan()`：`997`
- 关键 API：
  - `vkCreateInstance`：`175`
  - `vkEnumeratePhysicalDevices`：`1029/1036`
  - `createLogicalDevice`：`1088`
  - `vkGetDeviceQueue`：`1096`

### B. 基类 prepare（渲染框架搭建）

- `prepare()`：`221`
- 顺序调用（必看）：`228-236`
- 各子函数位置：
  - `createSynchronizationPrimitives`：`2948`
  - `createCommandPool`：`2968`
  - `setupDepthStencil`：`2978`
  - `setupFrameBuffer`：`3023`
  - `setupRenderPass`：`3047`
  - `createSurface`：`3278`
  - `createSwapChain`：`3301`

### C. 每帧提交链路

- `renderLoop()`：`307`
- `prepareFrame()`：`700`
- `submitFrame()`：`723`
- 关键 API：
  - `vkWaitForFences`：`704`
  - `acquireNextImage`：`709`
  - `vkQueueSubmit`：`737`
  - `vkQueuePresentKHR`：`748`

### D. resize 与销毁

- `windowResize()`：`3191`
- `VulkanExampleBase::~VulkanExampleBase()`：`921`

## 3.3 `base/VulkanSwapChain.cpp`

### A. surface 初始化

- `initSurface(...)`：`15`
- 关键点：
  - 创建平台 surface：Win32 在 `42`
  - 查询 queue family：`113/116`
  - present 支持：`124`
  - surface format：`166/169/186`

### B. swapchain 创建

- `create(...)`：`197`
- 关键点：
  - capabilities：`208`
  - present modes：`226/230`
  - mode 选择：`234/242/244`
  - `vkCreateSwapchainKHR`：`312`
  - 获取 images：`322/324`
  - 创建 image view：`344`

### C. 获取图像与清理

- `acquireNextImage`：`348`（内部 `vkAcquireNextImageKHR` 在 `352`）
- `cleanup`：`354`（销毁 view/swapchain/surface）

## 3.4 `base/VulkanDevice.cpp`

- `getQueueFamilyIndex`：`127`
- `createLogicalDevice`：`177`
- `createBuffer`：`323/382`
- `createCommandBuffer`：`490/504`
- `flushCommandBuffer`：`520/549`

你看预计算函数（BRDF LUT / IBL）时，会频繁看到 `createCommandBuffer + flushCommandBuffer`。

## 3.5 `examples/pbrtexture/pbrtexture.cpp`

### A. 生命周期

- 类定义：`14`
- 析构：`82`
- `prepare`：`1231`
- `render`：`1296`

### B. 准备阶段（prepare）

- `loadAssets`：`113`
- `generateBRDFLUT`：`230`
- `generateIrradianceCube`：`434`
- `generatePrefilteredCube`：`814`
- `prepareUniformBuffers`：`1193`
- `setupDescriptors`：`126`
- `preparePipelines`：`183`

### C. 帧阶段（render）

- `updateUniformBuffers`：`1208`
- `buildCommandBuffer`：`1244`

### D. UI

- `OnUpdateUIOverlay`：`1306`

---

## 4. “描述符绑定”和“Shader 资源槽”的一一对应

## 4.1 C++ 侧绑定声明（Descriptor Set Layout）

`examples/pbrtexture/pbrtexture.cpp:138-147`

- binding 0: scene UBO
- binding 1: params UBO
- binding 2: irradiance cube
- binding 3: BRDF LUT
- binding 4: prefiltered cube
- binding 5: albedo
- binding 6: normal
- binding 7: ao
- binding 8: metallic
- binding 9: roughness

## 4.2 C++ 侧写 descriptor

`examples/pbrtexture/pbrtexture.cpp:159-168`

这里把每个 binding 的“槽位”连到具体资源（`textures.xxx.descriptor` / `uniformBuffers.xxx.descriptor`）。

## 4.3 GLSL 侧对应

`shaders/glsl/pbrtexture/pbrtexture.frag`

- `binding=0`：`8`
- `binding=1`：`15`
- `binding=2..9`：`21-29`

这就是你要建立的关键认知：

- C++ `binding N` 写入什么资源
- Shader `layout(binding=N)` 读到的就是这个资源

---

## 5. PBR 计算在 shader 里的具体位置

文件：`shaders/glsl/pbrtexture/pbrtexture.frag`

## 5.1 BRDF 基础项

- GGX 分布函数 `D_GGX`：`50`
- 几何项 `G_SchlicksmithGGX`：`59`
- Fresnel（粗糙度修正）`F_SchlickR`：`73`

## 5.2 IBL 相关

- 预过滤反射采样 `prefilteredReflection`：`78`
- 直接光贡献 `specularContribution`：`89`
- 法线贴图变换 `calculateNormal`：`117`

## 5.3 最终输出

- 主函数 `main`：`128`
- Tone mapping：`166`
- Gamma correction：`169`
- 输出颜色 `outColor`：`172`

---

## 6. 每一帧里 CPU/GPU 做了什么（对应代码）

```text
CPU:
  render()                                    [pbrtexture.cpp:1296]
    -> prepareFrame()                         [vulkanexamplebase.cpp:700]
         wait fence                           [704]
         acquire image                        [709]
    -> updateUniformBuffers()                 [pbrtexture.cpp:1208]
    -> buildCommandBuffer()                   [1244]
         begin renderpass                     [1266]
         draw skybox                          [1279-1281]
         draw object                          [1285-1287]
         draw UI                              [1289]
    -> submitFrame()                          [vulkanexamplebase.cpp:723]
         vkQueueSubmit                        [737]
         vkQueuePresentKHR                    [748]

GPU:
  执行 command buffer
  写 color attachment
  交换链 present
```

---

## 7. resize 与异常路径（一定要看）

- resize 总入口：`base/vulkanexamplebase.cpp:3191`
- 先 `vkDeviceWaitIdle`：`3200`
- 重建：swapchain/depth/fb/sync：`3205/3211/3215/3232`
- 更新相机比例：`3237`

`prepareFrame/submitFrame` 两处都处理了 `OUT_OF_DATE/SUBOPTIMAL`：

- `prepareFrame`：`700-714`
- `submitFrame`：`723-751`

---

## 8. 输入与交互（你调试时常用）

- 键盘：
  - Pause：`KEY_P` 在 `base/vulkanexamplebase.cpp:1281`
  - UI 显示切换：`KEY_F1` 在 `1284`
  - 相机模式：`KEY_F2` 在 `1287`
  - 移动：`W/A/S/D` 在 `1303/1306/1309/1312`
- 鼠标相机：`handleMouseMove` 在 `3246`，旋转/平移逻辑在 `3265/3268/3271`
- pbrtexture UI 参数：`examples/pbrtexture/pbrtexture.cpp:1309-1310`（Exposure/Gamma）

---

## 9. 销毁顺序（排查泄漏必看）

## 9.1 样例层析构（pbrtexture）

- 入口：`examples/pbrtexture/pbrtexture.cpp:82`
- 释放：pipeline/layout/descriptor + 贴图 + UBO：`85-101`

## 9.2 基类析构（VulkanExampleBase）

- 入口：`base/vulkanexamplebase.cpp:921`
- 通用释放顺序：`924-959`
  - swapchain cleanup
  - descriptorPool
  - command buffers
  - renderPass/framebuffers
  - shader modules
  - depth resources
  - pipeline cache/command pool
  - fences/semaphores
  - UI resources
  - vulkanDevice
  - vkDestroyInstance

---

## 10. 推荐“逐行学习路径”（非常具体）

### 第 1 轮（只看主干，不抠细节）

1. `base/Entrypoints.h:25-33`
2. `base/vulkanexamplebase.cpp:997-1113`
3. `base/vulkanexamplebase.cpp:221-236`
4. `examples/pbrtexture/pbrtexture.cpp:1231-1240`
5. `examples/pbrtexture/pbrtexture.cpp:1296-1303`

### 第 2 轮（看资源如何接线）

1. `examples/pbrtexture/pbrtexture.cpp:138-147`
2. `examples/pbrtexture/pbrtexture.cpp:159-168`
3. `shaders/glsl/pbrtexture/pbrtexture.frag:21-29`

### 第 3 轮（看 PBR 预计算）

1. `examples/pbrtexture/pbrtexture.cpp:230-421`（BRDF LUT）
2. `examples/pbrtexture/pbrtexture.cpp:434-805`（Irradiance）
3. `examples/pbrtexture/pbrtexture.cpp:814-1185`（Prefiltered）

### 第 4 轮（看每帧调度）

1. `base/vulkanexamplebase.cpp:700-760`
2. `examples/pbrtexture/pbrtexture.cpp:1208-1303`

---

## 11. 你现在就可以做的“代码定位练习”

1. 找到 `vkCreateInstance`（`base/vulkanexamplebase.cpp:175`）。
2. 找到 `vkCreateDevice` 所在函数（`base/VulkanDevice.cpp:177`）。
3. 找到 `vkCreateSwapchainKHR`（`base/VulkanSwapChain.cpp:312`）。
4. 找到 `vkQueueSubmit`（`base/vulkanexamplebase.cpp:737`）。
5. 找到 `outColor` 最终输出（`shaders/glsl/pbrtexture/pbrtexture.frag:172`）。

如果你这 5 个点都能在 IDE 中准确跳到，就已经掌握了“从初始化到出图”的完整骨架。


