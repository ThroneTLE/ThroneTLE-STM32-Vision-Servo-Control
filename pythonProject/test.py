
import cv2
import numpy as np

# 创建窗口
cv2.namedWindow("Trackbars")

# 创建 HSV 阈值滑块
def nothing(x):
    pass

cv2.createTrackbar("H_low", "Trackbars", 0, 179, nothing)
cv2.createTrackbar("S_low", "Trackbars", 0, 255, nothing)
cv2.createTrackbar("V_low", "Trackbars", 0, 255, nothing)
cv2.createTrackbar("H_high", "Trackbars", 179, 179, nothing)
cv2.createTrackbar("S_high", "Trackbars", 255, 255, nothing)
cv2.createTrackbar("V_high", "Trackbars", 255, 255, nothing)

cap = cv2.VideoCapture(1)

while True:
    ret, frame = cap.read()
    if not ret:
        break

    hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)

    # 获取滑块值
    h_low = cv2.getTrackbarPos("H_low", "Trackbars")
    s_low = cv2.getTrackbarPos("S_low", "Trackbars")
    v_low = cv2.getTrackbarPos("V_low", "Trackbars")
    h_high = cv2.getTrackbarPos("H_high", "Trackbars")
    s_high = cv2.getTrackbarPos("S_high", "Trackbars")
    v_high = cv2.getTrackbarPos("V_high", "Trackbars")

    # 生成掩码
    lower = np.array([h_low, s_low, v_low])
    upper = np.array([h_high, s_high, v_high])
    mask = cv2.inRange(hsv, lower, upper)

    # 显示结果
    result = cv2.bitwise_and(frame, frame, mask=mask)

    cv2.imshow("Original", frame)
    cv2.imshow("Mask", mask)
    cv2.imshow("Filtered", result)

    if cv2.waitKey(1) & 0xFF == 27:  # ESC退出
        print(f"Lower: {lower}, Upper: {upper}")
        break

cap.release()
cv2.destroyAllWindows()
