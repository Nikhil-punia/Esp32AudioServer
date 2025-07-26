import requests
import time
import psutil
import platform
from datetime import datetime
import io
import wave
import pyaudio
from pydub import AudioSegment
import threading

def get_system_info():
    now = datetime.now()
    # Natural language time and date
    time_str = now.strftime("%I:%M %p").lstrip("0")
    date_str = now.strftime("%B %d, %Y")
    cpu_load = psutil.cpu_percent(interval=1)
    temp_str = "N/A"
    if hasattr(psutil, "sensors_temperatures"):
        try:
            temps = psutil.sensors_temperatures()
            for key in ['coretemp', 'cpu-thermal', 'cpu_thermal', 'acpitz']:
                if key in temps and temps[key]:
                    temp_str = temps[key][0].current
                    break
        except Exception:
            pass
    return time_str, date_str, temp_str, cpu_load

def play_wav_bytes(wav_bytes):
    with io.BytesIO(wav_bytes) as wav_io:
        with wave.open(wav_io, 'rb') as wf:
            p = pyaudio.PyAudio()
            stream = p.open(format=p.get_format_from_width(wf.getsampwidth()),
                            channels=wf.getnchannels(),
                            rate=wf.getframerate(),
                            output=True)
            data = wf.readframes(1024)
            while data:
                stream.write(data)
                data = wf.readframes(1024)
            stream.stop_stream()
            stream.close()
            p.terminate()

def play_mp3_bytes(mp3_bytes):
    audio = AudioSegment.from_file(io.BytesIO(mp3_bytes), format="mp3")
    p = pyaudio.PyAudio()
    stream = p.open(format=p.get_format_from_width(audio.sample_width),
                    channels=audio.channels,
                    rate=audio.frame_rate,
                    output=True)
    # Play the raw PCM data
    stream.write(audio.raw_data)
    stream.stop_stream()
    stream.close()
    p.terminate()

def tts_and_play(text, voice, url):
    payload = {
        "text": text,
        "voice": voice
    }
    try:
        print(f"Sending: {text}")
        response1 = requests.post("http://100.87.229.208:5000/termux_tts", json=payload, timeout=30)
        print(f"Status: {response1.status_code}")
        if response1.ok:
            print("TTS audio sent.")
        else:
            print("Error:", response1.text)
        response = requests.post(url, json=payload, timeout=30)
        print(f"Status: {response.status_code}")
        if response.ok:
            print("TTS audio received.")
            content_type = response.headers.get("Content-Type", "")
            if "wav" in content_type:
                play_wav_bytes(response.content)
            elif "mp3" in content_type or "audio/mpeg" in content_type:
                play_mp3_bytes(response.content)
            else:
                print("Unknown audio format:", content_type)
        else:
            print("Error:", response.text)
    except Exception as e:
        print("Request failed:", e)


def main():
    url = "http://100.87.229.208:5000/piper"
    voice = "en_US-amy-medium"

    while True:
        time_str, date_str, temp, cpu = get_system_info()
        text = (
            f"The time is {time_str} on {date_str}. "
            f"CPU temperature is {temp} degrees Celsius. "
            f"CPU load is {cpu} percent."
        )
        # Start a new thread for TTS and playback
        t = threading.Thread(target=tts_and_play, args=(text, voice, url))
        t.start()
        # Wait for the next interval
        time.sleep(30)

if __name__ == "__main__":
    main()