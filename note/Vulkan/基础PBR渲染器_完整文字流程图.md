# 基础 PBR 渲染器完整文字流程图（基于当前 Vulkan 样例）

下面这份文档对应“可落地的基础 PBR 渲染器”完整文字流程图，组织方式参考当前项目思路：`pbrtexture + gltfscenerendering + shadowmapping + hdr`。

重点覆盖：
1. 阶段关系
2. 资源依赖
3. Vulkan 绑定关系（descriptor/pipeline/renderpass/command buffer）

---

## 1. 全局生命周期图（启动到每帧）

```text
[App 启动]
   |
   v
[创建 Vulkan 基础对象]
Instance/Device/Swapchain/RenderPass/Framebuffers/CommandPool/SyncObjects
   |
   v
[加载静态资源]
Mesh(glTF) + MaterialTexture + EnvironmentCube(HDR cubemap)
   |
   v
[创建渲染资源]
UBO/SSBO + DescriptorSetLayout + DescriptorPool + DescriptorSets + Pipelines
   |
   v
[IBL 预计算]
BRDF LUT ---> Irradiance Cube ---> Prefiltered Cube
   |              |                    |
   |              +---------+----------+
   |                        v
   +------------------> [IBL 资源就绪]
   |
   v
[进入帧循环]
   |
   +--> [更新每帧数据] Camera/Light/ObjectTransform
   |
   +--> [Shadow Pass] 生成 ShadowMap
   |
   +--> [Main PBR Pass] 渲染 Skybox + Opaque PBR 到 HDR Color
   |
   +--> [ToneMapping Pass] HDR -> Swapchain
   |
   +--> [Present]
```

---

## 2. 资源依赖图（DAG）

```text
(输入资产)
  ├─ glTF Mesh/Node
  ├─ Material Textures: albedo/normal/ao/metallic/roughness
  └─ Environment Cube (HDR)

(预计算资源)
  Environment Cube ---------> Irradiance Cube
           |                       |
           +---------------------> Prefiltered Cube
  (解析积分) ---------------------> BRDF LUT (2D)

(运行时输入)
  Camera UBO + Light UBO + Object Transform Buffer
  + Material Textures
  + Irradiance Cube + Prefiltered Cube + BRDF LUT
  + Shadow Map

(运行时输出)
  Shadow Pass ----------> ShadowMap
  Main PBR Pass --------> HDR Color + HDR Depth
  ToneMapping Pass -----> Swapchain Image
```

关键依赖关系一句话版：
1. `Irradiance/Prefilter` 依赖 `Environment Cube`。
2. `Main PBR Pass` 依赖 `IBL 三件套 + 材质贴图 + ShadowMap + 每帧 UBO`。
3. `ToneMapping` 依赖 `Main Pass` 输出的 HDR 颜色。

---

## 3. 阶段内详细流程图（初始化阶段）

```text
[Init-1 设备与交换链]
Create Instance -> PhysicalDevice -> LogicalDevice -> Swapchain
Create Depth Image / RenderPass / Framebuffer / CommandPool / Fence / Semaphore

[Init-2 场景与纹理]
Load glTF (vertex/index/material/node hierarchy)
Load material textures (albedo/normal/ao/metallic/roughness)
Load environment cubemap (HDR)

[Init-3 IBL 预计算]
3.1 BRDF LUT Pass
  - 创建 2D image(view+sampler)
  - 离屏 renderpass 渲染全屏三角形
  - 输出 imageLayout: SHADER_READ_ONLY_OPTIMAL

3.2 Irradiance Cube Pass
  - 创建 cubemap image(6 faces)
  - 以 environment cube 为输入做卷积
  - 对 6 个 face 渲染并 copy 到目标 cubemap
  - 最终 layout: SHADER_READ_ONLY_OPTIMAL

3.3 Prefiltered Cube Pass
  - 创建带 mip 的 cubemap image
  - 对每个 mip + 每个 face 渲染（roughness 对应 mip）
  - 最终 layout: SHADER_READ_ONLY_OPTIMAL

[Init-4 Descriptor 与 Pipeline]
Create DescriptorSetLayouts (Frame/Material/Object)
Create DescriptorPool + Allocate DescriptorSets + WriteDescriptorSets
Create PipelineLayout(绑定 setLayout + push constants)
Create Pipelines(Shadow / Skybox / PBR / ToneMapping)
```

