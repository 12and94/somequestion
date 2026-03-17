# 现代渲染器算法在 Vulkan 中的实现指南（含代码与 Shader 示例）

## 1. 文档目标

本文档针对标准现代渲染器常见算法，给出“可落地”的 Vulkan 实现方式。
每个算法小节都包含以下内容：

1. 放在哪个 Pass
2. 关键 Vulkan 对象
3. 最小 C++ 录制示例 或 Shader 示例（至少一种，尽量两种）
4. 关键同步点（Barrier）

说明：以下代码是教学版最小示例，省略了错误处理、对象封装和完整上下文。

---

## 2. 推荐帧图与资源组织

## 2.1 帧图（示意）

1. Shadow Pass（深度）
2. Main Pass（Forward+ 或 GBuffer，输出 HDR Color + Depth）
3. Post Pass（Bloom、Tone Mapping、AA）
4. UI Pass
5. Present

## 2.2 常用资源

1. `hdrColor`：`R16G16B16A16_SFLOAT`，`COLOR_ATTACHMENT | SAMPLED | STORAGE`
2. `depth`：`D32_SFLOAT`，`DEPTH_STENCIL_ATTACHMENT | SAMPLED`
3. `shadowMap`：`D32_SFLOAT`，`DEPTH_STENCIL_ATTACHMENT | SAMPLED`
4. `indirectBuffer`：`INDIRECT_BUFFER | STORAGE_BUFFER`
5. `visibleBuffer`：`STORAGE_BUFFER`

---

## 3. 算法到 Vulkan 的实现（每节含代码）

## 3.1 Frustum Culling（Compute + Indirect）

放置位置：Main Pass 之前的 Compute Pass。

关键对象：

1. 对象包围体 SSBO
2. 可见列表 SSBO
3. Indirect 命令 SSBO/IndirectBuffer
4. Compute Pipeline

### C++（Compute Dispatch + Barrier）

```cpp
// 1) 绑定 culling compute pipeline
vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, cullPipeline);
vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, cullLayout,
    0, 1, &cullSet, 0, nullptr);

// 2) 调度
uint32_t groupCount = (objectCount + 63) / 64;
vkCmdDispatch(cmd, groupCount, 1, 1);

// 3) Compute 写 indirectBuffer -> DrawIndirect 读
VkBufferMemoryBarrier2 barrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
barrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
barrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
barrier.dstStageMask = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
barrier.dstAccessMask = VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
barrier.buffer = indirectBuffer;
barrier.offset = 0;
barrier.size = VK_WHOLE_SIZE;

VkDependencyInfo dep{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
dep.bufferMemoryBarrierCount = 1;
dep.pBufferMemoryBarriers = &barrier;
vkCmdPipelineBarrier2(cmd, &dep);
```

### GLSL（Compute Shader：写可见列表与 indirect）

```glsl
#version 460
layout(local_size_x = 64) in;

struct DrawIndexedIndirectCommand {
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    int  vertexOffset;
    uint firstInstance;
};

layout(std430, set = 0, binding = 0) readonly buffer Bounds {
    vec4 sphere[]; // xyz=center, w=radius
} bounds;

layout(std430, set = 0, binding = 1) buffer Visible {
    uint visibleIds[];
} visible;

layout(std430, set = 0, binding = 2) buffer Counter {
    uint visibleCount;
} counter;

layout(std430, set = 0, binding = 3) buffer Indirect {
    DrawIndexedIndirectCommand cmd;
} indirectCmd;

layout(push_constant) uniform PC {
    mat4 vp;
    vec4 frustumPlanes[6];
    uint objectCount;
} pc;

bool insideFrustum(vec4 s) {
    for (int i = 0; i < 6; ++i) {
        if (dot(pc.frustumPlanes[i], vec4(s.xyz, 1.0)) < -s.w) return false;
    }
    return true;
}

void main() {
    uint id = gl_GlobalInvocationID.x;
    if (id >= pc.objectCount) return;

    if (insideFrustum(bounds.sphere[id])) {
        uint dst = atomicAdd(counter.visibleCount, 1);
        visible.visibleIds[dst] = id;
    }

    // 只在 0 号线程更新 instanceCount（示例）
    if (id == 0) {
        indirectCmd.cmd.instanceCount = counter.visibleCount;
    }
}
```

