#!/usr/bin/env python3
"""
PC Audio → ESP-NOW Streaming Bridge

PCのオーディオ出力をキャプチャし、ESP32-S3のシリアルポートへ
バイナリストリームとして送信するスクリプト。
ESP32-S3側は STREAM_SOURCE_SERIAL モードで受信し、ESP-NOW経由で転送する。

使い方:
  1. pip install sounddevice numpy pyserial
  2. python stream_audio_to_serial.py --list     # デバイス一覧
  3. python stream_audio_to_serial.py --test --device 71  # 音が取れるか確認（シリアル不要）
  4. python stream_audio_to_serial.py --port COM5 --device 71  # ストリーミング開始

バイナリプロトコル (ESP32側 STREAM_SOURCE_SERIAL と対応):
  [0xBB] [numFrames_lo] [numFrames_hi] [L0_lo L0_hi R0_lo R0_hi] ...
"""

import argparse
import math
import sys
import struct
import time

import numpy as np
import serial
import sounddevice as sd

SYNC_BYTE = 0xBB
DEFAULT_SAMPLE_RATE = 8000
DEFAULT_FRAMES_PER_PACKET = 40
DEFAULT_BAUD = 921600


def _iter_devices():
    for i, dev in enumerate(sd.query_devices()):
        api_name = sd.query_hostapis(dev["hostapi"])["name"]
        yield i, dev, api_name


def list_devices(filtered: bool):
    print("Devices:")
    shown = 0
    for i, dev, api_name in _iter_devices():
        in_ch = dev["max_input_channels"]
        out_ch = dev["max_output_channels"]
        name = dev["name"]

        if filtered:
            is_wasapi = api_name == "Windows WASAPI"
            looks_like_loopback_src = (
                is_wasapi
                and in_ch >= 2
                and out_ch == 0
            )
            is_playback_device = is_wasapi and out_ch >= 2 and in_ch == 0

            if not (looks_like_loopback_src or is_playback_device):
                continue

        tag = ""
        if dev.get("max_input_channels", 0) >= 2 and dev.get("max_output_channels", 0) == 0:
            tag = " [INPUT/CAPTURE]"
        elif dev.get("max_output_channels", 0) >= 2 and dev.get("max_input_channels", 0) == 0:
            tag = " [OUTPUT - use with --loopback]"

        print(f"  [{i:>3}] {name}  (api={api_name}, in={in_ch}, out={out_ch}){tag}")
        shown += 1

    if filtered:
        print("")
        print("=== 使い方 ===")
        print("  [INPUT/CAPTURE] デバイス → そのまま --device ID で指定（--loopback 不要）")
        print("  [OUTPUT]  デバイス → --device ID --loopback で指定（再生中の音をキャプチャ）")
        print("")
        print("  まず --test で音が取れるか確認してください:")
        print("    python tools/stream_audio_to_serial.py --test --device <ID>")
        print("    python tools/stream_audio_to_serial.py --test --device <ID> --loopback")
        print("")
        if shown == 0:
            print("候補が出ない場合は --list-all で全件表示して選んでください。")


def list_scan():
    """全 WASAPI 入力デバイスを 3 秒ずつ開いてレベルをチェック"""
    print("=== Auto-scan: WASAPI input devices (3s each) ===")
    print("  音楽を再生しながら実行してください。\n")
    candidates = []
    for i, dev, api_name in _iter_devices():
        if api_name != "Windows WASAPI":
            continue
        in_ch = dev["max_input_channels"]
        if in_ch < 2:
            continue
        out_ch = dev["max_output_channels"]
        if out_ch > 0:
            continue
        candidates.append((i, dev["name"]))

    if not candidates:
        print("  WASAPI入力デバイスが見つかりません。")
        return

    for dev_id, name in candidates:
        peak = _quick_level_check(dev_id, duration=3.0)
        if peak > 0:
            db = 20.0 * math.log10(peak)
            bar = "#" * int(min(peak, 1.0) * 30)
            status = f"{db:5.1f} dBFS  [{bar}]"
        else:
            status = "  -inf dBFS  (無音)"
        print(f"  [{dev_id:>3}] {name}: {status}")

    print("\n  レベルが出ているデバイスIDを --device に指定してください。")


