import serial
import time

# ------------------------------------------
# シリアルポートとボーレートの設定
# ------------------------------------------
SERIAL_PORT = 'COM6'      # Windows → COMポート、Mac/Linux → '/dev/ttyUSB0'
BAUD_RATE = 115200        # ESP32のボーレートに合わせる
# BAUD_RATE = 921600        # ESP32のボーレートに合わせる

# ------------------------------------------
# 送信するデータと回数
# ------------------------------------------
TEST_DATA = "0,0,0,0,0,100,100,0\n"  # 送信データ（改行付き）
SEND_COUNT = 100                     # 送信回数
SEND_INTERVAL = 0.05                 # 送信間隔（秒）

def send_serial_data():
    try:
        # シリアルポートを開く
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        time.sleep(2)  # 接続安定のため待機

        print(f"\nポート {SERIAL_PORT} に接続しました。送信開始します。\n")

        # データ送信ループ
        for i in range(SEND_COUNT):
            ser.write(TEST_DATA.encode('utf-8'))
            print(f"{i + 1}回目のデータを送信しました: {TEST_DATA.strip()}")
            time.sleep(SEND_INTERVAL)

        print("\n★ 100回の送信が完了しました。")

    except serial.SerialException:
        print(f"\nポート {SERIAL_PORT} に接続できません。ポート設定を確認してください。")

    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print("シリアルポートを閉じました。")

if __name__ == "__main__":
    send_serial_data()
