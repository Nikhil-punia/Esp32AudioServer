# ESP32_Arduino_ESPIDF
Forked From https://github.com/schreibfaul1/ESP32_Arduino_ESPIDF.git

What is this Repo :
- This repo contains code for a audio server based on esp32 chip (currently config is s3 n8r8).
- There is functionality available at various http endpoint for connecting to :
   - Http streaming host
   - google tts
   - local tts (a method is required to be implemented in the audioI2s library for that , commented in the audio_util.cpp also the main.py should be running).
   - `http://<ip>/home` is the end point for web server ui (currently in development)
   - `http://<ip>/?link=` ... is the endpoint of connecting to host.
   - `http://<ip>/?bass=8&mid=8&tr=8` ... is the endpoint of set tone method.
- For TTS
  - `http://<ip>/speech` is the end point for Google TTS built in the audioI2S lib . {data for the POST request must be json containg field text}
  -  `http://<ip>/lspeech` is the end point for Local TTS for which main.py is required data for POST request is like `{"text":"Hello World","voice_id":"te-IN-ShrutiNeural"}`  NOTE : this method is  in developing stage .
 

There is often a desire to change values in the sdkconfig. This is not possible with the preconfigured Arduino framework.
Clone this project with PlatformIO, open a terminal and type: ``pio run -t menuconfig`` <br><br>
![image](https://user-images.githubusercontent.com/26044260/205142784-18e32ffe-2aef-4263-a295-6111025d29f7.png)