---

## 3.2 PBR（Cook-Torrance）

放置位置：Main Pass Fragment Shader。

关键对象：

1. 材质纹理（baseColor/normal/metallicRoughness/ao）
2. 光源 UBO/SSBO
3. 相机 UBO

### C++（Descriptor Layout 片段）

```cpp
std::array<VkDescriptorSetLayoutBinding, 5> b{};
b[0] = {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT};
b[1] = {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT}; // baseColor
b[2] = {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT}; // normal
b[3] = {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT}; // metallicRoughness
b[4] = {4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT}; // ao
```

### GLSL（PBR 核心片段）

```glsl
#version 460
const float PI = 3.14159265359;

float D_GGX(float NoH, float roughness) {
    float a  = roughness * roughness;
    float a2 = a * a;
    float d  = (NoH * NoH) * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d + 1e-6);
}

float G_Smith(float NoV, float NoL, float roughness) {
    float k = (roughness + 1.0);
    k = (k * k) / 8.0;
    float gv = NoV / (NoV * (1.0 - k) + k);
    float gl = NoL / (NoL * (1.0 - k) + k);
    return gv * gl;
}

vec3 F_Schlick(float VoH, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - VoH, 5.0);
}

vec3 EvalPBR(vec3 N, vec3 V, vec3 L, vec3 albedo, float metallic, float roughness) {
    vec3 H = normalize(V + L);
    float NoV = max(dot(N, V), 0.0);
    float NoL = max(dot(N, L), 0.0);
    float NoH = max(dot(N, H), 0.0);
    float VoH = max(dot(V, H), 0.0);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 F  = F_Schlick(VoH, F0);
    float D = D_GGX(NoH, roughness);
    float G = G_Smith(NoV, NoL, roughness);

    vec3 spec = (D * G * F) / max(4.0 * NoV * NoL, 1e-4);
    vec3 kd = (1.0 - F) * (1.0 - metallic);
    vec3 diff = kd * albedo / PI;
    return (diff + spec) * NoL;
}
```

---

## 3.3 IBL（Irradiance + Prefilter + BRDF LUT）

放置位置：

1. 预计算阶段（启动时或离线）
2. Main Pass Fragment Shader 采样

关键对象：

1. `irradianceCube`
2. `prefilterCube`（mip 对应 roughness）
3. `brdfLut2D`

### GLSL（运行时 IBL 采样）

```glsl
layout(set = 1, binding = 5) uniform samplerCube uIrradiance;
layout(set = 1, binding = 6) uniform samplerCube uPrefilter;
layout(set = 1, binding = 7) uniform sampler2D   uBrdfLut;

vec3 EvalIBL(vec3 N, vec3 V, vec3 F0, float roughness, vec3 albedo, float metallic) {
    float NoV = max(dot(N, V), 0.0);
    vec3 R = reflect(-V, N);

    vec3 irradiance = texture(uIrradiance, N).rgb;
    vec3 diffuse = irradiance * albedo * (1.0 - metallic);

    float maxMip = 5.0;
    vec3 prefiltered = textureLod(uPrefilter, R, roughness * maxMip).rgb;
    vec2 brdf = texture(uBrdfLut, vec2(NoV, roughness)).rg;
    vec3 spec = prefiltered * (F0 * brdf.x + brdf.y);

    return diffuse + spec;
}
```

### C++（预计算后转 Shader Read）

