# GPU渲染管线详解与高频追问记录

> 记录起点：用户提出“现在把我当成有一点了解的人，从头到尾再详细说明一遍GPU渲染管线每一步在做什么，注意每一步内容要详细”。
> 本文整理了该请求之后的完整讲解内容与延伸高频追问。

---

## 1. GPU渲染管线（从头到尾，工程视角）

## 0) CPU 侧提交（渲染前置）
在进入 GPU pipeline 前，CPU 通常要完成：
- 场景可见性粗筛（有时在 CPU，有时 GPU-driven）
- Draw/Dispatch 命令组织（命令缓冲、状态切换）
- 资源绑定：
  - 顶点/索引缓冲
  - 常量缓冲（MVP、材质参数）
  - 纹理与采样器
  - Pipeline State（shader、blend、depth、raster state）
- 提交到 GPU 队列（graphics queue）

关键点：CPU 这步决定“喂给 GPU 的工作质量”。Draw call 太碎、状态切换太多，会先卡在 CPU。

## 1) Input Assembler（输入装配）
主要职责：
- 从 Vertex Buffer 取顶点属性（position/normal/tangent/uv/color...）
- 从 Index Buffer 组装图元（通常三角形）
- 确定 primitive topology（triangle list/strip 等）

输出：顶点流 + 图元拓扑，送入 Vertex Shader。

常见问题：
- 顶点布局与 shader 输入不匹配（stride/offset 错）
- 索引越界或 winding 顺序错误导致面剔除异常

## 2) Vertex Shader（VS）
逐顶点执行，核心任务：
1. 空间变换：`local -> world -> view -> clip`（MVP）
2. 输出后续插值数据：worldPos、normal、tangent、uv、vertex color、shadow coord 等
3. 可做轻量顶点级计算（如 morph、skin 前处理）

输出：
- 必须包含 clip-space position（`SV_Position/gl_Position`）
- 其他 varying/interpolants 进入后续插值链

常见问题：
- 法线变换错误（非均匀缩放时应使用法线矩阵）
- 精度不足导致远处抖动

## 3) 可选阶段：Tessellation / Geometry Shader
### Tessellation（细分）
- Hull/Control Shader：定义细分因子
- Tessellator：硬件细分 patch
- Domain/Eval Shader：计算新顶点位置

常用于曲面 LOD、地形、位移细节。

### Geometry Shader（GS）
- 输入图元，输出 0~N 个图元
- 常见用途：法线可视化、阴影体、billboard
- 现代实时渲染中常因性能/吞吐原因较少作为主力

## 4) Clipping + Perspective Divide + Viewport Transform
流程：
1. 裁剪（Clip）：去掉视锥外图元
2. 透视除法：`x,y,z /= w` 得到 NDC
3. 视口变换：NDC 映射到屏幕坐标与深度范围

输出：屏幕空间三角形，准备光栅化。

常见问题：
- 近远平面设置不合理导致深度精度差
- 反向Z/深度范围配置不统一引发 Z-fighting

## 5) Rasterization（光栅化）
职责：
- 将三角形离散为片元（fragment）
- 计算重心坐标用于插值
- 执行面剔除（front/back culling）
- MSAA 时按 sample 生成覆盖信息

输出：片元输入给 Pixel/Fragment Shader。

常见问题：
- winding 与 cull mode 不一致导致“模型消失”
- overdraw 严重时像素阶段压力大

## 6) Pixel/Fragment Shader（PS/FS）
逐片元计算颜色（或 GBuffer）：
- 纹理采样（albedo/normal/roughness/metallic/AO...）
- 法线重建（TBN）
- 光照计算（Phong/Blinn/PBR）
- 阴影查询（shadow map/PCF/PCSS）
- 输出到 RT（forward 输出最终色；deferred 输出材质属性）

常见问题：
- 分支过多、纹理访问不友好导致 ALU/TEX 压力
- 法线空间混淆（tangent/world/view）
- gamma/linear 空间混用

## 7) Early-Z / Late-Z，Depth-Stencil Test
- Depth test：决定片元可见性（前景覆盖背景）
- Stencil test：模板遮罩与特殊效果控制

Early-Z 价值：在像素着色前剔除被挡片元，节省 PS 成本。

## 8) Output Merger（Blend + Write）
- 与目标缓冲已有颜色进行混合（透明、加色、alpha）
- 写入 color/depth/stencil buffer
- 必要时进行 MSAA resolve

常见问题：
- 透明排序错误
- 预乘 alpha 与非预乘 alpha 混用
- HDR/LDR 与 tone mapping 链路不一致

## 9) 后处理（Post Process）
常见 pass：Bloom、TAA/FXAA、SSAO/SSR、Tone Mapping、Color Grading、UI 叠加。

本质：对上一阶段结果做全屏处理，最终输出到交换链 backbuffer。

---

## 2. Forward vs Deferred 逐阶段差异

