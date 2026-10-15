# D2 — ESP32-S3 SPI Support To-Do List

- Branch: `s3-port-d2`
- Updated baseline: D1 `s3-port` at `ba81f18`, merged into D2 by `9a421ef`
- Team: Brendan and Jy
- Status: ready for implementation

## End outcome

SPI is a first-class runtime-configurable interface behind the same `app_cfg_t`, `bus_t`, registry, scheduler, REPL, and publisher path used by GPIO, I2C, and UART. `proto=spi` must resolve through `bus_lookup("spi")`; shared application code must not call Arduino or ESP-IDF SPI APIs directly.

The ESP32-S3 implementation must support validated runtime host, SCLK, MOSI, optional MISO, CS, clock, mode, TX bytes, RX length, and timeout. Failed configuration, initialization, transfer, timeout, or verification must be explicit and must never be published as valid sensor data.

## What D1 already completed

These are no longer D2 blockers or implementation tasks:

- [x] The shared sources are one Arduino library at `libraries/EdgeCore/src/`, also compiled by the Pico CMake build.
- [x] The ESP32-S3 platform layer is `libraries/EdgeS3/src/`; the production sketch is `esp32s3_edge/esp32s3_edge.ino`.
- [x] The registry and driver contract exist in `libraries/EdgeCore/src/bus/bus_if.h` and `bus_if.c`.
- [x] The scheduler owns bus calls from EngineTask; drivers do not mutate scheduler state from callbacks.
- [x] Invalid protocols are rejected through `app_cfg_t.cfg_error` instead of falling back to I2C.
- [x] The final D1 board, toolchain, USB, PSRAM, pin rules, test scripts, and evidence locations are documented.
- [x] D1 GPIO, I2C, UART, rules, MQTT, model storage/inference, fault, timing, and endurance tests have retained S3 evidence.
- [x] D2 requirements FR-16, FR-17, NFR-08 and tests T-17 to T-23 already exist in `parity/PARITY.md` as proposed rows.

Task 7 still requires every teammate to run the environment check, and D1 release task 23 remains pending, but neither blocks D2 implementation on this branch.

## Actual D1 integration points

| Concern | File(s) D2 must change |
|---|---|
| Protocol enum and runtime fields | `libraries/EdgeCore/src/bus/bus_common.h` |
| Parser and validation | `libraries/EdgeCore/src/application/config_parser.c` |
| Defaults, `SHOW`, and `CLEAR` | `libraries/EdgeCore/src/application/repl.c`, `scheduler.c` |
| Common driver contract and registry | `libraries/EdgeCore/src/bus/bus_if.h`, `bus_if.c` |
| Portable SPI sensor driver | new `libraries/EdgeCore/src/bus/spi.c` |
| Portable platform contract | `libraries/EdgeCore/src/hal/hal.h` |
| ESP32-S3 backend | `libraries/EdgeS3/src/hal_esp32s3.cpp` or a tightly scoped EdgeS3 SPI source |
| Pico compatibility | `project_v3/platform/pico/hal_pico.c`, root `CMakeLists.txt` |
| S3 build/test entry points | `tools/s3.sh`, new `parity/spi_test.py`, optional `esp32s3_spitest/` |
| Requirements and user documentation | `parity/PARITY.md`, `README.md`, `SETUP.md`, `ARCHITECTURE.md`, `project_v3/README.md` |

## Critical design corrections from the completed D1 code

- [ ] Add an explicit `deinit(void *ctx)` callback to `bus_t`. `scheduler_reset()` currently calls `free(ctx)` directly, which cannot release an SPI device, shared bus reference, lock, or platform handle safely.
- [ ] Update GPIO, I2C, and UART `bus_t` initializers with a compatible `deinit` entry (`NULL` where plain `free()` remains sufficient).
- [ ] Make `scheduler_reset()` stop streaming, call `bus->deinit(ctx)` when present, and only use `free(ctx)` as the compatibility fallback.
- [ ] On `init_cfg()` failure, clean up any partially returned context before releasing the scheduler slot.
- [ ] Add `proto_spi` explicitly to `proto_name()`. The current ternary mapping defaults every unrecognised enum to `gpio`, which is unsafe for future protocols.
- [ ] Do not put a 1024-byte automatic buffer on EngineTask's stack. The current scheduler sample buffer is 128 bytes and D1 recorded task stack headroom. Keep normal scheduled SPI samples bounded (recommended: 128 bytes) and run 8–1024-byte integrity/performance measurements through a dedicated test path using the same HAL/backend transaction implementation.
- [ ] Keep configured TX bytes in a bounded array inside `app_cfg_t`; never retain pointers into the parser's temporary 160/256-byte buffers.
- [ ] Treat TFT commands, offsets, rotation, colour order, inversion, and controller initialization as a device driver above the generic SPI transport.