---

## 4. 每帧执行图（命令提交与同步）

```text
[Frame N Begin]
AcquireNextImage(swapchainImageIndex)
WaitFence(frameFence) + ResetFence

[Update CPU -> GPU]
Map/Copy:
  - Camera matrices
  - Light params + light VP matrix
  - per-object model matrix(or dynamic offset)

[Record CmdBuffer]
  A) Shadow Pass
     BeginRenderPass(shadowRP, shadowFB)
     BindPipeline(shadow)
     BindDescriptorSets(frame + object)
     Draw scene geometry
     EndRenderPass
     Barrier: ShadowMap
       DEPTH_ATTACHMENT_OPTIMAL -> DEPTH_STENCIL_READ_ONLY_OPTIMAL

  B) Main PBR Pass (HDR offscreen)
     BeginRenderPass(mainRP, hdrFB)
     Draw skybox
       BindPipeline(skybox) + BindDescriptorSet(frame env cube)
     Draw opaque objects
       BindPipeline(pbr)
       BindDescriptorSet(frame: camera/light/IBL/shadow)
       BindDescriptorSet(material: 5 textures + factors)
       BindDescriptorSet(object: transform)
       PushConstants(可选: material scalar)
       DrawIndexed(...)
     EndRenderPass
     Barrier: HDRColor
       COLOR_ATTACHMENT_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL

  C) ToneMapping Pass
     BeginRenderPass(tonemapRP, swapchainFB)
     BindPipeline(tonemap)
     BindDescriptorSet(input HDR color)
     Draw fullscreen triangle
     EndRenderPass

[Submit]
QueueSubmit(wait: imageAvailable, signal: renderFinished, fence: frameFence)
QueuePresent(wait: renderFinished)
```

---

## 5. 绑定关系图（“谁绑定谁”）

```text
DescriptorSetLayout(set0/set1/set2) ----+
                                        |
                                        v
                                  PipelineLayout
                                        |
                     +------------------+------------------+
                     |                                     |
                Shadow Pipeline                        PBR Pipeline
                     |                                     |
             (与 shadow renderpass 兼容)           (与 main renderpass 兼容)
                     |                                     |
                     +------------------+------------------+
                                        |
                                    CmdBuffer
                                        |
     BeginRenderPass -> BindPipeline -> BindDescriptorSets -> PushConstants -> Draw
```

要点：
1. `PipelineLayout` 绑定的是 `DescriptorSetLayout`（接口定义）。
2. `DescriptorSet` 是按 layout 分配并写入“实际资源句柄”。
3. `RenderPass` 不“持有” pipeline，但 pipeline 创建时要声明兼容的 renderpass/subpass。
4. 真正执行时在 `CmdBuffer` 里同时绑定 `RenderPass + Pipeline + DescriptorSets`。

---

## 6. 推荐的 Descriptor 分层（基础渲染器可维护）

```text
Set 0: Frame级（每帧变）
  binding 0: Camera UBO (proj/view/camPos)
  binding 1: Light UBO (light pos/color/light VP/exposure/gamma)
  binding 2: Irradiance Cube
  binding 3: BRDF LUT
  binding 4: Prefiltered Cube
  binding 5: Shadow Map (depth sampler)

Set 1: Material级（每材质变）
  binding 0: albedoMap
  binding 1: normalMap
  binding 2: aoMap
  binding 3: metallicMap
  binding 4: roughnessMap
  (可选) binding 5: material param UBO

Set 2: Object级（每物体变）
  binding 0: dynamic UBO / SSBO (model matrix, object id, skin index...)
```

---

## 7. 阶段关系总结（先后不能错）

1. 先有 `EnvironmentCube`，才能做 `Irradiance/Prefilter`。
2. 先有 `IBL 三件套`，PBR 主 pass 才能正确计算环境光。
3. 先跑 `Shadow Pass`，主 pass 才能采样阴影。
4. 主 pass 输出 HDR 后，`ToneMapping` 才能写入 swapchain。
5. 每个“写后读”都要有正确的 layout transition/barrier。

---

## 8. 可对照的项目入口（本仓库）

