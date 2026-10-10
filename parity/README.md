# Parity test harness

The harness sends the same REPL commands to the Pico reference and the ESP32-S3 port and retains timestamped raw serial evidence. A stage passing is evidence only for the checks listed in that stage; it does not complete a checklist task by itself.

## Host setup

Use Python 3 and install the only harness dependency in a virtual environment:

```sh
python3 -m venv .venv
. .venv/bin/activate
python -m pip install -r parity/requirements.txt
```

Do not commit `.venv`, Wi-Fi credentials or broker credentials.

## Running a stage

```sh
python3 parity/parity_test.py --stage 4a --port /dev/cu.usbmodemXXXX
```

Omit `--port` to select the first macOS or Linux USB serial candidate. Use `--boot` when the stage must also capture startup, Wi-Fi and base-model messages. Each run replaces `parity/logs/<stage>.log`; copy or commit accepted evidence before rerunning a stage whose existing log must be retained.

| Stage | Required setup | Coverage |
|---|---|---|
| `4a` | Board only | REPL, inline config, rules and built-in inference |
| `4b` | Jumper from output GP21 to input GP22 | GPIO output/input, toggle, pulse, counter and PWM command |
| `4c-uart` | Jumper from GP16 TX to GP17 RX | UART request/response loopback |
| `4c-sonic` | HC-SR04: Trig GP16, Echo GP17 through a verified divider/level shifter, 5 V supply and common ground | Pulse width and timeout |
| `4c-i2c` | TSL2561: SDA GP16, SCL GP17, correct supply and ground | Identity and light registers |
| `4c-onewire` | DHT22/AM2302 data GP22 with pull-up, correct supply and ground | Checksum-valid humidity/temperature frame |
| `5-neg` | Board only; no device on the tested I2C address | Invalid input and capacity limits |

The GPIO numbers above are for the Pico reference only. Task 8 supplies non-conflicting ESP32-S3 pins; update a copied S3 command set or add pin parameters before using these hardware stages on the port.

## Evidence rule

Record the firmware commit, board/module revision, tool versions, tester and date alongside every accepted log. A missing device, timeout-only response, console message without an electrical measurement, or loopback is labelled exactly as such in `PARITY.md`; it is not evidence that a real sensor or waveform works.