## Definition of done

- [ ] `proto=spi` is parsed, validated, displayed by `SHOW`, scheduled, and resolved through `bus_lookup("spi")`.
- [ ] Runtime configuration supports host, SCLK, MOSI, optional MISO, CS, clock, mode, bounded TX bytes, bounded scheduled RX length, and timeout.
- [ ] Invalid settings are rejected before hardware state changes.
- [ ] The generic EdgeCore driver uses only the HAL contract; Arduino/ESP-IDF headers remain below the platform boundary.
- [ ] Compatible devices share one physical SPI host safely; incompatible host/pin reuse returns a clear conflict.
- [ ] `CLEAR`, failed initialization, and repeated reconfiguration release all contexts and platform resources.
- [ ] TX-only and TX/RX paths have explicit success and typed error semantics.
- [ ] MOSI-to-MISO loopback or a readable device proves byte-for-byte RX integrity.
- [ ] A TFT or another real SPI device proves useful TX operation through the same transport layer.
- [ ] Payloads from 8 through 1024 bytes have retained latency, throughput, timeout/error, and mismatch results.
- [ ] D1 boundary, Pico build, S3 build, HAL, REPL, negative, sensor, inference, and concurrency regressions pass.
- [ ] Evidence records commit, board, tool versions, pins, clock/mode, tester, and date.

## Confirmed hardware context

D1 final allocations are I2C GPIO1/2, HC-SR04 GPIO4/7, digital sensor GPIO8, PWM GPIO5, UART GPIO43/44, and native USB GPIO19/20. Strapping pins GPIO0/3/45/46 and flash/PSRAM pins are unavailable.

Use two separate D2 fixtures:

1. **RX/integrity fixture:** FSPI/SPI2, SCLK GPIO12, MOSI GPIO11, MISO GPIO13, CS GPIO10; connect MOSI to MISO for loopback or use a readable 3.3 V SPI device.
2. **TFT TX fixture:** SCLK GPIO12, MOSI GPIO11, CS GPIO10, DC GPIO9, reset GPIO8, and the confirmed backlight connection. This fixture has no MISO. GPIO8 conflicts with the D1 digital-sensor allocation and GPIO13 was used as TFT backlight in the standalone experiment, so do not run those conflicting devices simultaneously.

- [ ] Confirm the TFT controller from a rear marking or supplier listing: ST7789P3 versus NV3023B.
- [ ] Confirm whether the breakout backlight input is logic/PWM compatible before driving it from a GPIO.
- [ ] Photograph and record each fixture before testing.

The TFT alone cannot prove receive integrity because it exposes no SDO/MISO acknowledgement path.

## Teammate split

Keep one owner per active file to reduce conflicts.

### Brendan — shared contract, parser, scheduler, and integration

- [ ] Add `proto_spi` and bounded SPI configuration fields to `app_cfg_t`.
- [ ] Add parser defaults, parsing, validation, `SHOW`, and scheduler SPI summary.
- [ ] Extend `bus_t` lifecycle semantics and update existing drivers without changing their behaviour.
- [ ] Add `bus_spi` to the common registry and update Pico/S3 build manifests.
- [ ] Extend parity tests and keep README, architecture, configuration, and requirements synchronized.

### Jy — ESP32-S3 transport, fixtures, and evidence

- [ ] Implement the EdgeS3 SPI host/device manager, locking, transactions, timeout/error translation, and cleanup.
- [ ] Build the loopback/readable-device fixture and the separate TFT/real-device fixture.
- [ ] Implement the 8–1024-byte benchmark/test path using the same platform transaction code as `bus_spi`.
- [ ] Capture raw logs, heap measurements, build sizes, photos/video, and hardware metadata.

### Pair gates

- [ ] Approve the configuration schema, error values, scheduled transfer limit, and backend choice before coding.
- [ ] Review `bus_t`, HAL, and scheduler lifecycle changes together.
- [ ] Reproduce the main loopback test on the same commit before merge.

## Phase 1 — freeze the minimum contract

