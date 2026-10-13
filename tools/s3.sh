#!/bin/sh
# ESP32-S3 from the terminal: no Arduino IDE needed. Run from the repo root.
#
#   sh tools/s3.sh setup             one-time: core, MQTT library, Python env
#   sh tools/s3.sh flash             build esp32s3_edge and upload it
#   sh tools/s3.sh monitor           open the console (Ctrl-C to leave)
#   sh tools/s3.sh test <stage>      run a parity stage on the S3, e.g. 4a, 4b, 5-neg
#   sh tools/s3.sh mqtt <broker-ip>  run the 27-step MQTT test (node s3-001)
#   sh tools/s3.sh haltest           flash the HAL test sketch and run it
#
# The port is found by the board's Espressif USB ID; set PORT=... to override.
# Wi-Fi details go in esp32s3_edge/wifi_credentials.h (copy the .example).
set -e
FQBN=esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=cdc,FlashSize=4M,PSRAM=enabled,PartitionScheme=huge_app
PY=.venv/bin/python
[ -x "$PY" ] || PY=python3
PORT=${PORT:-$($PY -c "from serial.tools import list_ports; print(next((p.device for p in list_ports.comports() if p.vid == 0x303A), ''))" 2>/dev/null)}

need_port() { [ -n "$PORT" ] || { echo "No ESP32-S3 found on USB. Plug it in (or set PORT=/dev/cu.usbmodemXXXX)."; exit 1; }; }
build() { arduino-cli compile --fqbn "$FQBN" --libraries libraries "$1"; }
upload() { need_port; arduino-cli upload --fqbn "$FQBN" --port "$PORT" "$1"; sleep 3; }

case "$1" in
  setup)
    arduino-cli config set board_manager.additional_urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
    arduino-cli core update-index
    arduino-cli core install esp32:esp32@3.3.12
    arduino-cli lib install "MQTT@2.5.3"
    python3 -m venv .venv && .venv/bin/pip install -q pyserial paho-mqtt
    [ -f esp32s3_edge/wifi_credentials.h ] || cp esp32s3_edge/wifi_credentials.h.example esp32s3_edge/wifi_credentials.h
    echo "Done. Edit esp32s3_edge/wifi_credentials.h, then: sh tools/s3.sh flash" ;;
  build)   build esp32s3_edge ;;
  flash)   build esp32s3_edge && upload esp32s3_edge ;;
  monitor) need_port; arduino-cli monitor --port "$PORT" --config baudrate=115200 ;;
  test)    need_port; $PY parity/parity_test.py --stage "${2:-4a}" --board s3 --port "$PORT" ;;
  mqtt)    need_port; [ -n "$2" ] || { echo "usage: sh tools/s3.sh mqtt <broker-ip>"; exit 1; }
           $PY parity/mqtt_test.py --broker "$2" --node "${NODE:-s3-001}" --board s3 --port "$PORT" ;;
  haltest) build esp32s3_haltest && upload esp32s3_haltest && $PY parity/hal_test.py --port "$PORT" ;;
  *)       sed -n '2,13p' "$0" ;;
esac
