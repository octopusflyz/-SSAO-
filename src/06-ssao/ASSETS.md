SSAO Demo — 最小素材与下载指南

目标：为最小可行场景准备素材（人物、建筑、车辆、HDRI），并把模型放在 `assets/` 目录下（首选 glTF/.glb）。

推荐资源（可直接下载，均为学生友好）：

1) 人物与动画：Mixamo (https://www.mixamo.com)
   - 搜索并下载带动画的角色，导出为 FBX 或 glTF（若没 glTF，可用 Blender 转为 glTF）。
   - 动作建议：idle, walk, run, shoot

2) 静态模型（建筑/车）：glTF Sample Models (https://github.com/KhronosGroup/glTF-Sample-Models)
   - 例如："BoomBox", "Avocado" 等来做测试，或社区上传的车辆/建筑。

3) PBR 纹理与 HDRI：Poly Haven (https://polyhaven.com)
   - 下载一张环境 HDR（用于天空/反射）和部分高质量 PBR 贴图。

4) 小技巧：使用 Blender 批量将 FBX 转为 glTF（.glb）：
   - 打开 Blender -> File -> Import -> FBX
   - 选择模型后 File -> Export -> glTF 2.0 (.glb)
   - 导出时选择 "Embed textures" 以减少文件数量

放置约定：
- 把模型放到 `assets/models/`，贴图到 `assets/textures/`，HDR 放到 `assets/hdr/`。
- 示例入口文件名：`assets/scene.glb`（代码当前会尝试加载此文件）

许可注意：优先使用 CC0/CC-BY/教育许可的资源，并保存原始许可文件以备引用。