- [ ] Use `fspi` as the canonical ESP32-S3 host name; optionally accept `spi2` as a documented alias.
- [ ] First release supports poll mode only. Reject `mode=stream` for SPI with `unsupported` until queued streaming is deliberately implemented.
- [ ] Define shared negative error codes for invalid configuration, unsupported operation, conflict/busy, timeout, I/O failure, verification failure, no memory, and absent device.
- [ ] Define `poll_once()` semantics: positive RX byte count publishes data; zero is successful TX-only with no published sample; negative is an explicit error.
- [ ] Set normal scheduled limits (recommended `spi.tx_len <= 64`, `spi.read_len <= 128`) to match parser and scheduler constraints.
- [ ] Keep the 1024-byte benchmark outside normal publishing so console/MQTT line limits and EngineTask stack are not silently exceeded.
- [ ] Obtain team sign-off for FR-16, FR-17, NFR-08, and T-17 to T-23 before final merge.

Minimum configuration:

```ini
BEGINCFG|name=SPI_LOOP|proto=spi|mode=poll|freq_hz=2|spi.host=fspi|spi.sclk=12|spi.mosi=11|spi.miso=13|spi.cs=10|spi.hz=10000000|spi.mode=0|spi.tx=0xA5 0x5A|spi.read_len=2|spi.timeout_ms=100|ENDCFG
```

Validation checklist:

- [ ] Host is supported.
- [ ] Mode is 0–3 and transfer bit order is explicitly fixed/documented.
- [ ] Clock is within an approved minimum/maximum.
- [ ] Pins pass `hal_gpio_valid()`, are not reserved, and are unique where required.
- [ ] MISO is configured whenever RX bytes are requested.
- [ ] CS is valid; no-CS operation is rejected unless deliberately added.
- [ ] TX/RX lengths and timeout are bounded.
- [ ] A device joining an existing host matches its SCLK/MOSI/MISO configuration.
- [ ] TFT-only fields never enter the generic transport configuration.

## Phase 2 — shared EdgeCore changes

- [ ] Add `proto_spi` without renumbering existing protocol values.
- [ ] Add fixed-size SPI fields and defaults to `app_cfg_t` and `repl_init_defaults()`.
- [ ] Parse all agreed `spi.*` keys and detect malformed, truncated, or excessive byte lists.
- [ ] Perform complete SPI validation in `cfg_parse_end()` before `scheduler_add_config()` changes hardware.
- [ ] Replace the scheduler's default-to-GPIO `proto_name()` logic with an exhaustive mapping that can return `NULL` for invalid values.
- [ ] Add `deinit` to `bus_t`, update all existing drivers, and correct scheduler cleanup/rollback.
- [ ] Add `bus_spi` to `bus_if.h`, `bus_if.c`, and new `bus/spi.c`.
- [ ] Add a SPI-specific scheduler summary and make `SHOW` display all active SPI settings safely.
- [ ] Add new shared sources to root `CMakeLists.txt` so the Pico build continues compiling the identical EdgeCore code.

## Phase 3 — HAL and ESP32-S3 backend

- [ ] Define portable SPI open/transfer/close calls and typed errors in `libraries/EdgeCore/src/hal/hal.h`.
- [ ] Add Pico stubs in `project_v3/platform/pico/hal_pico.c` that return `HAL_ERR_UNSUPPORTED`; D2 has no Pico hardware requirement, but the shared build must still link.
- [ ] Implement the S3 backend under EdgeS3 using Arduino `SPIClass` first.
- [ ] Use direct ESP-IDF `spi_master` only if measured requirements for timeout, sharing, queueing, DMA, or errors cannot be satisfied through `SPIClass`; document the evidence and exception.
- [ ] Maintain one host record per bus/pin combination and one device record per CS/frequency/mode.
- [ ] Serialize transactions because EngineTask is the normal owner, while the dedicated benchmark must not race it.
- [ ] Reject incompatible host reuse instead of silently reconfiguring an active bus.
- [ ] Publish RX data only after a complete successful transfer.
- [ ] Release device, lock, and host resources after the final reference closes.
- [ ] Record DMA, alignment, maximum transfer, heap, and clock limitations.

## Phase 4 — automated functional and fault tests

