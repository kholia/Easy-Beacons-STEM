REM https://downloads.arduino.cc/arduino-cli.exe/arduino-cli_latest_Windows_64bit.zip

arduino-cli.exe config init
arduino-cli.exe core update-index
arduino-cli.exe core install esp8266:esp8266
arduino-cli.exe core install rp2040:rp2040

arduino-cli.exe lib install "TinyGPSPlus"
arduino-cli.exe lib install "Etherkit JTEncode"
arduino-cli.exe lib install "Etherkit Si5351"
arduino-cli.exe lib install "RTClib"
arduino-cli.exe lib install "Time"
arduino-cli.exe lib install "Adafruit BusIO"

arduino-cli.exe config init
arduino-cli.exe core update-index
arduino-cli.exe core install arduino:avr
arduino-cli.exe lib install "Etherkit JTEncode"
arduino-cli.exe lib install "Etherkit Si5351"

arduino-cli compile --fqbn=rp2040:rp2040:rpipico Pico-FT8-Beacon-OnlyGPS

arduino-cli compile --fqbn=rp2040:rp2040:rpipicow Amplified-WSPR-Beacon-v2/firmware
