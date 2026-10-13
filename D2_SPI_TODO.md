# D2 — ESP32-S3 SPI Support To-Do List

Branch: `s3-port-d2` (created from the D1 `s3-port` baseline)  
Team: Brendan and Jy  
Status: planning and implementation checklist

## End outcome

SPI is a first-class runtime-configurable interface behind the same `bus_t` abstraction and registry used by GPIO, UART, and I2C. Application and scheduler code reach an SPI device through `bus_lookup("spi")`; they do not call Arduino or ESP-IDF SPI APIs directly.

The finished feature must let a configuration select the SPI host and map SCLK, MOSI, MISO, and chip select at runtime, set clock frequency and SPI mode, transmit and receive data, and report configuration, initialization, timeout, transmit, receive, and verification failures explicitly. A failed operation must never be published as valid-looking sensor data.

## Definition of done

- [ ] `proto=spi` is parsed, validated, displayed by `SHOW`, scheduled, and resolved through the common bus registry.
- [ ] Runtime configuration supports bus/host, SCLK, MOSI, MISO, CS, clock frequency, SPI mode, transmit bytes, receive length, timeout, and any agreed duplex/queue options.
- [ ] Invalid protocol values and invalid SPI settings are rejected before hardware state is changed.
- [ ] The ESP32-S3 SPI backend owns platform APIs; shared application code remains platform-neutral.
- [ ] Multiple chip-select devices can safely share a compatible physical bus, or the implementation returns a clear conflict error.
- [ ] Contexts and hardware resources are released on `CLEAR`, replacement, failure, and reconfiguration.
- [ ] Transmit-only, receive, and full-duplex paths have explicit success/error semantics.
- [ ] A known-pattern loopback or readable SPI peripheral proves byte-for-byte receive integrity.
- [ ] A real SPI device or TFT demonstrates useful transmit-side operation through the same transport.
- [ ] Payloads from 8 to 1024 bytes have recorded latency, throughput, timeout/error count, and byte-mismatch count.
- [ ] D1 boundary checks, Pico reference build, ESP32-S3 build, existing parity tests, and new SPI tests pass or have documented dispositions.
- [ ] Requirements, architecture, configuration documentation, pin allocation, tests, and retained evidence identify the tested commit, board, tool versions, tester, and date.

## D1 constraints and dependencies

- [ ] Confirm the Task 8 pin layout before final wiring. Do not use native USB pins GPIO19/20, boot/strapping pins, or flash/PSRAM pins reserved on the actual module.
- [ ] Reuse the Task 11 HAL and `bus_t` boundary; do not introduce ESP32 headers into shared scheduler, parser, rule, or output code.
- [ ] Coordinate with Task 12 so the ESP32-S3 build compiles the same shared sources instead of a copied fork.
- [ ] Keep SPI work below the Task 13 engine ownership model. SPI callbacks or lower-level driver events must not mutate scheduler state directly.
- [ ] Preserve the D1 completion rule: compiling or drawing on a TFT is not sufficient without negative tests, measurements, and retained evidence.
- [ ] Record any required direct ESP-IDF `spi_master` use as a documented Arduino-layer exception with a test and justification.

## Suggested teammate split

The split is designed to minimize overlapping edits. Swap owners if needed, but keep one owner per file during active work.

### Brendan — shared contract and integration

- [ ] Own `app_cfg_t`, `proto_spi`, parser validation, defaults, `SHOW` output, scheduler summaries, registry wiring, and portable error semantics.
- [ ] Extend `bus_t` only after agreeing source-compatibility requirements with Jy and the D1 owner.
- [ ] Add host-side parser/registry tests and update the parity harness.
- [ ] Keep `README.md`, `ARCHITECTURE.md`, `project_v3/README.md`, and `parity/PARITY.md` synchronized with the implemented contract.

### Jy — ESP32-S3 backend and hardware evidence

- [ ] Own the ESP32-S3 SPI bus manager, device contexts, locking, initialization, transaction, timeout, cleanup, and error translation.
- [ ] Confirm the physical pinout and wire the loopback/readable device and optional TFT.
- [ ] Implement the benchmark/evidence logger and capture hardware results.
- [ ] Record Arduino-ESP32/ESP-IDF APIs used, DMA constraints, heap behavior, and any limitations.

### Pair work

