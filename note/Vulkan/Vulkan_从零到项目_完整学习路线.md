# Vulkan 从零到项目：完整学习路线（可落地版）

这份文档目标不是“背概念”，而是让你从 0 到能独立做一个小型 Vulkan 渲染器。
你可以把它当作学习路线 + 实战清单 + 排错手册。

---

## 1. 先建立正确预期

### 1.1 Vulkan 到底难在哪里
1. Vulkan 不是“帮你做事”的 API，而是“让你自己明确做事”的 API。
2. 难点不是单个函数，而是系统工程：资源生命周期、同步关系、命令组织、性能模型。
3. 你会遇到大量“黑屏但不报错”的阶段，这很正常。

### 1.2 Vulkan 的核心收益
1. 可预测性强，驱动黑盒少。
2. 多线程录制命令友好。
3. 更适合大型项目和长期维护。
4. 移动端和桌面端都有广泛支持（通过不同平台层封装）。

### 1.3 你应该怎么学
1. 不要上来读规范全书。
2. 先最小可运行，再逐步加模块。
3. 每加一个模块都做“可见结果”和“调试验证”。

---

## 2. 开始前必须准备什么

## 2.1 语言与工程基础
1. C++ 基础：RAII、智能指针、容器、异常处理、构建系统。
2. C++ 进阶：移动语义、模板基础、内存布局、性能分析。
3. 构建环境：CMake、调试器（VS/VSCode + lldb/gdb）。

## 2.2 图形学基础（你已有笔记）
1. 坐标系与矩阵。
2. 光栅化管线。
3. 纹理采样与 Mipmap。
4. 深度测试与混合。
5. 相机和投影。

## 2.3 工具链
1. Vulkan SDK。
2. Validation Layers（务必开）。
3. RenderDoc（帧抓取分析）。
4. GPU 厂商工具（Nsight / Radeon GPU Profiler / Xcode GPU Tools）。
5. Shader 编译工具（glslangValidator / DXC / shaderc）。

## 2.4 学习过程中“强制开启”的习惯
1. 每帧检查关键 API 返回值。
2. Debug 模式开启 validation layer。
3. 每做一层功能都抓一帧 RenderDoc 检查。
4. 记录每次修改前后的行为差异。

---

## 3. Vulkan 心智模型（先吃透这 10 句）

1. `Instance` 是 Vulkan 世界入口。
2. `PhysicalDevice` 是“你有哪些 GPU 可选”。
3. `Device` 是“选定 GPU 后的逻辑设备上下文”。
4. `Queue` 是命令执行通道。
5. `Swapchain` 是“呈现到屏幕”的缓冲链。
6. `CommandBuffer` 是“先录制，后提交”的命令列表。
7. `Pipeline` 是固定渲染状态和 shader 组合。
8. `Descriptor` 是 shader 访问资源的绑定桥梁。
9. `Buffer/Image + Memory` 分离，内存要自己管。
10. 同步要自己说清楚：Fence / Semaphore / Barrier。

---

## 4. 最小可运行三角形：你必须理解的完整链路

## 4.1 初始化阶段
1. 创建 `VkInstance`。
2. 创建窗口 surface。
3. 选择 `VkPhysicalDevice`。
4. 创建 `VkDevice` 和图形队列 / 呈现队列。
5. 创建 swapchain 和 image views。
6. 创建 render pass（或 dynamic rendering）。
7. 创建 graphics pipeline。
8. 创建 framebuffers（若使用 render pass 路径）。
9. 创建 command pool + command buffers。
10. 创建同步对象（semaphore + fence）。

## 4.2 每帧执行阶段
1. `acquire image` 获取当前要渲染的 swapchain image。
2. 录制 command buffer（begin -> render pass -> draw -> end）。
3. queue submit（等待 image available semaphore）。
4. present（等待 render finished semaphore）。
5. 用 fence 做 CPU/GPU 帧同步。

## 4.3 常见首阶段错误
1. 忘记设置 viewport/scissor（动态状态）。
2. vertex input 与 shader layout 不匹配。
3. render pass 附件格式与 swapchain 格式不匹配。
4. 同步对象使用时机错导致偶发闪烁。

---

## 5. 必须掌握的模块（按学习顺序）

## 5.1 模块 A：Buffer 与 Memory
目标：你能上传顶点、索引、uniform 数据。

要掌握：
1. `VkBuffer` 创建。
2. 内存需求查询：size/alignment/typeBits。
3. 内存类型选择：DEVICE_LOCAL vs HOST_VISIBLE。
4. staging buffer 上传路径。
5. ring buffer 处理每帧 uniform 更新。