1. `examples/pbrtexture/pbrtexture.cpp`
2. `shaders/glsl/pbrtexture/pbrtexture.frag`
3. `shaders/glsl/pbrtexture/filtercube.vert`
4. `examples/shadowmapping/shadowmapping.cpp`
5. `examples/hdr/hdr.cpp`

---

如果后续需要，可以在此文档后面继续补一版“工程实现清单图”：按 `Phase 1~6` 列出每个阶段需要新增的 C++ 文件、shader、descriptor binding 编号和验收标准。

---

## 9. 工程级细化补充（基于 `examples/pbrtexture` 实际代码）

本节补充上一版流程图里省略的细节，重点包含：
1. 真实函数调用顺序
2. 资源对象级依赖（image/view/sampler/memory/layout）
3. descriptor 与 shader binding 精确映射
4. pass 内 barrier / layout transition
5. 每帧提交与同步细节

参考文件：
- `examples/pbrtexture/pbrtexture.cpp`
- `shaders/glsl/pbrtexture/pbrtexture.vert`
- `shaders/glsl/pbrtexture/pbrtexture.frag`
- `shaders/glsl/pbrtexture/skybox.vert`
- `shaders/glsl/pbrtexture/skybox.frag`
- `shaders/glsl/pbrtexture/filtercube.vert`
- `base/vulkanexamplebase.cpp`
- `base/VulkanTools.cpp`

### 9.1 启动阶段真实时序（`prepare()`）

```text
prepare()
  ├─ VulkanExampleBase::prepare()      // 基础 Vulkan 对象
  ├─ loadAssets()                      // 模型 + 环境 cubemap + 5 张材质贴图
  ├─ generateBRDFLUT()                 // 生成 2D BRDF LUT
  ├─ generateIrradianceCube()          // 生成 irradiance cubemap
  ├─ generatePrefilteredCube()         // 生成 prefiltered cubemap
  ├─ prepareUniformBuffers()           // scene/skybox/params 三类 UBO（每帧一套）
  ├─ setupDescriptors()                // 每帧分配 scene/skybox 两个 descriptor set
  ├─ preparePipelines()                // skybox + pbr 两条 graphics pipeline
  └─ prepared = true
```

### 9.2 资源对象级依赖图

```text
[输入资产]
  environmentCube (HDR cube)
  object model (cerberus)
  albedo/normal/ao/metallic/roughness

[预计算输出]
  textures.lutBrdf (2D)
  textures.irradianceCube (cube + mips)
  textures.prefilteredCube (cube + mips)

[每帧 CPU->GPU]
  uniformBuffers[i].scene
  uniformBuffers[i].skybox
  uniformBuffers[i].params

[主渲染读取]
  skybox pipeline: skybox UBO + params UBO + environmentCube
  pbr pipeline: scene UBO + params UBO + IBL三件套 + 5张材质贴图
```

### 9.3 DescriptorSetLayout 与 shader binding 对照

`setupDescriptors()` 的 set layout 定义 `binding 0..9`：

```text
b0  UBO
b1  UBO
b2  COMBINED_IMAGE_SAMPLER
b3  COMBINED_IMAGE_SAMPLER
b4  COMBINED_IMAGE_SAMPLER
b5  COMBINED_IMAGE_SAMPLER
b6  COMBINED_IMAGE_SAMPLER
b7  COMBINED_IMAGE_SAMPLER
b8  COMBINED_IMAGE_SAMPLER
b9  COMBINED_IMAGE_SAMPLER
```

`descriptorSets[i].scene` 写入：

```text
b0 -> uniformBuffers[i].scene
b1 -> uniformBuffers[i].params
b2 -> textures.irradianceCube
b3 -> textures.lutBrdf
b4 -> textures.prefilteredCube
b5 -> textures.albedoMap
b6 -> textures.normalMap
b7 -> textures.aoMap
b8 -> textures.metallicMap
b9 -> textures.roughnessMap
```

`descriptorSets[i].skybox` 写入：

```text
b0 -> uniformBuffers[i].skybox
b1 -> uniformBuffers[i].params
b2 -> textures.environmentCube
```

shader 侧对应关系：

```text
pbrtexture.vert/frag 使用 b0..b9
skybox.vert 使用 b0
skybox.frag 使用 b1 + b2
```

### 9.4 IBL 三个生成 pass 的内部流程（细化）

#### 9.4.1 BRDF LUT (`generateBRDFLUT`)