- [ ] Review the configuration schema and error model before implementation.
- [ ] Review every boundary-crossing change together.
- [ ] Run the final hardware demo and complete the evidence table together.
- [ ] Do not merge until both teammates review the diff and reproduce the main test on the same commit.

## Phase 1 — freeze the SPI contract

- [ ] Confirm the target proof devices: MOSI-to-MISO loopback or readable SPI memory for RX integrity, plus the TFT or another real device for application-level TX proof.
- [ ] Confirm the exact ESP32-S3 module marking, available host, flash/PSRAM wiring, and safe GPIOs.
- [ ] Agree whether `bus_t` gains `transceive` and `deinit` callbacks or whether an equivalent versioned extension is used.
- [ ] Define one error enum for invalid configuration, unsupported operation, conflict/busy, timeout, I/O failure, verification failure, no memory, and device-not-present.
- [ ] Define byte-operation returns: positive valid RX byte count, zero successful TX-only operation, negative typed error.
- [ ] Decide whether the first release supports polling only. If streaming is deferred, return `unsupported` rather than silently pretending it started.
- [ ] Obtain team sign-off for proposed requirements FR-16, FR-17, and NFR-08 plus tests T-17 to T-23 in `parity/PARITY.md`.

Proposed minimum configuration:

```ini
BEGINCFG|name=SPI_LOOP|proto=spi|mode=poll|freq_hz=2|spi.host=spi2|spi.sclk=12|spi.mosi=11|spi.miso=13|spi.cs=10|spi.hz=10000000|spi.mode=0|spi.tx=0xA5 0x5A|spi.read_len=2|spi.timeout_ms=100|ENDCFG
```

Required validation:

- [ ] Supported host value.
- [ ] SPI mode is 0–3.
- [ ] Clock is positive and within the agreed hardware/backend limit.
- [ ] Pins are valid, available, unique where required, and compatible with the confirmed board layout.
- [ ] MISO is present whenever receive bytes are requested.
- [ ] CS is valid, or an explicit no-CS mode is documented.
- [ ] TX and RX lengths fit configured and backend limits.
- [ ] Timeout and queue/depth values are bounded.
- [ ] A proposed shared bus matches the existing host and bus-pin configuration.

## Phase 2 — extend the shared configuration and registry

- [ ] Add `proto_spi` without changing existing enum meanings unexpectedly.
- [ ] Add bounded SPI fields to `app_cfg_t`; avoid storing pointers into temporary parser buffers.
- [ ] Parse all agreed `spi.*` keys, including hexadecimal and decimal TX bytes.
- [ ] Fix unknown `proto=` handling so it returns an error instead of falling through to another protocol (D1 defect D-01).
- [ ] Add configuration validation before `scheduler_add_config()` commits a slot.
- [ ] Add SPI to `proto_name()`, `bus_if.h`, and the `bus_if.c` registry.
- [ ] Add an SPI-specific scheduler summary that prints host, pins, clock, mode, TX length, RX length, and timeout.
- [ ] Ensure `SHOW` reports the active SPI configuration without exposing stale/uninitialized fields.
- [ ] Add cleanup on `CLEAR`, failed initialization, and replacement/reconfiguration.
- [ ] Update build manifests for each supported target without duplicating source files.

## Phase 3 — implement the ESP32-S3 transport

- [ ] Implement one shared-bus record per host/bus-pin combination and one device context per CS line.
- [ ] Initialize the physical bus once and reference-count attached devices.
- [ ] Reject incompatible re-use of an initialized host instead of reconfiguring it underneath an active device.
- [ ] Serialize transactions on a shared host using the Task 13 ownership/locking design.
- [ ] Configure host, pins, CS, frequency, mode, duplex, queue depth, timeout, and maximum transfer size from the validated runtime configuration.
- [ ] Implement transmit-only, receive, and full-duplex transaction paths.
- [ ] Translate every Arduino/ESP-IDF result into the common error model.
- [ ] Ensure RX buffers are published only when the transaction succeeds and the returned length is valid.
- [ ] Remove devices and free the bus only after the final user releases it.
- [ ] Make repeated initialize/deinitialize cycles leak-free.
- [ ] Document DMA-capable buffer/alignment rules and use a bounded allocation strategy.

## Phase 4 — functional proof

