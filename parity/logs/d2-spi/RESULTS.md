# D2 SPI checkpoint results — 2026-10-16

Hardware: ESP32-S3 Super Mini / HW-747 on COM5, GPIO11 MOSI connected directly to GPIO13 MISO. The LCD and mmWave module were disconnected. SPI host/pins were `fspi`, SCLK 12, MOSI 11, MISO 13, and CS 10.

Toolchain: Arduino CLI 1.5.1, Arduino-ESP32 3.3.12, MQTT 2.5.3. Firmware used the Huge APP partition and QSPI PSRAM settings recorded in `tools/s3.sh`.

Results:

- `sh tools/check_hal_boundary.sh`: pass, including `bus/spi.c`.
- Pico 2 W shared-source build: pass; `bus/spi.c` compiled and `project_v3.elf` linked through the Pico unsupported stubs.
- Production firmware: 1,339,190 bytes flash; 177,940 bytes global RAM.
- Upload: pass; flash hash verified.
- `sh tools/s3.sh spi`: 22/22 pass.
- SPI modes 0, 1, 2, and 3: exact loopback pass at 10 MHz.
- Scheduled transfers: exact 2-byte and 128-byte loopback pass.
- Invalid host, stream mode, mode 4, reserved/duplicate pins, missing MISO, RX length 129, zero timeout, and empty transfer: rejected.
- Compatible two-device bus sharing: pass; incompatible host-pin reuse rejected; existing devices remained operational.
- 100 initialize/CLEAR cycles: pass.
- Free heap: 95,748 B before and 95,748 B after the lifecycle test.
- D1 stage `4a`: 14/14 pass.
- D1 stage `5-neg`: 27/27 pass after replacing the formerly unsupported `spi` case with `bogus`.

Raw serial evidence is in `spi.log`. Remaining D2 evidence gates are the dedicated 8–1024-byte performance/integrity harness, numerical latency/throughput results, concurrent workload, and the separate TFT TX proof.