```cpp
// 示例：把 prefilterCube 从 TRANSFER_DST 转为 SHADER_READ_ONLY_OPTIMAL
VkImageMemoryBarrier2 img{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
img.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
img.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
img.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
img.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
img.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
img.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
img.image = prefilterImage;
img.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevels, 0, 6};
VkDependencyInfo dep{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
dep.imageMemoryBarrierCount = 1;
dep.pImageMemoryBarriers = &img;
vkCmdPipelineBarrier2(cmd, &dep);
```

---

## 3.4 Shadow Mapping + PCF

放置位置：

1. Shadow Pass（深度写）
2. Main Pass（深度采样比较）

关键对象：

1. 深度-only Pipeline
2. 阴影图 `shadowMap`
3. 光源 VP 矩阵 UBO

### C++（Shadow Pass 录制示例）

```cpp
vkCmdBeginRendering(cmd, &shadowRenderingInfo); // depth attachment only
vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeline);
vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowLayout,
    0, 1, &frameSet, 0, nullptr);

for (auto& m : drawList) {
    vkCmdBindVertexBuffers(cmd, 0, 1, &m.vb, offsets);
    vkCmdBindIndexBuffer(cmd, m.ib, 0, VK_INDEX_TYPE_UINT32);
    vkCmdPushConstants(cmd, shadowLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Mat4), &m.model);
    vkCmdDrawIndexed(cmd, m.indexCount, 1, 0, 0, 0);
}
vkCmdEndRendering(cmd);
```

### GLSL（Main Pass 手动 PCF）

```glsl
float ShadowPCF(sampler2D shadowTex, vec4 lightClip, float bias) {
    vec3 ndc = lightClip.xyz / lightClip.w;
    vec2 uv = ndc.xy * 0.5 + 0.5;
    float current = ndc.z - bias;

    vec2 texel = 1.0 / vec2(textureSize(shadowTex, 0));
    float sum = 0.0;
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float depth = texture(shadowTex, uv + vec2(x, y) * texel).r;
            sum += current <= depth ? 1.0 : 0.0;
        }
    }
    return sum / 9.0;
}
```

### C++（Shadow 写 -> Main 读 Barrier）

```cpp
VkImageMemoryBarrier2 b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
b.srcStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
b.srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
b.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
b.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
b.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
b.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
b.image = shadowImage;
b.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};
VkDependencyInfo dep{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
dep.imageMemoryBarrierCount = 1;
dep.pImageMemoryBarriers = &b;
vkCmdPipelineBarrier2(cmd, &dep);
```

---

## 3.5 HDR + Tone Mapping

放置位置：Post Pass（全屏三角形）。

关键对象：

1. `hdrColor` 作为输入采样
2. Swapchain 图像作为输出

### C++（全屏 pass 录制）

```cpp
vkCmdBeginRendering(cmd, &postRenderingInfo);
vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, toneMapPipeline);
vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, postLayout,
    0, 1, &postSet, 0, nullptr);
// 画全屏三角形，vertex shader 里根据 gl_VertexIndex 生成坐标
vkCmdDraw(cmd, 3, 1, 0, 0);
vkCmdEndRendering(cmd);
```

### GLSL（ACES 近似 tone mapping）

```glsl
vec3 ACESFilm(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    vec3 hdr = texture(uHdrColor, vUV).rgb;
    vec3 mapped = ACESFilm(hdr * uExposure);
    outColor = vec4(mapped, 1.0);
}
```

---

## 3.6 Bloom（Bright + Downsample + Blur + Composite）

放置位置：Post Compute Pass（推荐）。

关键对象：

1. `bloomMip[i]` 链
2. Bright 提取 compute pipeline
3. Blur compute pipeline
4. Composite pipeline

### C++（Compute 多级 dispatch 示例）

