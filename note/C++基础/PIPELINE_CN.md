# vk_cs 当前完整流程说明（中文）

本文档对应当前仓库代码实现，目标是把“程序每一步做了什么”与“对应物理/数值公式”一一对齐，便于调试、优化和二次开发。

## 1. 总览

`vk_cs` 是一个基于 Vulkan Compute 的子空间有限元（Subspace FEM）仿真程序，核心特征：

1. 使用四面体网格（`.msh` 或 `.json`）和预计算子空间基底（`*_result.json`）。
2. 动力学状态 `x, x_n, v_n, x_star` 常驻 GPU。
3. 梯度与 Hessian-向量乘采用“两阶段（tet stage + vertex gather）”GPU 算法，避免原子冲突。
4. 牛顿迭代在 CPU 控制，重算/线性算子在 GPU 执行。
5. 线性子问题默认用 GPU 端 reduced-CG（固定迭代上限，支持 GPU 端早停）。
6. 可选输出 OBJ 序列、仅回读不写盘、或实时窗口渲染。

## 2. 启动与参数解析

入口在 `src/main.cpp`。

命令格式：

```powershell
.\build\Release\vk_cs.exe <mesh_path_or_dir> <basis_json_path> <frames> [--no-obj] [--download-only] [--render]
```

参数行为：

1. `--no-obj`：不回读、不写 OBJ（除非 `--render` 触发回读）。
2. `--download-only`：回读顶点，但不写 OBJ。
3. `--render`：开启实时窗口渲染（SDL3）。
4. 若输入是目录，会自动在目录里查找 mesh 与 `_result.json`。
5. 程序启动后会打印实际 Vulkan 计算设备名。

## 3. 输入数据与基础结构

## 3.1 网格输入

来自 `src/mesh_loader.cpp`：

1. 支持 Gmsh ASCII `.msh`。
2. 支持简化 `.json`（`vertices`, `tetrahedra`）。
3. 读入后统一做四面体方向修正（保证体积符号一致）。

## 3.2 子空间基底输入

来自 `src/subspace_loader.cpp`：

1. 读取 `basis`（行主序），并按 `subspace_dim_limit` 截断列数。
2. 读取可选材料参数：`young`, `poisson`。
3. 读取可选 `vertex_mass`。

硬约束：

1. `basis.rows == 3 * vertex_count`，否则初始化失败。
2. 这意味着基底必须与当前网格自由度一一匹配。

## 3.3 关键数据结构

`src/types.h` 中：

1. `MeshData`：顶点、四面体、`Dm^{-1}`、体积、质量。
2. `SubspaceBasisData`：`U`（`basis_row_major`）、材料与顶点质量。
3. `CsrAdjacency`：顶点到邻接四面体 CSR（用于阶段二 gather）。

## 4. 初始化阶段（Initialize）

流程在 `src/subspace_simulator.cpp::Initialize`。

## 4.1 材料与派生参数

来自 `src/sim_params.h`：

\[
\mu = \frac{E}{2(1+\nu)}, \quad
\lambda = \frac{E\nu}{(1+\nu)(1-2\nu)}
\]

\[
\alpha = 1 + \frac{\mu}{\lambda}, \quad
dt = \frac{\text{time\_step}}{\text{num\_substeps}}
\]

注：`alpha` 用在当前能量模型中（见后文）。

## 4.2 初始几何状态

对原始网格点做：

1. 先绕 Z 轴旋转（固定角度 `theta=0.5`）。
2. 平移到初始高度 `initial_height` 附近。
3. 施加可选初始拉伸（`initial_stretch_x/y/z`）。

得到：

1. `x_rest`：参考构形（rest pose）。
2. `x, x_n, x_star`：初始当前状态。
3. `v_n = 0`。

## 4.3 参考四面体预计算

`BuildRestTetData` 生成：

1. \( D_m^{-1} \in \mathbb{R}^{3\times3} \)。
2. \( V_0 \)（静止体积）。
3. 顶点 lumped mass：
\[
m_i = \sum_{t \ni i}\frac{\rho V_t}{4}
\]

## 4.4 CSR 建图

`BuildTetVertexCsr` 构建顶点到四面体的邻接索引：

1. `offsets[v]..offsets[v+1]-1` 给出顶点 `v` 邻接条目。
2. 每条包含 `(tet_id, local_id)`。

