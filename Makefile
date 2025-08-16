# https://github.com/arduino/arduino-cli/releases

# port := $(shell python3 board_detect.py)

TARGET?="/dev/ttyACM0"

default:
	@# echo $(port)
	# arduino-cli compile --fqbn=rp2040:rp2040:rpipico Pico-Beacons -e
	mkdir -p ~/.local/bin
	curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=~/.local/bin sh
	arduino-cli config init --overwrite
	arduino-cli core update-index
	arduino-cli core install rp2040:rp2040
	arduino-cli lib install "TinyGPSPlus"
	arduino-cli lib install "Etherkit JTEncode"
	arduino-cli lib install "Etherkit Si5351"
	arduino-cli lib install "RTClib"
	arduino-cli lib install "Time"
	arduino-cli lib install "Adafruit BusIO"
	arduino-cli compile --fqbn=rp2040:rp2040:rpipicow Amplified-WSPR-Beacon-v2/firmware -e
	arduino-cli upload --port=$(TARGET) --fqbn=rp2040:rp2040:rpipicow Amplified-WSPR-Beacon-v2/firmware

install_platform:
	arduino-cli config init --overwrite
	arduino-cli core update-index
	arduino-cli core install rp2040:rp2040
	arduino-cli core install esp8266:esp8266
	arduino-cli core install arduino:avr

deps:
	arduino-cli lib install "TinyGPSPlus"
	arduino-cli lib install "Etherkit JTEncode"
	arduino-cli lib install "Etherkit Si5351"
	arduino-cli lib install "RTClib"
	arduino-cli lib install "Time"
	arduino-cli lib install "Adafruit BusIO"

install_arduino_cli:
	mkdir -p ~/.local/bin
	curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=~/.local/bin sh