```text
创建 lutBrdf.image:
  format: R16G16_SFLOAT
  usage: COLOR_ATTACHMENT | SAMPLED
创建 lutBrdf.view + lutBrdf.sampler

创建离屏 renderpass:
  附件 finalLayout = SHADER_READ_ONLY_OPTIMAL
  配置 subpass dependencies

创建临时 pipeline:
  shader = genbrdflut.vert + genbrdflut.frag
  无顶点输入，vkCmdDraw(3)

提交一次命令后:
  lutBrdf 可直接被主 pbr shader 采样
```

#### 9.4.2 Irradiance Cube (`generateIrradianceCube`)

```text
目标 cube:
  textures.irradianceCube.image
  usage: SAMPLED | TRANSFER_DST

中间 offscreen:
  offscreen.image
  usage: COLOR_ATTACHMENT | TRANSFER_SRC

关键 layout 流程:
  offscreen: UNDEFINED -> COLOR_ATTACHMENT_OPTIMAL
  targetCube(all mips/layers): UNDEFINED -> TRANSFER_DST_OPTIMAL

for each mip m:
  for each face f:
    1) 渲染到 offscreen (filtercube.vert + irradiancecube.frag)
    2) offscreen: COLOR_ATTACHMENT_OPTIMAL -> TRANSFER_SRC_OPTIMAL
    3) vkCmdCopyImage(offscreen -> targetCube[m,f])
    4) offscreen: TRANSFER_SRC_OPTIMAL -> COLOR_ATTACHMENT_OPTIMAL

结束:
  targetCube: TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
```

#### 9.4.3 Prefiltered Cube (`generatePrefilteredCube`)

结构和 irradiance pass 同型，差异点：

```text
shader = prefilterenvmap.frag
push constant = mvp + roughness + numSamples
roughness 与 mip 对应:
  roughness = m / (numMips - 1)
结果写入 textures.prefilteredCube[mip, face]
最终 layout = SHADER_READ_ONLY_OPTIMAL
```

### 9.5 主渲染 pass 的绑定与执行

`preparePipelines()`：

```text
pipelineLayout <- descriptorSetLayout(1个set)

pipelines.skybox:
  shader: skybox.vert/skybox.frag
  cull: FRONT

pipelines.pbr:
  shader: pbrtexture.vert/pbrtexture.frag
  cull: BACK
  depthTest/depthWrite = true

二者均兼容同一个主 renderPass
```

`buildCommandBuffer()` 每帧命令：

```text
BeginRenderPass(main renderPass, frameBuffers[currentImageIndex])
  SetViewport/Scissor
  if (displaySkybox):
    BindDescriptorSets(... descriptorSets[currentBuffer].skybox)
    BindPipeline(... pipelines.skybox)
    Draw skybox

  BindDescriptorSets(... descriptorSets[currentBuffer].scene)
  BindPipeline(... pipelines.pbr)
  Draw object
  Draw UI
EndRenderPass
```

### 9.6 每帧同步（`VulkanExampleBase`）

```text
prepareFrame():
  wait fence(waitFences[currentBuffer])
  reset fence
  acquireNextImage(wait semaphore = presentCompleteSemaphores[currentBuffer])
  -> currentImageIndex

submitFrame():
  vkQueueSubmit(
    waitSemaphore   = presentCompleteSemaphores[currentBuffer],
    waitStage       = COLOR_ATTACHMENT_OUTPUT,
    signalSemaphore = renderCompleteSemaphores[currentImageIndex],
    commandBuffer   = drawCmdBuffers[currentBuffer],
    fence           = waitFences[currentBuffer])

  vkQueuePresentKHR(
    waitSemaphore = renderCompleteSemaphores[currentImageIndex],
    imageIndex    = currentImageIndex)

  currentBuffer = (currentBuffer + 1) % maxConcurrentFrames
```

### 9.7 一个常被忽略的点

当前 `pbrtexture` 样例没有“独立 HDR 后处理 pass”。  
它是在 `skybox.frag` 和 `pbrtexture.frag` 内直接做 tonemap + gamma，然后直接写 swapchain 目标。

这意味着：
1. 它是单主 pass 输出路径（不是“离屏 HDR 再后处理”的多 pass 链）。
2. 如果要扩展 bloom/曝光直方图/TAA，建议改成 HDR 离屏再做 post-process。