- [ ] Add `parity/spi_test.py` and a `sh tools/s3.sh spi` entry point.
- [ ] T-17: configure SPI through the REPL, confirm `CFG: ok`, `SHOW`, scheduler summary, and `bus_spi` execution.
- [ ] T-18: validate host/pin/clock/mode changes, compatible bus sharing, and incompatible reuse refusal.
- [ ] T-19: verify deterministic loopback/readable-device payloads at 8, 16, 32, 64, 128, 256, 512, and 1024 bytes.
- [ ] T-20: operate the confirmed TFT or another real SPI device through the generic transport plus its device layer.
- [ ] T-21: reject invalid host, mode, clock, reserved/duplicate/conflicting pins, missing MISO, excessive lengths, timeout, and disconnected/absent device; publish no valid sample.
- [ ] T-22: run 100 initialize/deinitialize cycles with no progressive heap loss; failed reconfiguration must leave an existing device usable.
- [ ] T-23: benchmark and run SPI concurrently with D1 GPIO/I2C plus inference, with no reset or unexplained corruption.
- [ ] Add deliberate mismatch/corruption detection to the loopback test rather than treating received zeros as success.

## Phase 5 — benchmark and retained evidence

- [ ] Run at least 10 warm-ups and 1,000 measured transactions for selected payload/clock combinations.
- [ ] Record median, p95, p99, maximum latency, and effective payload throughput.
- [ ] Record attempts, platform errors, timeouts, mismatched transactions, mismatched bytes, resets, and heap before/after.
- [ ] Report transaction error rate and byte error rate separately.
- [ ] Store raw/CSV logs and a concise result summary under `parity/logs/d2-spi/`.
- [ ] Record commit, ESP32-S3FH4R2/HW-747 board, Arduino CLI/IDE, core 3.3.12, ESP-IDF version, FQBN, CPU frequency, host, pins, mode, clock, backend, tester, and date.

## Phase 6 — exact D1 regression gate

- [ ] Run `sh tools/check_hal_boundary.sh`.
- [ ] Run `sh tools/s3.sh check` to compile the bring-up and production sketches with the pinned environment.
- [ ] Rebuild the Pico target from the shared sources; `bus_spi` must link through unsupported Pico HAL stubs.
- [ ] Run `sh tools/s3.sh haltest`.
- [ ] Run S3 stages `4a`, `4b`, `4c-mag`, `4c-uart`, and `5-neg`.
- [ ] Run the new `sh tools/s3.sh spi` suite.
- [ ] Run inference while SPI traffic is active and record task stack high-water marks and heap.
- [ ] Re-run MQTT/fault/endurance tests if scheduler buffers, publisher behaviour, task timing, or memory use changed.
- [ ] Compare new D1 results with retained logs under `parity/logs/s3/`; explain every difference.

## Phase 7 — documentation and merge gate

- [ ] Replace proposed D2 settings with the implemented schema and measured limits in all documentation.
- [ ] Fill the ESP32-S3 result/evidence columns for FR-16, FR-17, NFR-08, and T-17 to T-23.
- [ ] Update the final SPI pins without presenting the mutually exclusive loopback and TFT fixtures as simultaneous wiring.
- [ ] Document the confirmed TFT controller and the fact that TFT success proves TX only.
- [ ] List every direct ESP-IDF call and justify why Arduino SPI was insufficient, or record that no new exception was required.
- [ ] Review the final diff together and remove credentials, generated binaries, and local build output.
- [ ] Merge only when all Definition of done items pass and every evidence link resolves from a clean checkout.

## Fastest safe execution order

1. Freeze the scheduled size limits, error enum, host name, and backend choice.
2. Add `bus_t.deinit` and scheduler cleanup first; run D1 regressions immediately.
3. Add config fields/parser/validation and registry wiring; test invalid configurations before hardware work.
4. Add HAL calls, Pico unsupported stubs, and the EdgeS3 SPI backend.
5. Prove 2-byte and 128-byte MOSI↔MISO loopback through the REPL/scheduler.
6. Add the dedicated 8–1024-byte integrity/benchmark harness.
7. Add the confirmed TFT/real-device layer and TX proof.
8. Run fault, lifecycle, concurrency, and full D1 regression gates.
9. Fill evidence and documentation, review together, then merge.

## Remaining decisions before coding

- [ ] Confirm the RX proof: jumper loopback versus a readable SPI device.
- [ ] Confirm the TFT controller and backlight electrical behaviour.
- [ ] Approve normal scheduled TX/RX limits and keep the 1024-byte benchmark path separate.
- [ ] Approve `SPIClass` as the first backend and the evidence threshold for falling back to ESP-IDF.
- [ ] Approve maximum SPI clock and numerical performance thresholds.
- [ ] Confirm that queued/streaming SPI is deferred from D2.
