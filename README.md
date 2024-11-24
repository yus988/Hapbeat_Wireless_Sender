# Hapbeat Wireless Sender Firmware

Hapbeat Wireless Sender Firmwareは、M5Stackシリーズをベースにしたデバイス間通信やセンサーデータ送信をサポートするファームウェアです。本プロジェクトは、ESP32ベースのデバイスを使用し、ESP-NOWやMQTTプロトコルを利用してデータ通信を行います。

## 特徴

- **複数デバイス対応**: M5Stack CoreS3、Atom、Stampシリーズなど、複数のM5Stackデバイスに対応。
- **通信プロトコル**: ESP-NOW、MQTTをサポート。
- **センサー対応**: TCS34725カラーセンサーのサポート。
- **カスタマイズ可能**: ビルドフラグを利用して柔軟な機能設定が可能。

## 必要な環境

- **ハードウェア**: M5Stackシリーズ（例: M5Stack CoreS3、M5Atom）
- **ソフトウェア**: 
  - [PlatformIO](https://platformio.org/) IDE
  - Python 3.x（PlatformIOの依存関係）

## セットアップ手順

1. **PlatformIOのインストール**  
   [公式サイト](https://platformio.org/)を参照してPlatformIOをインストールしてください。

2. **リポジトリのクローン**  
   このリポジトリをローカル環境にクローンします：
   ```bash
   git clone https://github.com/yus988/Wireless_Sender.git
   cd Wireless_Sender

# 使用ライブラリとライセンス情報

このプロジェクトで使用している外部ライブラリと、そのライセンス情報は以下の通りです。各ライブラリの詳細なライセンス内容は、`licenses/` フォルダ内のファイルを参照してください。

| ライブラリ名                  | バージョン   | ライセンス       | ライセンスファイル名                     |
|---------------------------|-----------|---------------|-------------------------------------|
| M5Unified                | 0.1.14    | MITライセンス  | [M5Unified_0.1.14_LICENSE.md](./M5Unified_0.1.14_LICENSE.md) |
| Adafruit TCS34725         | 1.4.4     | BSDライセンス  | [Adafruit_TCS34725_1.4.4_LICENSE.md](./Adafruit_TCS34725_1.4.4_LICENSE.md) |
| MQTT (256dpi)             | 2.5.2     | MITライセンス  | [MQTT_2.5.2_LICENSE.md](./MQTT_2.5.2_LICENSE.md) |
| PubSubClient              | 2.8       | MITライセンス  | [PubSubClient_2.8_LICENSE.md](./PubSubClient_2.8_LICENSE.md) |
| ArduinoJson               | 7.1.0     | MITライセンス  | [ArduinoJson_7.1.0_LICENSE.md](./ArduinoJson_7.1.0_LICENSE.md) |
| FastLED                   | 3.7.0     | MITライセンス  | [FastLED_3.7.0_LICENSE.md](./FastLED_3.7.0_LICENSE.md) |

---

## 注意事項

- 各ライブラリのライセンス条件に従い、本プロジェクトを利用してください。
- このフォルダ内のライセンスファイルは、それぞれのライブラリの公式リポジトリから取得したものです。


   