### 9.8 扩展为“基础渲染器（含 shadow + HDR 后处理）”的详细关系

```text
[Frame Begin]
  Acquire swapchain image
  Update buffers
       |
       v
[Pass 1: Shadow]
  输出: shadowDepth
  layout: DEPTH_ATTACHMENT_OPTIMAL
       |
       | barrier
       v
[Pass 2: Main PBR (HDR Offscreen)]
  输入: Frame/Object/Material + IBL + ShadowMap
  输出: hdrColor + hdrDepth
  hdrColor: COLOR_ATTACHMENT_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
       |
       v
[Pass 3: ToneMapping]
  输入: hdrColor
  输出: swapchain image
       |
       v
Present
```

建议的 Descriptor 分层：

```text
Set0(Frame):
  b0 camera
  b1 lights + lightVP + exposure/gamma
  b2 irradiance
  b3 brdfLUT
  b4 prefiltered
  b5 shadowMap

Set1(Material):
  b0 albedo
  b1 normal
  b2 ao
  b3 metallic
  b4 roughness

Set2(Object):
  b0 model/object data (dynamic UBO or SSBO)
```

---

## 10. 问答补充：UBO / SSBO / Push Constants

### 10.1 这三个东西的文字流程图

```text
                    ┌──────────────── CPU 侧 ────────────────┐
Frame N 开始         │                                        │
                     │ 1) 写 UBO 数据（相机、灯光、矩阵等）     │
                     │    -> 映射内存 memcpy 到 VkBuffer       │
                     │                                        │
                     │ 2) 写 SSBO 初始数据（可选）             │
                     │    -> 大数组/实例/粒子等                │
                     │                                        │
                     │ 3) 录制命令到 CmdBuffer                 │
                     │    vkCmdBindDescriptorSets(...)        │
                     │      (里面绑定了 UBO/SSBO 的 descriptor)│
                     │    vkCmdPushConstants(...)              │
                     │      (直接塞一小块常量)                 │
                     │    vkCmdDraw / vkCmdDispatch            │
                     └────────────────┬───────────────────────┘
                                      │ 提交
                                      v
                    ┌──────────────── GPU 侧 ────────────────┐
                    │ VS/FS/CS 读取 UBO（只读、小而稳定）      │
                    │ VS/FS/CS 读取 SSBO（大数据）             │
                    │ CS/FS 可写 SSBO（读写缓冲）              │
                    │ VS/FS/CS 读取 Push Constants（很小、快） │
                    └────────────────┬───────────────────────┘
                                      │
                                      v
                    下一 pass/下一帧继续复用 Buffer 资源
                    （若 SSBO 被写后再读，需要 barrier 同步）
```

角色定位图：

```text
UBO            = 小参数仓库（通常只读）
SSBO           = 大数据仓库（可读写）
Push Constants = 指令里夹带的小纸条（超小、超频繁更新）
```

存在位置差别：

1. UBO  
   真正存储在 `VkBuffer + VkDeviceMemory`，通过 descriptor 绑定给 shader。  
2. SSBO  
   也在 `VkBuffer + VkDeviceMemory`，通过 descriptor 绑定，常见 compute 写、graphics 读。  
3. Push Constants  
   不是独立 buffer 资源，而是 command buffer 记录的一小段常量状态，通过 `vkCmdPushConstants` 写入。  

---

### 10.2 为什么 Push Constants 适合传 `mvp / roughness`

原因是这两个参数同时满足两点：

1. 数据量小  
   `mvp`（`mat4`）+ `roughness`（`float`）属于小块参数，符合 push constants 容量特性。  
2. 更新频率高  
   在 cubemap 预计算（按 face/mip 循环）里，每次 draw 都可能改 `mvp` 和 `roughness`。  
   这类“每 draw 高频小更新”用 `vkCmdPushConstants` 比频繁改 UBO/descriptor 更直接。

在 `filtercube` / `prefilter` 场景中：

1. `mvp` 用于切换当前 cubemap 面的观察方向。  
2. `roughness` 用于指定当前预过滤目标粗糙度（常与 mip 级对应）。  

一句话总结：  
Push Constants 非常适合“每次 draw 前临时改变的小参数”，`mvp/roughness` 正是这种参数。
