from flask import Flask, request, jsonify, Response
import pyttsx3
import uuid
import os
from gtts import gTTS
import asyncio
import edge_tts

app = Flask(__name__)

# Load available pyttsx3 voices
engine = pyttsx3.init()
available_voices = {v.id: v.name for v in engine.getProperty('voices')}


@app.route('/voices', methods=['GET'])
def list_voices():
    """List available system voices (pyttsx3)."""
    return jsonify(available_voices)


@app.route('/tts', methods=['POST'])
def tts_stream():
    """Stream offline TTS using pyttsx3."""
    data = request.get_json(force=True)
    text = data.get('text')
    voice_id = data.get('voice')
    rate = data.get('rate')

    if not text:
        return jsonify({"error": "Missing 'text' parameter"}), 400

    filename = f"/tmp/{uuid.uuid4().hex}.wav"

    try:
        engine = pyttsx3.init()
        if voice_id:
            engine.setProperty('voice', voice_id)
        if rate:
            engine.setProperty('rate', rate)
        engine.save_to_file(text, filename)
        engine.runAndWait()
    except Exception as e:
        return jsonify({"error": str(e)}), 500

    def generate():
        with open(filename, "rb") as f:
            while chunk := f.read(4096):
                yield chunk
        os.remove(filename)

    return Response(generate(), mimetype='audio/wav')


@app.route('/gtts', methods=['POST'])
def gtts_stream():
    """Stream Google gTTS result."""
    data = request.get_json(force=True)
    text = data.get('text')
    lang = data.get('lang', 'en')

    if not text:
        return jsonify({"error": "Missing 'text' parameter"}), 400

    filename = f"/tmp/{uuid.uuid4().hex}.mp3"

    try:
        tts = gTTS(text=text, lang=lang)
        tts.save(filename)
    except Exception as e:
        return jsonify({"error": str(e)}), 500

    def generate():
        with open(filename, "rb") as f:
            while chunk := f.read(4096):
                yield chunk
        os.remove(filename)

    return Response(generate(), mimetype='audio/mpeg')


@app.route('/edge_tts', methods=['POST'])
def edge_tts_stream():
    """Stream TTS using Microsoft Edge Neural voices."""
    data = request.get_json(force=True)
    text = data.get('text')
    voice = data.get('voice', 'en-US-AriaNeural')
    rate = str(data.get('rate')) if 'rate' in data else None
    pitch = str(data.get('pitch')) if 'pitch' in data else None

    if not text:
        return jsonify({"error": "Missing 'text' parameter"}), 400

    filename = f"/tmp/{uuid.uuid4().hex}.mp3"

    async def generate_tts():
        options = {"text": text, "voice": voice}
        if rate and rate not in ["0%", "+0%", "-0%", "0", ""]:
            options["rate"] = rate
        if pitch and pitch not in ["0%", "+0%", "-0%", "0", ""]:
            options["pitch"] = pitch

        try:
            communicate = edge_tts.Communicate(**options)
            await communicate.save(filename)
        except Exception as e:
            if "pitch" in str(e).lower() or "rate" in str(e).lower():
                communicate = edge_tts.Communicate(text=text, voice=voice)
                await communicate.save(filename)
            else:
                raise e

    try:
        asyncio.run(generate_tts())
    except Exception as e:
        return jsonify({"error": str(e)}), 500

    def generate():
        with open(filename, "rb") as f:
            while chunk := f.read(4096):
                yield chunk
        os.remove(filename)

    return Response(generate(), mimetype='audio/mpeg')


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, threaded=True)
# This code provides a Flask application that serves as a TTS server.
# It supports both offline TTS using pyttsx3 and online TTS using Google gTTS and Microsoft Edge TTS.
# The server can handle requests to list available voices, stream TTS audio, and supports various parameters like voice ID, rate, and pitch.
# The audio is streamed in chunks to the client, and temporary files are cleaned up after use