检查点：
1. 能画出带颜色三角形。
2. 能更新 MVP 矩阵并旋转模型。

## 5.2 模块 B：Image 与纹理
目标：你能正确采样纹理并理解 image layout。

要掌握：
1. `VkImage` + `VkImageView` + `VkSampler`。
2. `vkCmdCopyBufferToImage` 上传纹理。
3. image layout transition（TRANSFER_DST -> SHADER_READ_ONLY）。
4. mipmap 生成或离线导入。

检查点：
1. 能给模型贴纹理。
2. RenderDoc 能看到正确的 image layout 变化。

## 5.3 模块 C：Descriptor 与 Pipeline Layout
目标：你能给 shader 绑定 UBO、纹理、采样器。

要掌握：
1. descriptor set layout。
2. descriptor pool 与 set 分配。
3. update descriptor sets。
4. pipeline layout 与 set/binding 对齐。

检查点：
1. 多个物体能共享 pipeline 但使用不同 descriptor。
2. 不出现“binding mismatch”验证错误。

## 5.4 模块 D：深度测试与深度资源
目标：你能正确渲染遮挡关系。

要掌握：
1. depth image 创建和格式选择。
2. depth attachment 配置。
3. depth test / write 参数。

检查点：
1. 近处物体正确遮挡远处。
2. 相机移动时无遮挡错乱。

## 5.5 模块 E：模型加载与多物体渲染
目标：你能导入网格并组织 draw call。

要掌握：
1. 顶点结构与 attribute layout。
2. 索引缓冲。
3. per-object 常量与批处理。

检查点：
1. 加载 obj/gltf 模型。
2. 多物体渲染稳定且无错位。

## 5.6 模块 F：同步进阶
目标：你能解释并修复常见同步问题。

要掌握：
1. CPU/GPU 同步：Fence。
2. GPU/GPU 提交依赖：Semaphore（binary/timeline）。
3. Barrier：stage mask + access mask + layout transition。
4. 多帧并行（frames in flight）。

检查点：
1. 不出现随机黑帧、闪烁、偶发错误。
2. 能解释“为什么这个 barrier 需要这样写”。

## 5.7 模块 G：后处理与多 Pass
目标：你能做一个后处理链路（如 bloom 或 tone mapping）。

要掌握：
1. 离屏渲染到中间 RT。
2. Pass 间资源转场。
3. 全屏三角形后处理。

检查点：
1. 能切换开关验证后处理前后差异。
2. 能稳定输出到 swapchain。

---

## 6. Vulkan 最难点专题：同步到底怎么想

## 6.1 不要背 API，先背问题
每个同步都在回答三件事：
1. 谁先谁后执行。
2. 谁写的数据何时对谁可见。
3. 资源当前处于什么使用状态（layout/访问类型）。

## 6.2 Fence / Semaphore / Barrier 分工
1. Fence：CPU 等 GPU 完成。
2. Semaphore：提交之间的等待关系。
3. Barrier：命令内部或命令序列中资源可见性和状态转换。

## 6.3 最常见资源转场例子
场景：Pass1 写颜色纹理，Pass2 采样这个纹理。

你需要保证：
1. Pass1 真的写完。
2. 写入对 Pass2 可见。
3. 纹理 layout 从颜色附件切到采样只读。

如果三者少任何一项，都可能出现“偶发黑块或闪烁”。

## 6.4 排错策略
1. 先把同步写“保守正确”，再优化。
2. 打开 validation，先消灭报错。
3. 用 RenderDoc 检查每个 pass 输入输出是否如预期。
4. 对偶发错误先固定帧率与线程调度，降低非确定性。

---

## 7. Shader 与资源绑定你要做到的“完全懂”

## 7.1 你必须建立的对应关系
shader 里：
```glsl
layout(set=0, binding=0) uniform CameraUBO { ... };
layout(set=0, binding=1) uniform sampler2D albedoTex;
```

Vulkan 里必须有：
1. set=0 的 descriptor set layout。
2. binding=0 的 UBO 类型。
3. binding=1 的 combined image sampler 类型。
4. pipeline layout 引用了这个 set layout。

任何一个不一致都会出问题。

## 7.2 常见组织方式
1. set0：每帧全局数据（相机、光照）。
2. set1：材质数据（纹理、参数）。
3. set2：对象数据（变换、实例信息）。

这样做利于复用和减少更新成本。

---

## 8. 性能优化路线（从能跑到跑快）