## 4.5 Vulkan 资源初始化

`VkTwoPhaseOps::Initialize` 做：

1. Vulkan instance/device/queue/command pool 初始化。
2. 静态缓冲上传：`basis`, `x_rest`, `mass`, `tets`, `DmInv`, `vol`, CSR。
3. 动态缓冲创建：`x/x_star/x_n/v_n/p/force/result`、reduced buffers、CG 控制与间接 dispatch 缓冲。
4. 创建并绑定 compute pipeline：
   - 梯度阶段 A/B
   - Hessian-vector 阶段 A/B
   - `predict_state`、`update_velocity_state`
   - `reconstruct_x_from_reduced`
   - `build_world_from_reduced`
   - `project_world_to_reduced_stage1/2`
   - `reduced_cg_init`、`reduced_cg_update`

## 5. 核心物理模型与目标函数

当前实现可视为隐式欧拉一步最小化问题：

\[
\Phi(x) =
\frac{1}{2dt^2}\sum_i m_i\|x_i-x_i^\*\|^2
\;+\;
\sum_t V_t\,\Psi(F_t)
\;+\;
E_{\text{ground}}(x)
\]

其中：

1. \(x^\* = x_n + dt\,v_n + dt^2 g\)（预测位形）。
2. 变形梯度：
\[
F = D_s D_m^{-1},\quad
D_s=[x_1-x_0,\;x_2-x_0,\;x_3-x_0]
\]
3. 四面体能量密度（与 shader 对应）：
\[
\Psi(F)=\frac{\mu}{2}\|F\|_F^2 + \frac{\lambda}{2}\big(\det(F)-\alpha\big)^2
\]
4. 一阶 Piola 应力：
\[
P = \mu F + \lambda\big(J-\alpha\big)J F^{-T},\quad J=\det(F)
\]
5. 地面罚函数（启用时）：
\[
E_{\text{ground},i}=
\begin{cases}
\frac12 k\,(y_g-y_i)^2,& y_i<y_g\\
0,& y_i\ge y_g
\end{cases}
\]

## 6. 每帧/每子步执行流程

`Run(total_frames)` 外层每帧循环，每帧执行 `num_substeps` 次 `Substep()`。

单个子步如下。

## 6.1 预测步（GPU）

`predict_state.comp`：

\[
x^\* = x_n + dt\,v_n + dt^2 g,\qquad x \leftarrow x^\*
\]

## 6.2 牛顿迭代（CPU 控制 + GPU 算子）

循环 `newton_iters` 次，或满足收敛提前退出。

### 6.2.1 reduced 梯度计算

调用 `ComputeGradientReducedFromQ(q)`，内部顺序：

1. `reconstruct_x_from_reduced.comp`：
\[
x = x_{\text{rest}} + Uq
\]
2. 梯度两阶段：
   - `gradient_tet_stage.comp`：每个四面体计算局部 4 顶点贡献并写入 `tetContrib`。
   - `gradient_vertex_gather.comp`：按 CSR 汇总到顶点，得到世界梯度 \(g\)。

顶点梯度形式：
\[
g_i = \frac{m_i}{dt^2}(x_i-x_i^\*) + g^{\text{elastic}}_i + g^{\text{ground}}_i
\]

3. 投影到 reduced 空间：
   - `project_world_to_reduced_stage1.comp`：按 chunk 并行归约。
   - `project_world_to_reduced_stage2.comp`：chunk 求和。

\[
g_r = U^T g
\]

收敛判据：
\[
\frac{\|g_r\|_2}{r} < \text{convergence\_reduced\_avg\_norm}
\]

### 6.2.2 线性子问题

解：
\[
H_r \Delta q = -g_r
,\qquad
H_r p = U^T H (Up)
\]

有两条路径：

1. `use_reduced_direct_solve=true` 且 `r` 小时：
   - 用 \(H_r e_j\) 列装配 \(H_r\)。
   - 对称化后加阻尼：
\[
H_r \leftarrow H_r + \eta I
\]
   - CPU Cholesky 求解。
2. 默认 GPU reduced-CG（`SolveReducedCgGpuFixed`）：
   - `reduced_cg_init.comp` 初始化 \(x=0,r=b,p=b\)。
   - 每次迭代执行：
     1. 间接调度计算 \(Ap = H_r p\)（含 `build_world + hessp + project`）。
     2. `reduced_cg_update.comp` 更新