def _quick_level_check(device_id: int, duration: float = 3.0) -> float:
    """指定デバイスを短時間開いてピークレベルを返す"""
    peak = 0.0

    def cb(indata, frames, time_info, status):
        nonlocal peak
        p = float(np.max(np.abs(indata)))
        if p > peak:
            peak = p

    dev_info = sd.query_devices(device_id)
    rate = int(round(dev_info.get("default_samplerate", 48000)))
    try:
        with sd.InputStream(device=device_id, samplerate=rate, channels=2,
                            dtype="float32", callback=cb):
            time.sleep(duration)
    except Exception as e:
        print(f"  [{device_id:>3}] ERROR: {e}")
    return peak


def build_packet(frames: np.ndarray) -> bytes:
    num_frames = len(frames)
    header = struct.pack("<BH", SYNC_BYTE, num_frames)
    pcm = frames.astype(np.int16).tobytes()
    return header + pcm


def _resample_linear(stereo: np.ndarray, in_rate: int, out_rate: int) -> np.ndarray:
    if in_rate == out_rate:
        return stereo
    n = stereo.shape[0]
    if n <= 1:
        return np.zeros((0, 2), dtype=np.float32)

    m = int(round(n * (out_rate / in_rate)))
    if m <= 0:
        return np.zeros((0, 2), dtype=np.float32)

    x_old = np.linspace(0.0, 1.0, num=n, endpoint=False, dtype=np.float32)
    x_new = np.linspace(0.0, 1.0, num=m, endpoint=False, dtype=np.float32)
    out = np.empty((m, 2), dtype=np.float32)
    out[:, 0] = np.interp(x_new, x_old, stereo[:, 0]).astype(np.float32, copy=False)
    out[:, 1] = np.interp(x_new, x_old, stereo[:, 1]).astype(np.float32, copy=False)
    return out


def _level_bar(peak: float, width: int = 30, db_min: float = -60.0) -> str:
    if peak <= 0.0:
        db = db_min - 1.0
        db_str = " -inf"
    else:
        db = 20.0 * math.log10(peak)
        db_str = f"{db:5.1f}"
    ratio = max(0.0, min(1.0, (db - db_min) / (0.0 - db_min)))
    filled = int(ratio * width)
    bar = "#" * filled + "-" * (width - filled)
    return f"[{bar}] {db_str}dB"


def _make_wasapi_extra(loopback: bool):
    if not loopback:
        return None
    try:
        return sd.WasapiSettings(loopback=True)  # type: ignore
    except TypeError:
        pass
    try:
        return sd.WasapiSettings(exclusive=False)
    except Exception:
        pass
    return None


def _open_input_stream(device_id, rate, channels, dtype, callback, loopback):
    """InputStream を開く。loopback フラグも考慮する。"""
    extra = _make_wasapi_extra(loopback)

    try:
        return sd.InputStream(
            device=device_id, samplerate=rate, channels=channels,
            dtype=dtype, blocksize=0, callback=callback,
            extra_settings=extra)
    except sd.PortAudioError:
        if extra is not None:
            return sd.InputStream(
                device=device_id, samplerate=rate, channels=channels,
                dtype=dtype, blocksize=0, callback=callback)
        raise


