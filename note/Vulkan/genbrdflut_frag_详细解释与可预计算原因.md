# genbrdflut.frag 详细解释与“为什么可以这么做”

## 1. 这个 shader 在做什么

`shaders/glsl/pbrtexture/genbrdflut.frag` 的作用是预计算一张 **2D BRDF LUT**。  
这张表把“镜面 IBL 中昂贵的一部分积分”提前烘焙为纹理，运行时只需要采样。

- 输入坐标：
  - `inUV.x` -> `NoV`（法线与视线夹角余弦）
  - `inUV.y` -> `roughness`
- 输出：
  - `outColor.rg` 两个系数（常称 A/B 或 scale/bias）

对应主着色器的使用：

- `vec2 brdf = texture(samplerBRDFLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;`
- `vec3 specular = reflection * (F * brdf.x + brdf.y);`

文件：
- `shaders/glsl/pbrtexture/genbrdflut.frag`
- `shaders/glsl/pbrtexture/pbrtexture.frag`

---

## 2. 核心函数拆解

### 2.1 `random(vec2 co)`
提供少量扰动，降低采样结构化噪声。

### 2.2 `hammersley2d(i, N)`
生成低差异序列采样点，比纯随机更均匀，积分收敛更快。

### 2.3 `importanceSample_GGX(Xi, roughness, normal)`
把二维采样点映射到 GGX 分布下的半程向量 `H`。
- roughness 小 -> 分布更集中
- roughness 大 -> 分布更分散

### 2.4 `G_SchlicksmithGGX(dotNL, dotNV, roughness)`
几何遮蔽项 `G` 的近似，表示微表面自遮挡。

### 2.5 `BRDF(NoV, roughness)`
主积分函数：
1. 固定 `N = (0,0,1)`，把问题降维为 2D 查表。
2. 循环 `NUM_SAMPLES`（默认 1024）采样。
3. 每个样本计算 `H`、`L`、`G_Vis`、`Fc`。
4. 分别累加到 `LUT.x` 与 `LUT.y`。
5. 平均后返回。

---

## 3. 为什么“可以这么做”

这不是魔法，是工程近似（split-sum）。

### 3.1 原始镜面 IBL积分很贵
每像素都做半球积分实时成本太高。

### 3.2 可拆分为两部分
在 GGX + Schlick Fresnel + 各向同性假设下，可近似拆分为：
1. 与环境贴图相关部分（由 prefiltered environment map 提供）
2. 仅与 `NoV` 和 `roughness` 相关部分（由 BRDF LUT 提供）

### 3.3 各向同性让维度降到 2D
固定法线方向（`N` 指向 z）后，参数可压缩为：
- `NoV`
- `roughness`
所以 LUT 做成二维即可。

### 3.4 Fresnel 可线性分解
代码中把 Fresnel 相关项拆成两路累计（`LUT.x / LUT.y`），运行时再与 `F` 组合。

---

## 4. 运行时收益

运行时从“每像素大量 Monte Carlo 积分”变成：
1. 采样预过滤环境图（`prefilteredMap`）
2. 采样 BRDF LUT（`samplerBRDFLUT`）
3. 常数次组合计算

这就是实时 PBR 常见的高性价比路径。

---

## 5. 结论

`genbrdflut.frag` 能预计算，是因为 split-sum 近似把镜面 IBL 拆成“环境相关 + BRDF相关”。  
BRDF相关项只依赖 `NoV` 与 `roughness`，因此能烘焙成 2D LUT，在运行时高效查表。
