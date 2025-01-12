import serial
import time

# シリアルポート設定（ESP32のポートを指定）
ser_send = serial.Serial('COM6', 115200)  # 送信側ESP32
ser_recv = serial.Serial('COM19', 115200)  # 受信側ESP32

# ★ 初期化（エラー対策）
send_time_pc = None
recv_time_pc = None

try:
    while True:
        # 送信側のデータ取得
        if ser_send.in_waiting:
            send_line = ser_send.readline().decode().strip()
            if "SendTime" in send_line:
                send_time_pc = time.perf_counter_ns()
                print(f"[PC受信] 送信時刻: {send_line}, PC受信時刻: {send_time_pc} ns")

        # 受信側のデータ取得
        if ser_recv.in_waiting:
            recv_line = ser_recv.readline().decode().strip()
            if "ReceiveTime" in recv_line:
                recv_time_pc = time.perf_counter_ns()
                print(f"[PC受信] 受信時刻: {recv_line}, PC受信時刻: {recv_time_pc} ns")

        # ★ 遅延時間の計算
        if send_time_pc is not None and recv_time_pc is not None:
            delay_time_ns = recv_time_pc - send_time_pc
            delay_time_ms = delay_time_ns / 1_000_000
            print(f"★ 測定遅延: {delay_time_ms:.6f} ms\n")

            # 初期化
            send_time_pc = None
            recv_time_pc = None

except KeyboardInterrupt:
    print("\n★ プログラムを終了します (Ctrl + C)")

finally:
    # ★ シリアルポートのクローズ処理
    if ser_send.is_open:
        ser_send.close()
        print(f"送信側ポート  を閉じました。")

    if ser_recv.is_open:
        ser_recv.close()
        print(f"受信側ポート  を閉じました。")

    print("★ 全てのポートを安全に閉じました。プログラムを終了します。")