## 8.1 第一步：先看 CPU 还是 GPU 瓶颈
1. CPU 高：命令录制、资源更新、同步等待、驱动调用太多。
2. GPU 高：过绘、带宽、复杂 shader、RT 分辨率、后处理链路。

## 8.2 CPU 侧优化
1. 减少频繁创建销毁对象。
2. 命令缓冲按帧复用，按线程分配 command pool。
3. 合理使用 secondary command buffer 并行录制。
4. 批处理和实例化减少 draw call。

## 8.3 GPU 侧优化
1. 降低 overdraw。
2. 合理使用 depth pre-pass 或 early-z 思路。
3. 控制后处理分辨率和 pass 数量。
4. 减少不必要的 image layout 来回切换。
5. 关注带宽敏感资源格式。

## 8.4 移动端额外重点
1. 适配 tile 架构，减少中间 RT 写回。
2. 合并 pass，减少外存往返。
3. 控制纹理带宽和采样次数。

---

## 9. 一条完整的实战里程碑（建议照着做）

## M0：窗口清屏
目标：初始化成功，能呈现纯色。

## M1：三角形
目标：最小 pipeline 跑通。

## M2：MVP 变换
目标：相机和模型矩阵生效。

## M3：纹理贴图
目标：image 上传与采样跑通。

## M4：深度测试
目标：3D 遮挡关系正确。

## M5：模型加载
目标：导入 glTF 并显示。

## M6：多光源 + PBR 基础
目标：基础材质流程成型。

## M7：阴影（先方向光）
目标：ShadowMap 路径打通。

## M8：后处理（Tone Mapping + Bloom）
目标：HDR 管线完整。

## M9：性能与稳定性
目标：多帧并行、剔除、资源池、稳定帧时。

完成 M9 后，你就不是“会用 Vulkan”，而是“能做 Vulkan 项目”。

---

## 10. 常见报错与黑屏排查清单

## 10.1 黑屏优先级检查
1. swapchain image 是否 acquire 成功。
2. command buffer 是否真的 submit。
3. render pass / framebuffer / attachments 是否匹配。
4. pipeline 是否绑定成功。
5. viewport/scissor 是否正确。
6. descriptor 是否更新且绑定。
7. shader 是否编译与反射匹配。
8. 同步是否导致读未写资源。

## 10.2 Validation 常见错误类型
1. 生命周期错误（对象先销毁后使用）。
2. 资源状态错误（layout/access 不匹配）。
3. descriptor 绑定错误（set/binding/type 不一致）。
4. 同步错误（missing barrier / wrong stage mask）。

## 10.3 你必须养成的习惯
1. 每次只改一件事。
2. 出错后先回退到上一个可运行状态。
3. 保存“最小复现”分支。

---

## 11. 面试回答模板（你可以直接背）

## 11.1 Vulkan 的核心特点
`Vulkan 是显式低层图形 API，应用侧负责内存、同步和命令组织。代价是开发复杂度高，收益是可预测性和多线程扩展性更强，适合大型实时渲染引擎。`

## 11.2 Vulkan 同步怎么理解
`我把同步分三层：Fence 处理 CPU-GPU 完成关系，Semaphore 处理提交依赖，Barrier 处理资源可见性和 layout 转换。真正难点是把渲染图中的数据依赖正确映射到 stage/access。`

## 11.3 Vulkan 内存管理怎么做
`Vulkan 资源与内存分离，工程里通常不用每资源独立分配，而是用大块子分配或 VMA。常见策略是 staging 上传到 device-local，结合 ring buffer 管理动态常量。`

## 11.4 为什么 Vulkan 不一定天然更快
`Vulkan提供更高上限和可控性，但收益依赖实现质量。如果同步和资源管理做得不好，性能和稳定性都可能变差。`

---

## 12. 你现在可以怎么开始（本周可执行）

## Day 1-2
1. 安装 Vulkan SDK。
2. 跑通官方最小示例。
3. 打开 validation layer，确认无报错。

## Day 3-4
1. 自己写 M0 + M1（清屏 + 三角形）。
2. 学会用 RenderDoc 抓帧看 drawcall。

## Day 5-7
1. 完成 M2 + M3（MVP + 纹理）。
2. 写一个小结：你遇到的 5 个坑和修复方法。

只要第一周走完，你就已经跨过最难启动期。

---

## 13. 最后一句话

学 Vulkan 的关键不是“看懂了多少 API”，而是：
1. 你能不能把一个渲染目标拆成可执行模块。
2. 你能不能用调试工具定位问题。
3. 你能不能在复杂度上升时保持系统稳定。

这份文档就是让你按“可执行路径”去掌握 Vulkan。
