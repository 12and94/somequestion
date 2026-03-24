# ShadowMap 产生、变化、使用：完整文字流程图（基于当前项目）

你这个问题非常关键。  
我用项目里的 `shadowmapping` 样例给你讲“完整闭环”：

- 阴影图在哪里产生
- 资源怎么创建
- 每帧怎么变化
- 主渲染怎么使用
- Vulkan 里具体绑定关系是什么

参考代码：
- `examples/shadowmapping/shadowmapping.cpp`
- `shaders/glsl/shadowmapping/offscreen.vert`
- `shaders/glsl/shadowmapping/scene.vert`
- `shaders/glsl/shadowmapping/scene.frag`

---

## 先给结论

`shadow map` 本质是“从光源视角渲染出来的深度图”。  
主相机渲染时，每个像素都去这张深度图查“这个点在光源看来是不是被挡住”，然后决定亮还是暗。

---

## 1. 阴影图在哪里产生

在这个样例里，产生位置是 `buildCommandBuffer()` 的**第一段 render pass**（offscreen pass）：

- 使用单独的 `offscreenPass.renderPass`
- 绑定 `offscreenPass.frameBuffer`
- 这个 framebuffer 只有一个深度附件 `offscreenPass.depth`
- shader 用 `offscreen.vert`（只有顶点阶段，写深度，不输出颜色）

对应代码点：
- `prepareOffscreenRenderpass()` 创建深度-only render pass
- `prepareOffscreenFramebuffer()` 创建深度 image/view/sampler/framebuffer
- 第一 pass 生成 shadow map

---

## 2. 阴影图资源是怎么创建的

`prepareOffscreenFramebuffer()` 里核心是：

1. 创建 `VkImage` 作为深度图  
`usage = DEPTH_STENCIL_ATTACHMENT_BIT | SAMPLED_BIT`  
这句很关键：既要“写深度”，又要“后续采样”。

2. 创建 `VkImageView`（`aspect = DEPTH`）

3. 创建 `VkSampler`（后续 fragment shader 采样深度图）

4. 把这个 depth view 挂到 offscreen framebuffer 上

---

## 3. 阴影图的布局变化（image layout）

这个样例主要靠 offscreen render pass 的附件描述 + subpass dependency 完成，不需要你手写额外 barrier：

1. 初始 `UNDEFINED`
2. offscreen pass 内作为 `DEPTH_STENCIL_ATTACHMENT_OPTIMAL` 写入
3. pass 结束转成 `DEPTH_STENCIL_READ_ONLY_OPTIMAL`，供第二 pass 采样

关键定义在：
- `attachment.finalLayout = DEPTH_STENCIL_READ_ONLY_OPTIMAL`
- 两个 `VkSubpassDependency`（写深度 -> 读采样的可见性）

---

## 4. 每帧怎么“变化”

只要光源或场景动，shadow map 每帧都会重算。  
这个样例里光源是动画的：

1. `updateLight()` 更新 `lightPos`
2. `updateUniformBuffers()` 重新计算 `depthMVP = P_light * V_light * M`
3. 第一 pass 用新的 `depthMVP` 渲染深度图
4. 第二 pass 采样这个“本帧新阴影图”

---

## 5. 主渲染里怎么用 shadow map

第二 pass（正常场景渲染）：

1. vertex shader (`scene.vert`) 计算 `outShadowCoord`  
`outShadowCoord = biasMat * lightSpace * model * vec4(inPos,1)`

2. fragment shader (`scene.frag`) 采样 `shadowMap`  
把 `inShadowCoord / w` 变成投影坐标，取深度比较：

- `dist = texture(shadowMap, uv).r`
- 如果 `dist < currentDepth`，说明当前点在光源看去被挡住，判定为阴影

3. 输出颜色时乘上 `shadow factor`

---

## 6. 绑定关系（Descriptor / Pipeline）

这个样例的 set layout 很直接：

- `binding 0`：UBO（矩阵、光源参数）
- `binding 1`：shadow map sampler

分三个 descriptor set（每帧各一套）：

1. `offscreen`：只用 `binding 0`（写 shadow map 不需要采样）
2. `scene`：`binding 0 + binding 1`（主渲染采样 shadow map）
3. `debug`：`binding 0 + binding 1`（把 shadow map 可视化）

---

## 7. 完整文字流程图（生成 -> 变化 -> 使用）

```text
[初始化阶段]
  |
  |-- 创建 offscreen 深度资源
  |     depthImage (D16) + depthView + depthSampler
  |     usage: DEPTH_STENCIL_ATTACHMENT | SAMPLED
  |
  |-- 创建 offscreen renderPass
  |     attachment:
  |       initialLayout = UNDEFINED
  |       finalLayout   = DEPTH_STENCIL_READ_ONLY_OPTIMAL
  |     subpass: depth-only
  |     dependencies: (shader read <-> depth write) 同步
  |
  |-- 创建 offscreen framebuffer (只挂 depthView)
  |
  |-- 创建 descriptor/pipeline
        offscreen pipeline: offscreen.vert (depth only, 启用 depth bias)
        scene pipeline: scene.vert + scene.frag (采样 shadowMap)
        debug pipeline: quad.vert + quad.frag (显示深度图)

=====================================================

[每帧 Frame N]
  |
  |-- 更新光源位置 updateLight()
  |-- 计算 depthMVP (light projection * light view * model)
  |-- 更新 UBO:
  |     offscreen UBO.depthMVP
  |     scene UBO.lightSpace/depthBiasMVP/lightPos...
  |
  |-- Pass 1: Shadow Pass (offscreen)
  |     BeginRenderPass(offscreenPass)
  |     depth attachment clear = 1.0
  |     Bind pipeline: pipelines.offscreen
  |     Bind descriptor: descriptorSets[current].offscreen
  |     vkCmdSetDepthBias(...)   // 防止 acne
  |     Draw scene
  |     EndRenderPass
  |     结果: depthImage 从“写深度目标”转成“只读采样”
  |
  |-- Pass 2: Main Scene Pass
  |     BeginRenderPass(main swapchain pass)
  |     Bind pipeline: sceneShadow 或 sceneShadowPCF
  |     Bind descriptor: descriptorSets[current].scene
  |       b0 = scene UBO
  |       b1 = shadowMap(depthSampler + depthView)
  |     Draw scene
  |       VS: 计算 outShadowCoord
  |       FS: shadow map 深度比较 -> shadow factor
  |     EndRenderPass
  |
  |-- Submit + Present
```

---

## 8. 阴影判定的“数学变化过程”图

```text
世界坐标点 P_world
   -> 乘 lightViewProj 得到 P_lightClip
   -> 除以 w 得到 P_lightNDC
   -> 乘 biasMat 映射到 [0,1] 纹理空间
   -> 得到 shadowUV + currentDepth

shadowMap 采样:
   dist = texture(shadowMap, shadowUV).r

比较:
   if dist < currentDepth:
      在阴影中
   else:
      被光照到
```

---

## 9. 接入 PBR 流程时怎么放

如果把它接到 `pbrtexture`，结构是：

1. 先做 shadow pass 生成 depth map
2. PBR 主 pass 的 fragment 增加 `binding` 采样 shadow map
3. direct light 项乘 `shadowFactor`
4. IBL 环境光通常不乘这个 shadow（或只乘 AO，不乘 shadow map）