```cpp
// Bright pass
vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, brightPipeline);
vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, bloomLayout,
    0, 1, &brightSet, 0, nullptr);
vkCmdDispatch(cmd, (width + 7) / 8, (height + 7) / 8, 1);

for (uint32_t i = 0; i < mipCount - 1; ++i) {
    // downsample i -> i+1
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, downsamplePipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, bloomLayout,
        0, 1, &downsampleSet[i], 0, nullptr);
    vkCmdDispatch(cmd, (mipW[i+1] + 7) / 8, (mipH[i+1] + 7) / 8, 1);

    // 简化示例：每级后插 barrier
    VkMemoryBarrier2 mb{VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
    mb.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    mb.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
    mb.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    mb.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
    VkDependencyInfo di{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    di.memoryBarrierCount = 1;
    di.pMemoryBarriers = &mb;
    vkCmdPipelineBarrier2(cmd, &di);
}
```

### GLSL（Bright 提取）

```glsl
#version 460
layout(local_size_x = 8, local_size_y = 8) in;
layout(set = 0, binding = 0, rgba16f) uniform readonly image2D srcHdr;
layout(set = 0, binding = 1, rgba16f) uniform writeonly image2D dstBright;
layout(push_constant) uniform PC { float threshold; } pc;

void main() {
    ivec2 p = ivec2(gl_GlobalInvocationID.xy);
    vec3 c = imageLoad(srcHdr, p).rgb;
    float luma = dot(c, vec3(0.2126, 0.7152, 0.0722));
    vec3 b = (luma > pc.threshold) ? c : vec3(0.0);
    imageStore(dstBright, p, vec4(b, 1.0));
}
```

---

## 3.7 抗锯齿（MSAA / FXAA / TAA）

### 3.7.1 MSAA（Vulkan Pipeline 状态）

```cpp
VkPipelineMultisampleStateCreateInfo msaa{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
msaa.rasterizationSamples = VK_SAMPLE_COUNT_4_BIT;
msaa.sampleShadingEnable = VK_TRUE;
msaa.minSampleShading = 0.5f;
// 主 pass 写入 multisampled color，随后 resolve 到单采样图
```

### 3.7.2 FXAA（Fragment Shader 简化示例）

```glsl
vec3 FXAA(sampler2D tex, vec2 uv, vec2 invRes) {
    vec3 c  = texture(tex, uv).rgb;
    vec3 n  = texture(tex, uv + vec2(0, -invRes.y)).rgb;
    vec3 s  = texture(tex, uv + vec2(0,  invRes.y)).rgb;
    vec3 e  = texture(tex, uv + vec2( invRes.x, 0)).rgb;
    vec3 w  = texture(tex, uv + vec2(-invRes.x, 0)).rgb;

    float l = dot(c, vec3(0.299, 0.587, 0.114));
    float ln = dot(n, vec3(0.299, 0.587, 0.114));
    float ls = dot(s, vec3(0.299, 0.587, 0.114));
    float le = dot(e, vec3(0.299, 0.587, 0.114));
    float lw = dot(w, vec3(0.299, 0.587, 0.114));

    float edge = abs((ln + ls + le + lw) * 0.25 - l);
    return mix(c, (n + s + e + w + c) / 5.0, clamp(edge * 6.0, 0.0, 1.0));
}
```

### 3.7.3 TAA（重投影核心片段）

```glsl
vec3 ResolveTAA(vec2 uv) {
    vec2 vel = texture(uVelocity, uv).xy;
    vec2 prevUV = uv - vel;
    vec3 curr = texture(uCurrColor, uv).rgb;
    vec3 hist = texture(uHistoryColor, prevUV).rgb;
    float alpha = 0.1; // 历史权重
    return mix(curr, hist, alpha);
}
```

关键同步点：history 图像在帧间 ping-pong，确保上一帧写完成后本帧再读。

---

## 3.8 Instancing / Indirect Draw

放置位置：Main Pass Draw 阶段。

关键对象：

1. 实例数据 Buffer（mat4 + materialId）
2. Indirect 命令 Buffer

### GLSL（Vertex：读取实例矩阵）