- [ ] Registry proof: show `proto=spi` resolving to `bus_spi` through normal configuration and scheduler flow.
- [ ] Loopback/readable-device proof: transmit deterministic bytes and compare every returned byte.
- [ ] TX proof: initialize the TFT or chosen SPI peripheral through the same transport and produce a deterministic visible/result pattern.
- [ ] Multi-device proof: attach two CS devices to one compatible bus, or document why the available hardware prevents the test.
- [ ] Runtime reconfiguration proof: change an allowed device setting and continue operating.
- [ ] Rollback proof: attempt an invalid/incompatible reconfiguration and show the previous device remains usable.
- [ ] Concurrent proof: run SPI while at least one existing I2C/GPIO workload is active; repeat with ML inference when D4 integration is ready.

## Phase 5 — negative and fault tests

- [ ] Invalid/unknown host is rejected.
- [ ] Modes below 0 or above 3 are rejected.
- [ ] Zero, negative, or excessive clock is rejected.
- [ ] Invalid, reserved, duplicated, or conflicting pins are rejected.
- [ ] Receive request with MISO disabled is rejected.
- [ ] Oversized TX/RX request is rejected.
- [ ] Disconnected loopback or absent device reports an error/verification failure, not zero-filled valid data.
- [ ] Timeout and queue/bus-busy paths return distinct errors.
- [ ] Deliberate byte corruption is detected.
- [ ] Failed initialization and failed reconfiguration release partial resources.
- [ ] One hundred initialize/deinitialize cycles show no progressive heap loss.
- [ ] Sustained operation produces no watchdog reset or unexplained sample loss.

## Phase 6 — benchmark and retained evidence

- [ ] Measure payload sizes 8, 16, 32, 64, 128, 256, 512, and 1024 bytes.
- [ ] Perform at least 10 warm-ups and 1,000 measured transactions per selected payload/clock combination.
- [ ] Record median, p95, p99, and maximum latency.
- [ ] Record effective payload throughput.
- [ ] Record attempts, platform errors, timeouts, mismatched transactions, mismatched bytes, resets, and heap before/after.
- [ ] Report transaction error rate and byte error rate separately.
- [ ] Retain CSV/raw logs plus a short Markdown result summary under `parity/logs/d2-spi/`.
- [ ] Include commit, board/module, Arduino IDE/core, underlying ESP-IDF version, CPU frequency, host, pins, mode, clock, DMA/transaction method, tester, and date.

## Phase 7 — regression, documentation, and merge gate

- [ ] Run the D1 HAL boundary checker.
- [ ] Rebuild the frozen Pico reference and confirm D2 shared changes do not break it.
- [ ] Build the ESP32-S3 target using the pinned Task 7 environment.
- [ ] Run existing REPL/configuration/scheduler tests and the new SPI tests.
- [ ] Update `README.md`, `ARCHITECTURE.md`, `project_v3/README.md`, `SETUP.md`, and `parity/PARITY.md` with actual—not planned—settings and evidence.
- [ ] List all direct ESP-IDF calls and why Arduino SPI APIs were insufficient.
- [ ] Document unsupported modes/devices and any approved deviations.
- [ ] Review the final diff together and remove generated files, credentials, and local build output.
- [ ] Merge only when the Definition of done is satisfied and evidence links resolve from a clean checkout.

## Final demonstration

1. Boot the ESP32-S3 and show the pinned build/tool versions.
2. Submit the SPI configuration through the same REPL/configuration path as I2C/UART/GPIO.
3. Show the scheduler registering `SPI_LOOP` on `spi` through the common registry.
4. Run a known-pattern transaction and show the exact returned bytes.
5. Inject one invalid configuration and one disconnected/verification failure; show both are explicit errors and no sample is published.
6. Run the real SPI device/TFT proof.
7. Present the 8–1024-byte benchmark summary and error counts.
8. Run the regression smoke test and identify the tested commit and retained evidence.

## Open decisions

- [ ] Exact SPI proof device and whether the TFT exposes MISO.
- [ ] Final safe SCLK/MOSI/MISO/CS pins after Task 8 is closed.
- [ ] Arduino `SPIClass` only versus a documented ESP-IDF `spi_master` backend.
- [ ] Whether `bus_t` can change directly or requires a backward-compatible extension.
- [ ] Approved maximum SPI clock and numeric performance thresholds.
- [ ] Whether streaming/queued transactions are in D2 scope or explicitly deferred.