def test_device(device_id, loopback: bool):
    """シリアルなしで音声入力レベルを確認するテストモード"""
    dev = sd.query_devices(device_id)
    name = dev["name"]
    in_ch = dev["max_input_channels"]
    out_ch = dev["max_output_channels"]
    rate = int(round(dev.get("default_samplerate", 48000)))

    print(f"=== Test Mode ===")
    print(f"Device [{device_id}]: {name}")
    print(f"  in={in_ch}, out={out_ch}, rate={rate}Hz, loopback={loopback}")

    if not loopback and in_ch == 0:
        print(f"\n  ERROR: このデバイスは入力チャンネルがありません。")
        print(f"  出力デバイスの再生音を取りたい場合は --loopback を付けてください。")
        return

    if loopback and out_ch == 0 and in_ch > 0:
        print(f"  NOTE: このデバイスは既に入力デバイスです。--loopback は不要かもしれません。")

    peak_l = 0.0
    peak_r = 0.0
    callback_count = 0

    def cb(indata, frames, time_info, status):
        nonlocal peak_l, peak_r, callback_count
        callback_count += 1
        if status:
            print(f"\n  [audio status] {status}", file=sys.stderr)
        stereo = indata[:, :2] if indata.shape[1] >= 2 else np.column_stack(
            [indata[:, 0], indata[:, 0]])
        pl = float(np.max(np.abs(stereo[:, 0])))
        pr = float(np.max(np.abs(stereo[:, 1])))
        if pl > peak_l:
            peak_l = pl
        if pr > peak_r:
            peak_r = pr

    print(f"\n  Listening... (Ctrl+C to stop)")
    print(f"  音楽やテスト音を再生してレベルが動くか確認してください。\n")

    try:
        stream = _open_input_stream(device_id, rate, 2, "float32", cb, loopback)
        with stream:
            while True:
                time.sleep(0.5)
                bar_l = _level_bar(peak_l)
                bar_r = _level_bar(peak_r)
                print(f"\r  cb={callback_count:>6}  L {bar_l}  R {bar_r}  ", end="", flush=True)
                peak_l = 0.0
                peak_r = 0.0
    except sd.PortAudioError as e:
        print(f"\n  ERROR: ストリームを開けませんでした: {e}")
        if loopback:
            print(f"  → このデバイスは --loopback に対応していない可能性があります。")
            print(f"  → --loopback なしで試すか、別のデバイスを選んでください。")
    except KeyboardInterrupt:
        print(f"\n\n  Test finished. callbacks={callback_count}")
        if callback_count == 0:
            print("  WARNING: コールバックが一度も呼ばれていません。デバイスに問題がある可能性があります。")


def stream(port: str, baud: int, device_id, sample_rate: int,
           frames_per_packet: int, loopback: bool):
    ser = serial.Serial(port, baud, timeout=1)
    print(f"Serial: {port} @ {baud}")
    print(f"Audio: device={device_id}, target={sample_rate} Hz stereo, "
          f"{frames_per_packet} frames/pkt")
    print("Streaming... (Ctrl+C to stop)")

    packets_sent = 0
    t_start = time.time()
    peak_l = 0.0
    peak_r = 0.0

    def _reset_peaks():
        nonlocal peak_l, peak_r
        peak_l = 0.0
        peak_r = 0.0

    dev = sd.query_devices(device_id)
    in_rate = int(round(dev.get("default_samplerate", sample_rate)))
    if in_rate <= 0:
        in_rate = sample_rate

    if in_rate != sample_rate:
        print(f"Audio: input_rate={in_rate} Hz (will resample -> {sample_rate} Hz)")

    pending = np.zeros((0, 2), dtype=np.float32)

    def audio_callback(indata, frame_count, time_info, status):
        nonlocal packets_sent, pending, peak_l, peak_r
        if status:
            print(f"[audio] {status}", file=sys.stderr)

        stereo = indata[:, :2] if indata.shape[1] >= 2 else np.column_stack(
            [indata[:, 0], indata[:, 0]])

        pl = float(np.max(np.abs(stereo[:, 0])))
        pr = float(np.max(np.abs(stereo[:, 1])))
        if pl > peak_l:
            peak_l = pl
        if pr > peak_r:
            peak_r = pr

        stereo = _resample_linear(stereo, in_rate=in_rate, out_rate=sample_rate)
        if stereo.size == 0:
            return

        if pending.size:
            stereo = np.vstack([pending, stereo])
            pending = np.zeros((0, 2), dtype=np.float32)

        pcm16 = np.clip(stereo * 32767.0, -32768, 32767).astype(np.int16, copy=False)

        i = 0
        total = len(pcm16)
        while i + frames_per_packet <= total:
            chunk = pcm16[i:i + frames_per_packet]
            if len(chunk) == 0:
                break
            pkt = build_packet(chunk)
            try:
                ser.write(pkt)
                packets_sent += 1
            except serial.SerialException:
                return
            i += frames_per_packet

        if i < total:
            pending = stereo[i:total].astype(np.float32, copy=False)

    try:
        try_rate = in_rate
        try:
            inp = _open_input_stream(device_id, try_rate, 2, "float32",
                                     audio_callback, loopback)
            with inp:
                _monitor_loop(t_start, lambda: packets_sent,
                              lambda: (peak_l, peak_r), _reset_peaks)
        except sd.PortAudioError as e:
            if try_rate != sample_rate:
                print(f"\n[audio] open failed at {try_rate}Hz, retry at {sample_rate}Hz: {e}",
                      file=sys.stderr)
                in_rate = sample_rate
                inp = _open_input_stream(device_id, sample_rate, 2, "float32",
                                         audio_callback, loopback)
                with inp:
                    _monitor_loop(t_start, lambda: packets_sent,
                                  lambda: (peak_l, peak_r), _reset_peaks)
            else:
                raise
    except KeyboardInterrupt:
        print("\nStopped.")
    finally:
        ser.close()