\[
\alpha_k = \frac{r_k^Tr_k}{p_k^T(Ap_k + \gamma p_k)}
\]
\[
x_{k+1}=x_k+\alpha_k p_k,\quad
r_{k+1}=r_k-\alpha_k(Ap_k+\gamma p_k)
\]
\[
\beta_k=\frac{r_{k+1}^Tr_{k+1}}{r_k^Tr_k},\quad
p_{k+1}=r_{k+1}+\beta_k p_k
\]
     3. 依据 `rrStop` 与阈值设置 `converged`，并把后续间接 dispatch 设为 0（GPU 端早停）。

### 6.2.3 更新 reduced 坐标

\[
q \leftarrow q + \Delta q
\]

## 6.3 子步收尾（GPU）

`ReconstructAndUpdateVelocityFromQ(q)`：

1. 重构：
\[
x = x_{\text{rest}} + Uq
\]
2. 更新速度与历史位形：
\[
v_n = \frac{x - x_n}{dt},\qquad x_n \leftarrow x
\]

## 7. 两阶段算子细节（避免原子冲突）

统一模式：

1. Stage A（按 tet 并行）：每个四面体写连续 4 条局部贡献。
2. Stage B（按 vertex 并行）：每顶点通过 CSR 遍历相关四面体并累加。

优点：

1. 不需要对全局顶点数组做高冲突 `atomicAdd`。
2. 访问模式稳定，吞吐更可控。

## 8. Hessian-向量乘具体形式

`hessp_tet_stage.comp` 使用 \(dP = \partial P / \partial F : dF\) 的显式 9x9 形式，等价于：

\[
H_F = \mu I + \lambda\left(\frac{\partial J}{\partial F}\otimes\frac{\partial J}{\partial F}
 + (J-\alpha)\frac{\partial^2 J}{\partial F^2}\right)
\]

然后：

\[
dF = dD_s D_m^{-1},\quad
dH = dP D_m^{-T}
\]

再转换为四面体 4 顶点增量并进入阶段 B 汇总，最终得到：

\[
Hp = \frac{M}{dt^2}p + \text{elastic\_hessp}(p) + \text{ground\_hessp}(p)
\]

其中地面项在 \(y<y_g\) 时加上 \(k\,p_y\)。

## 9. 输出、回读与渲染

每帧子步结束后：

1. 若需要（`download_only` 或写 OBJ 或 `--render`），执行 `DownloadX`。
2. 若写 OBJ：写入 `<mesh>.vkcs_output/frame_*.obj`。
3. 若 `--render`：
   - SDL3 窗口实时显示；
   - 右键拖拽旋转、滚轮缩放、`R` 重置、`Esc` 退出渲染循环；
   - 当前渲染是 CPU 光栅化显示，不参与物理求解。

## 10. 计时统计字段含义

主程序最终输出：

1. `predict`：预测步耗时。
2. `gradient_reduced`：reduced 梯度（含重构 + 梯度 + 投影）。
3. `linear_solve_total`：线性解总耗时（CG 或 direct）。
4. `hessian_reduced_calls`：线性解中 Hessian-reduced 相关耗时累计。
5. `reconstruct_final`：子步收尾重构与速度更新。
6. `download_x`：GPU->CPU 回读位形耗时。
7. `write_obj`：写盘耗时。
8. `render`：实时渲染耗时。

## 11. 当前实现边界与注意事项

1. 物理求解使用 FP32 shader。
2. 当前 `SolveReducedCgGpuFixed` 是“固定迭代上限 + GPU 端可早停”。
3. 子空间维度默认 `r=40`，可通过 `subspace_dim_limit` 截断。
4. 直接解（CPU Cholesky）只建议在低维 `r` 使用。
5. 若只评估“纯模拟”，建议使用 `--no-obj` 并关闭渲染。

## 12. 一句话流程图

\[
\text{载入 mesh/basis} \rightarrow
\text{预计算 }(D_m^{-1},V,m,\text{CSR}) \rightarrow
\text{GPU 初始化} \rightarrow
\text{每帧每子步: Predict} \rightarrow
\text{Newton: } g_r,\;H_r\Delta q=-g_r,\;q\!+\!=\Delta q \rightarrow
\text{重构并更新速度} \rightarrow
\text{可选回读/输出/渲染}
\]

