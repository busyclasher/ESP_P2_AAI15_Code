# D1 Requirements and Parity Table (Tasks 4 and 5)

Owner: Hasif. Requirements are proposed for team review (task 4). The Pico column is the executed baseline (task 5); rows marked pending need hardware or a network that was not available during the run. The ESP32-S3 column is filled in during tasks 13 to 21 using the same commands.

**Reference under test:** `s3-port` commit `543a759` (firmware sources unchanged through the later documentation/test commits), Raspberry Pi Pico 2 W (RP2350, confirmed from the bootloader's `INFO_UF2.TXT`), Pico SDK 2.3.0, Arm GNU Toolchain 14.2.Rel1. Tester: Hasif, 2026-09-28 to 2026-09-29. Build record and artifact hashes: [logs/3-build.txt](logs/3-build.txt).

**Reproducing a result:** the exact command text for every row is in [parity_test.py](parity_test.py) under the stage and step named in the table. Run `python3 parity/parity_test.py --stage <stage>` with the board connected over USB; the raw serial capture is written to `parity/logs/<stage>.log`.

## 1. Requirements (task 4, proposed)

### Functional

| ID | Requirement | Acceptance criterion | Planned test | Owner | Evidence |
|---|---|---|---|---|---|
| FR-01 | The REPL commands `HELP`, `SHOW`, `SHOWRULES`, `RUN`, `STOP`, `CLEAR` and `CLEARRULES` behave as on the Pico | Each command returns the same response lines as the Pico reference | P-01 to P-06 | Pico: Hasif. S3: task 13 | `4a.log` |
| FR-02 | An inline `BEGINCFG` block is parsed, validated and scheduled | `CFG: ok (added #n: NAME)` and the sensor appears in `SHOW` | P-07 | Pico: Hasif. S3: task 14 | `4a.log` |
| FR-03 | Invalid input is refused without disrupting the running system | Each case in N-01 to N-08 produces its documented message and the board keeps responding | N-01 to N-08 | Pico: Hasif. S3: tasks 13, 14, 18 | `5-neg.log` |
| FR-04 | GPIO digital input honours pull and reports both levels | Reads `00` and `01` when the pin is driven low and high | P-09, P-10 | Pico: Hasif. S3: task 14 | `4a.log`, `4b.log` |
| FR-05 | GPIO counter mode counts level changes | Count increases by one per change seen between samples | P-11 | Pico: Hasif. S3: task 15 | `4b.log` |
| FR-06 | GPIO pulse mode measures HC-SR04 echo width | Width changes with distance; no echo reports `len=1 : 00` | P-12, P-13 | Pico: Hasif. S3: task 15 | `4c-sonic.log` |
| FR-07 | GPIO onewire mode reads a DHT22 or AM2302 | Humidity and temperature frames decode to plausible values | P-14 | Pico: Hasif. S3: task 15 | Pending |
| FR-08 | I2C reads a register after optional pre-writes, with repeated start | A known device returns its identity register; a missing device is rejected | P-15, N-02 | Pico: Hasif. S3: task 16 | `5-neg.log` |
| FR-09 | UART request and response with pre-bytes, delay and read length | Bytes sent return intact and in order | P-16 | Pico: Hasif. S3: task 17 | `4c-uart.log` |
| FR-10 | Rule engine evaluates `calc`/`when` and runs every local action | True and false expressions are deterministic; `log`, GPIO `HIGH`/`LOW`/`TOGGLE`/`PULSE`, `pwm` and `batch:` actions run as documented; pulse length and PWM duty are measured | P-18 to P-22, G-03 | Pico: Hasif. S3: task 18 | `4a.log`, `4b.log`; measurement pending |
| FR-11 | Inference runs the built-in model through `infer.set` and `infer.run` | An `[INFER] invoke=` line with a score, mask and threshold is printed | P-23 | Pico: Hasif. S3: task 20 | `4a.log` |
| FR-12 | Wi-Fi connects and MQTT carries telemetry, text and JSON configuration, and commands | Telemetry reaches the broker; configuration and commands sent over MQTT are applied | P-25 to P-27 | Pico: Hasif. S3: task 19 | Pending |
| FR-13 | Remote rule actions (`cmd.to`, `cfg.to`) and sensor bridging work between nodes | A rule on one node changes the state of another | P-28 | Pico: Hasif. S3: task 19 | Pending |
| FR-14 | A model uploaded with `send_model.py` is stored and loaded without rebuilding firmware | Upload completes and `infer.run` uses the new model | P-29 | Pico: Hasif. S3: task 20 | Pending |
| FR-15 | Capacity is at least 8 sensors and 4 rules | The ninth sensor and fifth rule are refused with a message | N-04, N-06 | Pico: Hasif. S3: tasks 14, 18 | `5-neg.log` |
| FR-16 | JSON configuration/rules and emitted text/JSON retain their documented schema | Valid single and combined JSON payloads create the same sensors/rules as inline input; malformed or oversized JSON is rejected; output field names, types and byte order match the reference contract | G-01 | Pico: Hasif. S3: tasks 13, 14, 19 | Pending network test |
| FR-17 | Scheduler and GPIO digital behaviour cover pull, invert, debounce, invalid pins and overload | Each option has a positive/negative test; overload follows the approved missed-deadline policy without starving control or networking | G-02 | Pico: Hasif. S3: task 14 | Pending |
| FR-18 | I2C ownership and fault recovery are bounded | NACK, timeout, repeated start, multiple devices and a stuck/recovered bus have recorded results and do not block the scheduler beyond the configured timeout | G-05 | Pico: Hasif. S3: task 16 | Missing-device evidence only; remaining cases pending |
| FR-19 | UART polling/streaming handles framing and bounded input | Pre-bytes, delay, line mode, partial message, framing, timeout, overflow and sustained input have deterministic outcomes without using the console UART | G-06 | Pico: Hasif. S3: task 17 | Loopback evidence only; remaining cases pending |
| FR-20 | Wi-Fi/MQTT recovers safely from transport and payload faults | Wi-Fi loss, broker loss, reconnect, malformed/oversized input and queue saturation follow documented policies without leaks, resets or stale subscriptions | G-07, G-08 | Pico: Hasif. S3: task 19 | Pending network test |
| FR-21 | Model replacement is bounded, integrity checked and persistent | Valid upload survives reboot; invalid, oversized, corrupt, out-of-order and interrupted transfers leave no corrupt active model; a previous known-good model remains usable | G-09 | Pico: Hasif. S3: task 20 | Pending network/model test |
| FR-22 | The S3 FreeRTOS runtime has explicit ownership and remains responsive | Task priorities/affinities match the approved design; all waits time out; shared objects are protected; stack/heap metrics and watchdog status remain within NFR limits | G-10 to G-12 | Pico: not applicable (non-RTOS reference). S3: task 13 | Pending S3 implementation |

### Non-functional

Values in the target column are proposals for team review, derived from the Pico measurements where a valid measurement exists. The team must approve or replace every proposed number before Task 10 freezes the architecture.

| ID | Requirement | Pico measurement / baseline | Proposed ESP32-S3 acceptance target | Planned test | Owner | Target evidence |
|---|---|---|---|---|---|---|
| NFR-01 | Main loop and sampling period | Loop period 1000 to 1201 ms, mean 1006.4 ms over 301 passes. A sensor configured at `freq_hz=2` was sampled every 1002 to 1003 ms because the main loop sleeps 1000 ms | At 1 Hz, period 1000 ms ±10%; for non-blocking GPIO at 2–10 Hz, mean period within ±10% and p99 jitter reported; missed deadlines counted | Timestamp at least 300 samples per selected rate, then scheduler-overload run | Pico: Hasif. S3: task 14 owner | Raw timestamp log plus calculation output |
| NFR-02 | REPL response latency | Board response within about 1.2 s; host USB delivery occasionally lagged by about 5 s | p95 board response at most 250 ms and maximum 1 s while sensors/MQTT/inference are active | Timestamped command/reply pairs at idle and under load | Pico: Hasif. S3: task 13 owner | Raw log and latency summary |
| NFR-03 | Inference latency | `invoke=` 961 to 1298 µs over 5 runs | p95 no slower than 1298 µs over 100 identical runs; maximum recorded | P-23 plus repeated inference stage | Pico: Hasif. S3: task 20 owner | Serial log and percentile summary |
| NFR-04 | Inference agreement | `MSE=0.4179` for input 1.0 in slot 0 | Absolute score difference at most 0.01 for the same model, normalization and input | P-23 plus fixed input vectors for each score mode | Pico: Hasif. S3: task 20 owner | Model hash, input vectors and output log |
| NFR-05 | Watchdog stability | No watchdog events in the recorded Pico runs; Pico has a different runtime architecture | No watchdog reset or task starvation during the full suite and approved endurance run; every blocking path has a finite bound | G-10, G-12 and fault suite | S3: tasks 13/21 owners | Reset reason, heartbeat and watchdog log |
| NFR-06 | Program/storage fit | Code 639,852 B; zero-initialised RAM 295,424 B | Fits the Task 8 module and selected partition layout; firmware/model partitions retain at least 10% free capacity after final image/model | Build/partition size report | S3: tasks 12/20 owners | Map/size report and partition table |
| NFR-07 | Runtime memory/stack headroom | Not measured on Pico | Minimum free heap remains at least 20% of post-startup free heap; every application task retains at least 25% of its allocated stack after the full suite | G-11 at boot, peak load and after endurance | S3: tasks 13/21 owners | Heap minima and per-task high-water log |
| NFR-08 | MQTT recovery latency | Not run on Pico | With Wi-Fi/broker restored, online state, subscriptions and presence recover within 35 s; backoff is bounded at 30 s | G-07 repeated Wi-Fi and broker interruptions | Pico: Hasif. S3: task 19 owner | Broker capture and serial timestamps |
| NFR-09 | Queue saturation and leak resistance | Pico inbound queue drops oldest at capacity 6; no saturation measurement | Each queue exposes or logs rejection/drop counts; control/configuration is never silently overwritten; heap after 1000 saturation messages is within 5% of its pre-test value | G-08 | S3: tasks 13/19 owners | Queue counters and before/after heap log |
| NFR-10 | Continuous operation | No resets across separate reference runs; no continuous duration recorded | Proposed 8-hour mixed sensor/MQTT/inference run with no reset, deadlock or unbounded memory decrease; team confirms duration | G-12 | Team / task 21 | Continuous timestamped log and final resource snapshot |
| NFR-11 | Model-transfer recovery | Not run on Pico | Interrupted/corrupt/oversized transfers complete or reject within a bounded timeout and never replace the last valid model; valid model remains usable after reboot | G-09 | Pico: Hasif. S3: task 20 owner | Broker/serial capture, CRC/model hash and reboot result |

### Reference defects found

These are behaviours of the supplied Pico firmware, recorded as the baseline. The team decides in task 10 whether the ESP32-S3 port reproduces each one for strict parity or fixes it.

| ID | Defect | Evidence |
|---|---|---|
| D-01 | An unsupported `proto=` value is not rejected. The parser keeps its default, so `proto=spi` becomes an I2C sensor on SDA 4, SCL 5, address `0x76`, and the reply is `CFG: ok` | N-01 |
| D-02 | A digital config without `gpio.pin` is accepted and silently reads GP2 | N-03 |
| D-03 | A malformed `when=` expression is accepted with `RULE: ok`; rules are only checked for a source and table space | N-07 |
| D-04 | A non-numeric pin in a GPIO action is accepted when the rule is added and only reported when it fires | N-05 |
| D-05 | The UART driver never empties its receive buffer. On the first run after boot, a stray `00` entered when the pins were set up, and every later reply stayed one byte out of step | P-17 |
| D-06 | Sampling is capped at about 1 Hz by the fixed 1000 ms main-loop delay, whatever `freq_hz` requests | NFR-01 |

## 2. Parity table (task 5)

**Status summary: task 5 is not complete, and no real sensor has been verified on the Pico yet.** Of 38 executed/reference rows, 23 pass: 16 need no wiring, 6 use jumper-wire loopbacks with no sensor attached (GP21 to GP22 for GPIO, GP16 to GP17 for UART), and 1 passes only on the timeout path because the attached HC-SR04 never responded. Four defects, one UART quirk and one timing measurement are recorded. Nine rows are pending: every real sensor (HC-SR04, TSL2561 light sensor, one-wire, UART radar) and every Wi-Fi/MQTT row. The additional gap rows in section 3 are planned but have not been executed and are not part of the 38-row count.

| ID | Req | Stage / step | Expected | Pico observed | Pico result | Evidence | ESP32-S3 result |
|---|---|---|---|---|---|---|---|
| P-01 | FR-01 | `4a` / `repl-help` | Command list | `Commands:` block | Pass | `4a.log` | |
| P-02 | FR-01 | `4a` / `repl-show`, `repl-show-cfg` | Sampling state, sensors and rule count | `[SHOW] sampling=0`, `[SCHED] #0 name=DIG …`, `[RULE] count=0` | Pass | `4a.log` | |
| P-03 | FR-01 | `4a` / `repl-run`, `repl-stop` | `RUN` / `STOP` echoed | `RUN`, `STOP` | Pass | `4a.log` | |
| P-04 | FR-01 | `4a` / `repl-showrules` | Rule count | `[RULE] count=2` | Pass | `4a.log` | |
| P-05 | FR-01 | `4a` / `repl-clearrules` | Rules cleared | `[RULE] reset` | Pass | `4a.log` | |
| P-06 | FR-01 | `4a` / `repl-clear` | Sensors and rules cleared | `CLEARED` | Pass | `4a.log` | |
| P-07 | FR-02 | `4a` / `cfg-digital` | Config accepted and scheduled | `CFG: ok (added #0: DIG)` | Pass | `4a.log` | |
| P-08 | FR-01 | `4b` / `boot` | Wi-Fi attempt, inference start, model load | `WiFi connect failed` (placeholder credentials), `Base model loaded: 16 inputs, 16 outputs`, 16 normalisation params, 16 threshold masks | Pass | `4b.log` | |
| P-09 | FR-04 | `4a` / `run-data` | Pin with pull-down reads low | `: 00 …` | Pass | `4a.log` | |
| P-10 | FR-04 | `4b` / `gpio-high`, `gpio-low`, `gpio-toggle` | GP22 follows GP21 through a jumper | `01` after HIGH, `00` after LOW, alternating on TOGGLE | Pass | `4b.log` | |
| P-11 | FR-05 | `4b` / `gpio-counter` | Count rises with each toggle | Reached `03 00 00 00` | Pass | `4b.log` | |
| P-12 | FR-06 | `4c-sonic` / `sonic-read` | Echo width changes with distance | No valid echo: the HC-SR04 was powered at 3.3 V and needs 5 V with a 1:2 resistor divider on Echo | **Pending hardware** | `4c-sonic.log` | |
| P-13 | FR-06 | `4c-sonic` / `sonic-read` | No echo reports a timeout frame | `len=1 : 00` on 44 of 44 readings | Pass | `4c-sonic.log` | |
| P-14 | FR-07 | `4c-onewire` / `onewire-read` | DHT22 or AM2302 checksum-valid 4-byte humidity/temperature frame | Test stage written; no sensor of this type available | **Pending hardware** | | |
| P-15 | FR-08 | `4c-i2c` / `cfg-tsl-id`, `i2c-id` | TSL2561 identity register reads `0x5X` | Device did not acknowledge at `0x29`, `0x39` or `0x49`; wiring to be rechecked | **Pending hardware** | `4c-i2c.log` | |
| P-16 | FR-09 | `4c-uart` / `uart-echo-a`, `uart-echo-b` | Bytes return intact and in order | `A5 5A` and `12 34 56` with GP16 looped to GP17 | Pass | `4c-uart.log` | |
| P-17 | FR-09 | `4c-uart` / `uart-echo-a` | First read after boot aligned | Offset by one byte on the first run after boot (D-05) | Recorded quirk | Observed 2026-09-29; that log was replaced by the passing rerun | |
| P-18 | FR-10 | `4a` / `rule-log`, `run-data` | `log` action fires when `when` is true | `[RULE] R_LOG: pin low (value=0.000 …)` | Pass | `4a.log` | |
| P-19 | FR-10 | `4b` / `gpio-high`, `gpio-low`, `gpio-toggle` | GPIO actions change the pin | Confirmed electrically on GP22 | Pass | `4b.log` | |
| P-20 | FR-10 | `4b` / `gpio-pulse` | `PULSE` action runs | `gpio 21=PULSE 200ms`; pulse length not measured | Pass | `4b.log` | |
| P-21 | FR-10 | `4b` / `pwm-run` | PWM duty set | `pwm 21 duty=0.500`; waveform not measured | Pass | `4b.log` | |
| P-22 | FR-10 | `4a` / `rule-infer` | `batch:` with escaped `\|` accepted and run | `RULE: ok`, then inference ran | Pass | `4a.log` | |
| P-23 | FR-11 | `4a` / `run-data` | Inference prints latency, score, mask, threshold | `invoke=1298us`, `MSE=0.4179`, `mask=1000`, `thresh=1.1407 OK` | Pass | `4a.log` | |
| P-24 | NFR-01 | All stages | Loop period and sample interval recorded | 1000 to 1201 ms; `freq_hz=2` sampled at about 1 Hz | Measured | All logs | |
| P-25 | FR-12 | Not yet written | Wi-Fi connect with real credentials | Not run | **Pending network** | | |
| P-26 | FR-12 | Not yet written | Telemetry on `pico/<node>/sensor/<name>` | Not run | **Pending network** | | |
| P-27 | FR-12 | Not yet written | Text and JSON configuration and commands over MQTT | Not run | **Pending network** | | |
| P-28 | FR-13 | Not yet written | `cmd.to`, `cfg.to` and bridging between two nodes | Not run; needs a second Pico | **Pending network** | | |
| P-29 | FR-14 | Not yet written | `send_model.py` upload, then `infer.run` | Not run | **Pending network** | | |
| P-30 | FR-09 | Not yet written | The team's reference UART sensor (radar) returns its frames | Not run. UART has only been proven with a loopback wire (P-16) | **Pending hardware** | | |
| N-01 | FR-03 | `5-neg` / `neg-proto` | Unsupported protocol refused | Accepted as I2C on default pins (D-01) | Defect recorded | `5-neg.log` | |
| N-02 | FR-08 | `5-neg` / `neg-i2c-nodev` | Missing I2C device refused | `init_cfg FAILED for NODEV`, `CFG: add failed (-2)` | Pass | `5-neg.log` | |
| N-03 | FR-03 | `5-neg` / `neg-nopin` | Config without a pin refused | Accepted on GP2 (D-02) | Defect recorded | `5-neg.log` | |
| N-04 | FR-15 | `5-neg` / `slot-1` to `slot-8`, `neg-slots` | Eight sensors accepted, ninth refused | 8 × `CFG: ok`, then `[SCHED] no free slots` | Pass | `5-neg.log` | |
| N-05 | FR-03 | `5-neg` / `rule-4-badpin`, `neg-badpin-run` | Non-numeric GPIO pin refused | Accepted, then reported when fired: `bad gpio pin in action='gpio:X=HIGH'` (D-04) | Defect recorded | `5-neg.log` | |
| N-06 | FR-15 | `5-neg` / `rule-1` to `rule-4-badpin`, `neg-rule-full` | Four rules accepted, fifth refused | `[RULE] table full`, `RULE: parse error` | Pass | `5-neg.log` | |
| N-07 | FR-03 | `5-neg` / `neg-badexpr` | Malformed expression refused | `RULE: ok` (D-03) | Defect recorded | `5-neg.log` | |
| N-08 | FR-03 | `5-neg` / `neg-rule-nosource`, `neg-toolong`; `4a` / `repl-unknown` | Refused with a message | `[RULE] missing source=`, `Line too long`, `Unknown: FOO` | Pass | `5-neg.log`, `4a.log` | |

## 3. Planned coverage gaps

These rows make the remaining Tasks 13–21 coverage explicit. They are not passes and do not change the executed summary above. Before running a row, add the exact commands, fixtures, measurement method and expected messages to the harness or a companion host-side test, then record both targets' outcomes.

| ID | Req | Feature / fault | Pico plan/result | ESP32-S3 plan/result | Owner | Required evidence |
|---|---|---|---|---|---|---|
| G-01 | FR-16 | Inline versus single/combined JSON configuration/rules; malformed and oversized JSON; output schema | Publish documented fixtures to the reference and capture created objects/errors | Repeat identical broker payloads in task 19 | Pico: Hasif. S3: task 19 owner | Payload files, broker capture and serial log |
| G-02 | FR-17, NFR-01 | GPIO pull/invert/debounce/invalid pin plus scheduler rate, jitter and overload | Jumper/signal-generator test; record D-02 where applicable | Repeat on approved pins at 1, 2, 5 and 10 Hz; overload all slots | Pico: Hasif. S3: task 14 owner | Wiring, timestamp log, jitter script output and missed-deadline count |
| G-03 | FR-10 | GPIO pulse duration and PWM frequency/duty are electrically correct | Measure P-20/P-21 with logic analyser/oscilloscope | Repeat using S3 GPIO/LEDC | Pico: Hasif. S3: task 18 owner | Instrument screenshot/export and configured values |
| G-04 | FR-05 | Counter event-loss limit and concurrency | Drive a known pulse train and compare sent/received counts | Repeat at approved maximum rate while MQTT/REPL run | Pico: Hasif. S3: task 15 owner | Generator settings, event totals and loss calculation |
| G-05 | FR-18 | I2C repeated-start, NACK, timeout, two devices and stuck-bus recovery | Run on reference devices; force SDA/SCL fault safely | Repeat through `Wire` or justified lower-level backend | Pico: Hasif. S3: task 16 owner | Wiring, configs and bounded-time recovery log |
| G-06 | FR-19 | UART framing, line mode, partial input, timeout, overflow and sustained traffic | Host/second-UART fixture sends deterministic frames | Repeat without sharing USB console | Pico: Hasif. S3: task 17 owner | Generator script/config and received/error counts |
| G-07 | FR-20, NFR-08 | Wi-Fi loss, broker loss, backoff, reconnect, subscriptions and presence | Execute after Pico credentials/broker are available | Repeat with pinned Arduino MQTT library | Pico: Hasif. S3: task 19 owner | Broker capture and serial timestamps |
| G-08 | FR-20, NFR-09 | Malformed/oversized MQTT payload and inbound/outbound queue saturation | Publish bounded fault corpus; record Pico drop policy | Repeat; verify explicit reject/drop counters and heap stability | Pico: Hasif. S3: task 19 owner | Fault corpus, counters and heap before/after |
| G-09 | FR-21, NFR-11 | Valid, oversized, corrupt, out-of-order and interrupted model transfer plus reboot persistence | Run `send_model.py` and fault variants; current persistence expectation to be confirmed | Repeat against approved partition and last-known-good design | Pico: Hasif. S3: task 20 owner | Model hash/CRC, broker/serial log and post-reboot inference |
| G-10 | FR-22, NFR-02, NFR-05 | REPL and control remain responsive while sensor, MQTT, inference and model operations contend | Pico latency baseline under comparable load | Exercise approved task/queue topology and watchdog | Pico: Hasif. S3: task 13 owner | Command/reply latency, heartbeat and reset reason |
| G-11 | FR-22, NFR-06, NFR-07 | Image/partition fit, heap minima and every task stack high-water mark | Record Pico build size; runtime heap/stack is not architecturally comparable | Capture at boot, peak mixed load and test end | S3: tasks 12/13/21 owners | Map/partition report and diagnostics log |
| G-12 | FR-22, NFR-05, NFR-10 | Continuous mixed workload | Record any agreed Pico comparison run | Proposed 8-hour sensor/MQTT/inference run after team approval | Team / task 21 | Continuous log, fault summary and final resource snapshot |

## 4. Task 4 review record

The requirements remain a draft until reviewers approve the proposed numeric limits, owners, defect policy and coverage rows. Review against a specific commit.

| Reviewer | Scope | Result / requested change | Date | Commit |
|---|---|---|---|---|
| TBD | Functional requirements and Pico baseline | Pending | TBD | TBD |
| TBD | Timing, memory, watchdog and endurance limits | Pending | TBD | TBD |
| TBD | Hardware/bus feasibility | Pending | TBD | TBD |
| TBD | MQTT/model fault coverage | Pending | TBD | TBD |