def _monitor_loop(t_start, get_packets, get_peaks, reset_peaks):
    while True:
        time.sleep(0.5)
        elapsed = time.time() - t_start
        pkt = get_packets()
        pps = pkt / elapsed if elapsed > 0 else 0
        pl, pr = get_peaks()
        bar_l = _level_bar(pl)
        bar_r = _level_bar(pr)
        reset_peaks()
        print(f"\r  pkts={pkt:>7}  {pps:5.1f}pkt/s  L {bar_l}  R {bar_r}  ",
              end="", flush=True)


def main():
    parser = argparse.ArgumentParser(
        description="PC Audio → ESP-NOW Streaming Bridge")
    parser.add_argument("--list", action="store_true",
                        help="候補デバイス一覧を表示")
    parser.add_argument("--list-all", action="store_true",
                        help="全デバイス一覧を表示")
    parser.add_argument("--scan", action="store_true",
                        help="全 WASAPI 入力デバイスを自動スキャンして音が出ているものを探す")
    parser.add_argument("--test", action="store_true",
                        help="シリアルなしで音声レベルを確認するテストモード")
    parser.add_argument("--port", type=str, default=None,
                        help="ESP32のシリアルポート (例: COM5, /dev/ttyACM0)")
    parser.add_argument("--baud", type=int, default=DEFAULT_BAUD,
                        help=f"ボーレート (default: {DEFAULT_BAUD})")
    parser.add_argument("--device", type=int, default=None,
                        help="オーディオデバイスID (--list で確認)")
    parser.add_argument("--loopback", action="store_true",
                        help="WASAPI loopbackで出力デバイスの再生音を取得（Windowsのみ）")
    parser.add_argument("--rate", type=int, default=DEFAULT_SAMPLE_RATE,
                        help=f"送信側へ流すサンプルレート (default: {DEFAULT_SAMPLE_RATE})")
    parser.add_argument("--frames", type=int, default=DEFAULT_FRAMES_PER_PACKET,
                        help=f"フレーム数/パケット (default: {DEFAULT_FRAMES_PER_PACKET})")
    args = parser.parse_args()

    if args.list_all:
        list_devices(filtered=False)
        return
    if args.list:
        list_devices(filtered=True)
        return
    if args.scan:
        list_scan()
        return

    if args.test:
        if args.device is None:
            parser.error("--test には --device が必要です")
        test_device(args.device, args.loopback)
        return

    if args.port is None:
        parser.error("--port を指定してください (まず --test でデバイスを確認)")

    stream(args.port, args.baud, args.device, args.rate, args.frames, args.loopback)


if __name__ == "__main__":
    main()