## CPU提交阶段
- Forward：每物体通常直接做材质+光照路径，光源多时 shader 压力高
- Deferred：先几何写 GBuffer，再做屏幕空间光照 pass

## 顶点阶段
两者差异不大，主要差别集中在像素与带宽。

## 像素阶段（核心差异）
### Forward
- 片元阶段直接计算最终光照
- 光源越多，单像素计算越重
- 透明支持更自然

### Deferred
- 第一遍写 GBuffer（法线、材质、深度等）
- 第二遍屏幕空间读取 GBuffer 统一算光照
- 多光源扩展性好

## 带宽与存储
- Forward：相对轻
- Deferred：多RT写读，显存/带宽压力大

## 抗锯齿与透明
- Forward：MSAA 更自然，透明流程简单
- Deferred：MSAA 成本高，透明通常单独 forward pass

## 典型适用
- Forward：移动端、透明多、带宽紧张
- Deferred：桌面端、多动态光源

一句话：Forward 简单透明友好；Deferred 多光强但带宽重，工程常用混合方案。

---

## 3. Early-Z 失效原因与优化

### 常见失效/受限场景
1. Pixel Shader 使用 `discard/clip`
2. Pixel Shader 写深度（`SV_Depth/gl_FragDepth`）
3. 透明/alpha相关流程导致早深度收益降低
4. 深度状态配置不当（关闭深测/深写等）
5. 复杂状态组合触发硬件保守路径

### 提升 Early-Z 的常见工程手段
1. Pre-Z / Depth Prepass
2. 减少 `discard`
3. 尽量不在 PS 写深度
4. 不透明前到后排序
5. 透明单独 pass

面试短版：Early-Z 在着色前剔除无效片元；`discard`、写深度、透明等会降低收益；可用 Pre-Z 与状态/顺序优化恢复命中率。

---

## 4. MSAA 通俗解释 + 实现位置

## 什么是 MSAA（通俗）
MSAA = 在一个像素内部采多个 sample 点来判断边缘覆盖率，让边缘更平滑。

直观上：
- 普通像素只看 1 个点
- 4x/8x MSAA 看 4/8 个点
- 根据覆盖比例合成，减轻锯齿

## 怎么实现（核心）
1. 覆盖率按 sample 判断（光栅化阶段）
2. 着色通常按像素执行一次（常规模式）
3. 输出阶段进行 resolve，把多sample结果合成最终像素

## 在管线中的位置
- Rasterization：计算 sample 覆盖
- Depth/Stencil：常按 sample 可见性处理
- Output/Resolve：多sample合成单像素

---

## 5. 为什么 Deferred + MSAA 成本高

核心：Deferred 本就重 GBuffer 读写，MSAA 又把每像素扩成多 sample，带宽/存储/功耗显著上升。

具体：
1. Deferred 多RT写读本身就吃带宽
2. MSAA 引入多sample数据（4x/8x 成本上升）
3. 光照阶段边缘sample处理更复杂

工程应对：
- 不透明 Deferred + 后处理AA（TAA/FXAA）
- Forward+/Clustered 替代重 Deferred+MSAA
- 按平台分档开启 MSAA

---

## 6. TAA 为什么拖影（Ghosting）

一句话：TAA 混历史帧，历史对不齐或衰减慢时会残留旧画面。

### 主因
1. 历史重投影误差（motion vector 不准/遮挡变化）
2. 历史权重过高（旧信息衰减慢）
3. Disocclusion（新露出区域历史无有效信息）

### 高发场景
- 快速镜头运动
- 高对比细纹理（栅栏、树叶）
- 半透明/粒子

### 缓解手段
1. 历史夹取（clamp/neighborhood clamp）
2. 动态调权（速度快时降低历史权重）
3. 改善运动向量质量
4. Reactive mask
5. 后续锐化

面试短版：TAA 通过时域累积降锯齿，拖影源于历史重投影误差和历史权重过高，工程上用 clamp、动态权重、motion vector 优化与 reactive mask 降低 ghosting。

---

## 7. 快速背诵版（2分钟）

GPU 渲染从 CPU 提交命令开始，经输入装配、顶点着色、可选细分/几何、裁剪与视口变换进入光栅化；光栅化生成片元后由像素着色器计算颜色，再通过深度/模板测试与混合写入帧缓冲，最后后处理输出到屏幕。

Forward 直接在像素阶段算最终光照，透明友好；Deferred 先写 GBuffer 再做屏幕空间光照，多光源扩展性好但带宽压力大。Early-Z 可在着色前剔除被挡片元，但在 discard、写深度、透明等场景收益下降。MSAA 通过像素内多sample覆盖来平滑边缘，主要发生在光栅化/深测/resolve链路。Deferred + MSAA 成本高的本质是 GBuffer 与多sample叠加导致带宽和存储开销上升。TAA 拖影则来自历史重投影误差与历史权重过高，通常用 clamp、动态调权和高质量运动向量缓解。
