import cv2
import numpy as np
import serial
import time

# 串口配置
SERIAL_PORT = "COM9"  # ⚠ 修改为你的串口号
BAUD_RATE = 115200

# 红点阈值（HSV）
red_lower = np.array([121, 0, 166])
red_upper = np.array([179, 255, 255])

# 白线阈值（HSV）
white_lower = np.array([85, 100, 61])
white_upper = np.array([147, 143, 255])

# 初始化串口
try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.5)
    time.sleep(2)
    print(f"[OK] 已连接串口 {SERIAL_PORT} @ {BAUD_RATE}bps")
except Exception as e:
    print(f"[ERROR] 打开串口失败: {e}")
    exit(1)

# 打开摄像头
cap = cv2.VideoCapture(1)
if not cap.isOpened():
    print("[ERROR] 无法打开摄像头")
    ser.close()
    exit(1)

while True:
    ret, frame = cap.read()
    if not ret:
        print("[WARN] 摄像头读取失败")
        break

    hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)

    # 红点检测
    mask_red = cv2.inRange(hsv, red_lower, red_upper)
    contours_red, _ = cv2.findContours(mask_red, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

    # 白线检测
    mask_white = cv2.inRange(hsv, white_lower, white_upper)
    contours_white, _ = cv2.findContours(mask_white, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

    red_center = None
    white_center = None

    if contours_red:
        c = max(contours_red, key=cv2.contourArea)
        (x, y, w, h) = cv2.boundingRect(c)
        red_center = (x + w // 2, y + h // 2)
        cv2.circle(frame, red_center, 5, (0, 0, 255), -1)

    if contours_white:
        c = max(contours_white, key=cv2.contourArea)
        (x, y, w, h) = cv2.boundingRect(c)
        white_center = (x + w // 2, y + h // 2)
        cv2.circle(frame, white_center, 5, (255, 255, 255), -1)

    # 计算向量并发送
    if red_center and white_center:
        dx = white_center[0] - red_center[0]  # 右为正
        dy = red_center[1] - white_center[1]  # 上为正（反转Y）

        frame_w = frame.shape[1]
        frame_h = frame.shape[0]

        # 归一化到 [-1, 1]，以半宽半高为单位
        dx_norm = dx / (frame_w / 2)
        dy_norm = dy / (frame_h / 2)

        dx_norm = max(-1.0, min(1.0, dx_norm))
        dy_norm = max(-1.0, min(1.0, dy_norm))

        # 发送整数，放大100倍
        msg = f"{int(dx_norm * 100)},{int(dy_norm * 100)}\n"
        ser.write(msg.encode())

        print(f"[TX] dx={dx}, dy={dy} (norm: {dx_norm:.2f}, {dy_norm:.2f})")

        # 画向量箭头
        cv2.arrowedLine(frame, red_center, white_center, (0, 255, 0), 2)

    # 窗口显示
    cv2.imshow("Tracking", frame)
    if cv2.waitKey(1) & 0xFF == 27:  # ESC 退出
        break

# 释放资源
cap.release()
cv2.destroyAllWindows()
ser.close()