```glsl
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNrm;

layout(std430, set = 2, binding = 0) readonly buffer InstanceData {
    mat4 model[];
} inst;

layout(set = 0, binding = 0) uniform FrameUBO {
    mat4 viewProj;
} frame;

void main() {
    mat4 M = inst.model[gl_InstanceIndex];
    gl_Position = frame.viewProj * M * vec4(inPos, 1.0);
}
```

### C++（Indirect Draw）

```cpp
vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mainPipeline);
vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mainLayout,
    0, 1, &frameSet, 0, nullptr);

vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer, offsets);
vkCmdBindIndexBuffer(cmd, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

vkCmdDrawIndexedIndirect(cmd, indirectBuffer, 0,
    drawCount, sizeof(VkDrawIndexedIndirectCommand));
```

---

## 3.9 Render Graph（自动依赖 + Barrier）

放置位置：渲染器调度层。

关键对象：

1. Pass 节点
2. 资源读写边
3. 编译阶段的拓扑排序与 barrier 生成

### C++（伪代码：Pass 声明）

```cpp
graph.AddPass("Shadow", [&](PassBuilder& b) {
    b.WriteDepth("shadowMap");
}, [&](VkCommandBuffer cmd, const Resources& r) {
    RecordShadowPass(cmd, r.shadowMap);
});

graph.AddPass("Main", [&](PassBuilder& b) {
    b.ReadDepth("shadowMap");
    b.WriteColor("hdrColor");
    b.WriteDepth("depth");
}, [&](VkCommandBuffer cmd, const Resources& r) {
    RecordMainPass(cmd, r.hdrColor, r.depth, r.shadowMap);
});

graph.AddPass("Tonemap", [&](PassBuilder& b) {
    b.ReadSampled("hdrColor");
    b.WriteColor("swapchain");
}, [&](VkCommandBuffer cmd, const Resources& r) {
    RecordTonemapPass(cmd, r.hdrColor, r.swapchain);
});

graph.Compile(); // 生成拓扑顺序 + barrier
```

### C++（伪代码：自动插入 Barrier）

```cpp
for (auto& edge : compiledEdges) {
    VkImageMemoryBarrier2 img = BuildBarrierFromUsage(edge.prevUsage, edge.nextUsage, edge.image);
    VkDependencyInfo dep{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dep.imageMemoryBarrierCount = 1;
    dep.pImageMemoryBarriers = &img;
    vkCmdPipelineBarrier2(cmd, &dep);
}
```

---

## 4. 常见同步模板（可直接抄用）

## 4.1 Shadow 写 -> Main 读

```cpp
// src: depth attachment write
// dst: fragment sampled read
```

## 4.2 Main HDR 写 -> Post 采样读

```cpp
// oldLayout = COLOR_ATTACHMENT_OPTIMAL
// newLayout = SHADER_READ_ONLY_OPTIMAL
```

## 4.3 Compute 写 indirect -> DrawIndirect 读

```cpp
// srcStage = COMPUTE_SHADER
// dstStage = DRAW_INDIRECT
```

建议：尽量把上述模板封装成统一函数，避免散落在各个业务 pass 中。

---

## 5. 调试与验证建议

1. 全程开启 Validation Layer。
2. 每个 pass 打 debug label，RenderDoc 抓帧逐 pass 检查。
3. 针对每个算法做“开关对比”：功能正确性 + 帧时变化。
4. 对复杂算法（Bloom/TAA/CSM）输出中间纹理做可视化检查。

---

## 6. 里程碑建议（按可学习性排序）

1. Shadow + PBR（先建立主链路）
2. IBL + HDR/ToneMapping（补齐现代画质）
3. Bloom + AA（后处理体系）
4. Frustum Culling + Instancing + Indirect（性能体系）
5. Render Graph（工程化与可维护性）

完成以上里程碑后，这份文档中的每个算法都可以在 Vulkan 中对应到明确代码路径与 Shader 实现。
