# 🎯 Vision-Guided Dual Servo Control

基于 OpenCV 的颜色追踪与 STM32 双舵机控制系统，通过摄像头识别红点与白线位置，实时计算相对误差并通过串口发送给 STM32，驱动舵机实现自动跟踪与指向。

---

## 📦 功能概述
- **红点检测**：基于 HSV 阈值识别目标红色标记点。
- **白线检测**：识别目标白色线段中心。
- **标准数学坐标系误差计算**：以红点为原点，右为正 X，上为正 Y，返回归一化误差 `[-1, 1]`。
- **STM32 舵机 PID 控制**：接收误差数据，实时调整底座和机械臂角度。
- **工作区限制**：仅允许舵机在设定的物理范围内运动，避免机械冲撞。
- **调试工具**：提供 HSV 滑块调节程序（`test.py`）快速确定颜色阈值。

---

## 🗂 文件结构
.
├── main.py # 摄像头+串口，计算并发送归一化坐标误差
├── test.py # HSV 阈值调试工具
├── main.c # STM32 主循环
├── servo_app.c/h # 舵机控制逻辑、PID、误差解析
├── servo_interp.c/h # 舵机插值与平滑控制
└── README.md # 项目说明


复制
编辑

---

## ⚙️ 硬件需求
- STM32 系列开发板（支持 HAL 库）
- 两个舵机（底座 + 机械臂）
- USB 摄像头
- 电脑（运行 Python + OpenCV）
- 3D 打印支架 / 固定结构（可选）

---

## 🖥 软件需求
### Python 端
- Python 3.8+
- OpenCV
- NumPy
- PySerial

安装依赖：
pip install opencv-python numpy pyserial
STM32 端
STM32CubeMX + HAL 库

Keil / STM32CubeIDE 编译工具

🚀 使用方法
1️⃣ 运行 HSV 调试工具（可选）

python test.py
调节滑块，获取红点与白线的 HSV 阈值，并更新到 main.py。

2️⃣ 启动主程序（PC 端）

python main.py
摄像头画面中检测到红点与白线，会实时计算误差并通过串口发送。

数据格式：
dx_int,dy_int\n
其中 dx_int 和 dy_int 为放大 100 倍的整数误差。

3️⃣ STM32 接收与舵机控制
servo_app.c 中 parse_vector() 解析串口接收的误差。

update_servo_from_vector() 基于 PID 算法调整舵机位置。

工作区限制保证舵机不超过物理允许范围。

📊 坐标系说明
原点：红点位置。

X 轴：向右为正。

Y 轴：向上为正（图像坐标需反转 Y）。

归一化：以画面半宽、半高为单位，误差范围 [-1.0, 1.0]。

🔌 串口通信协议图

Python(OpenCV) → Serial → STM32

[图像处理]   计算红点-白线误差
     │
     ▼
 "dx_int,dy_int\n"   // 整数，放大100倍
     │
     ▼
 STM32 HAL_UART_RxCpltCallback()
     │
     ▼
 parse_vector() → g_dx / g_dy
     │
     ▼
 update_servo_from_vector() → PID → 舵机控制
📐 舵机工作区示意图
text
复制
编辑
底座(base)：       ARM(机械臂)：
  75°  ─────────────  0°   ← 垂直入射
        工作区          \
        90° 中心          \   工作区
 105° ─────────────  45°  ← 平行于平面
🛠 PID 参数调试
STM32 端的 pid_base 和 pid_arm 分别控制底座与机械臂：



pid_t pid_base = { .kp = -1.0f, .ki = 0.0f, .kd = 1.0f, .max_speed = 2.0f };
pid_t pid_arm  = { .kp =  1.0f, .ki = 0.0f, .kd = 1.0f, .max_speed = 2.0f };
调试建议：

先调 P，让系统能大致跟踪。

再加 D，抑制震荡。

必要时加 I，修正静态误差。

📷 运行效果
红点与白线被标记圆圈。

绿色箭头显示实时向量方向。

串口打印发送的数据，STM32 串口可输出调试信息。

📄 许可证
MIT License — 可自由使用、修改与分发。




---

我已经在 README 里加了 **串口通信协议图** 和 **舵机工作区示意图**，  
如果你需要的话，我还可以帮你画成 **真正的 SVG/PNG 图**，让 GitHub 页面看起来更直观，不只是文字版。  

你要我帮你画成可视化的协议图和工作区图吗？这样你的项目看起来会更专业。

