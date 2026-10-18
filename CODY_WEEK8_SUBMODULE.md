# Cody Week 8 Sub-Module Ownership

Status: pre-Tribunal working position on `d3`, branched from `s3-port-d2`, 2026-10-18. This file identifies Cody's responsibility and the changes he must lead. It does not claim that unimplemented code or unrun hardware tests are complete.

Implementation status: the owned portable timing, EventRecord, ApplicationTask and occupancy contracts are now implemented and host-tested; see [CODY_PRD_IMPLEMENTATION.md](CODY_PRD_IMPLEMENTATION.md). Physical entrance timing, low-level non-blocking Echo capture, multi-node transport and reset persistence remain evidence-gated dependencies.

## 1. Cody's subsystem

Cody owns **System Integration and Validation**. In the implemented task model this means:

- `ApplicationTask`: consume completed entrance events and inference results, choose the configured classification result, update occupancy exactly once, and issue bounded output requests.
- Integrated evidence: define application-level acceptance criteria and interpret the hardware/runtime measurements supplied by Sitt and Hasif.
- D3 integration: own the timing contract and application-level timing consequences in the [D3 task list](D3_TASKS.md), while coordinating implementation with the scheduler, platform and driver owners.
- D6 integration: freeze the end-to-end event contract across the sensor, processing and output nodes.
- Validation: define cross-subsystem tests, expected results, evidence format and the final readiness judgement.

Cody does **not** silently take ownership of platform, sensor-driver, scheduler, MQTT, ML, SPI or low-level benchmarking internals. He owns the contract that those modules must satisfy for the application to integrate.

### Authoritative ownership boundary

**Cody directly owns**

- D3 timing contract and application-level timing integration;
- D6 end-to-end application integration contract;
- EventRecord, ApplicationTask, occupancy and output semantics;
- acceptance criteria and evidence interpretation for integrated behaviour.

**Cody coordinates**

- hardware/sensor timing with Sitt;
- scheduling/ordering with Brendan;
- MQTT/runtime/platform behaviour with Hasif;
- inference behaviour with Jun Yong.

**Cody does not directly own**

- all low-level drivers;
- all benchmarking implementation;
- all platform internals.

## 2. Responsibility map

| Change | Cody's responsibility | Dependency owner | Cody's deliverable | Completion evidence |
|---|---|---|---|---|
| F4: wiring diagrams per node | Integration lead: freeze which devices belong on the sensor, processing and output nodes and verify that every device has one task owner. | Sitt: sensor wiring and electrical checks. Brendan/JY: W25Q32 fixture. | One reviewed device-to-node table and three consistent diagrams. | Diagrams, wiring photographs and successful device checks on the actual boards. |
| F5: non-blocking HC-SR04 echo timing | Define the application-level timing/timeout contract and accept the result only if it leaves margin in the 50 ms window. | Sitt/Hasif: GPIO HAL or capture implementation. Brendan: SchedulerTask integration. | Approved capture outcomes, timeout semantics and D3 acceptance criteria. | No busy-wait in SchedulerTask; echo/no-echo measurements; Cody's pass/fail interpretation. |
| F6: SchedulerTask WCET | Define the integrated workload and interpret supplied measurements against the 50 ms contract. | Hasif: runtime instrumentation/measurement. Sitt: hardware timing. Brendan: scheduler configuration. | WCET acceptance threshold and integration conclusion. | `parity/wcet_test.py` raw log plus Cody's evidence interpretation. |
| F7: HC-SR04 trigger order and spacing | Define the required ordering semantics and the application consequence of timeout/ambiguity. | Sitt: physical crosstalk measurement. Brendan: scheduling/ordering implementation. | Approved ordering, timeout and degraded-result contract. | Two-sensor trace and crosstalk evidence interpreted against the contract. |
| F8: LD2450 continuous UART | Application integration owner: define the presence event that opens an observation window and the stale/fault behaviour. | Sitt: UART parser/buffer. Brendan: service/task scheduling. | Presence interface: timestamp, validity, expiry and quality flag. | Long stream test, overflow counter, corrupt frame and disconnect recovery. |
| F9: sensor-node EventRecord | **Primary owner.** Freeze the application-facing record and require the sensor node to assemble the window before MQTT. | Brendan: window producer. Hasif: MQTT schema/routing. Jun Yong: ML feature input. | Versioned fixed-size EventRecord with event ID, timing, features and quality. | Golden encode/decode tests plus delayed/out-of-order MQTT test showing one intact record. |
| F10: event ID, de-duplication and reset | **Primary owner.** Define exactly-once occupancy semantics and reset policy. | Hasif: MQTT/reset storage support. Brendan: source sequence/boot ID. | Event ID format, bounded de-dup state, idempotent transition rules and persistence decision. | Duplicate, replay, reboot and power-loss fixtures against known occupancy. |
| F15: TSL2561/GY-511 role | Application decision owner: retain a sensor only if it contributes a defined feature, fault check or demonstration requirement. | Jun Yong: model feature value. Sitt: hardware availability. | Keep/drop decision with requirement and feature mapping. | Updated parts list/config/diagrams and classifier feature definition. |
| R1: 50 ms integrated timing | Own the end-to-end timing budget, acceptance criteria and evidence interpretation. | Sitt supplies sensor/hardware timing; Hasif supplies RTOS/queue/platform measurements; Brendan supplies scheduler/ordering behaviour. | End-to-end pass/fail judgement with measured margin. | Full hardware evidence with p95/p99/max, misses, queue peaks, stack and heap. |
| R2: entry/exit order over MQTT | **Primary owner with Brendan.** Remove dependence on raw-reading arrival order by accepting only sensor-node EventRecords. | Brendan: local window ordering. Hasif: MQTT delivery. | Ordering and stale/duplicate acceptance rules. | Reorder/delay/duplicate injection cannot reverse or double-count an event. |
| R5: occupancy integrity | **Primary owner.** Maintain the only authoritative application count and its transition history. | Hasif: persistence mechanism if approved. | Occupancy state machine and recovery rules. | Four event classes, duplicate events and node resets produce expected count. |
| C1: sampling path before further ML optimisation | Integration gate owner. Do not declare readiness until the real sensor-to-event path works. | Whole team. | Readiness checklist and evidence index. | One real entrance event reaches classification, occupancy and output on a fixed commit. |
| D2 event logging interface | Approve the application/logging boundary, record fields and failure semantics; keep logging below application priority and outside SchedulerTask. | Brendan/JY: SPI/W25Q32 transport and device layer. | Review of `D2_EVENT_RECORD_DESIGN.md`; align EventRecord and flash record IDs/classes. | T-20/T-23: readback, CRC/recovery, queue diagnostics and no sampling disruption. |

## 3. Changes Cody makes directly

### 3.1 Freeze the event contract

The sensor node creates one entrance event. MQTT carries the event as one record; the processing node does not reconstruct the window from raw 20 Hz arrival order.

Minimum logical fields:

| Field | Rule |
|---|---|
| `version` | Reject unsupported versions. |
| `node_id` | Identifies the sensor node that formed the window. |
| `boot_id` | Changes on each source-node boot. |
| `sequence` | Increments once when a window becomes an event, before queueing. |
| `event_id` | Stable tuple `(node_id, boot_id, sequence)`. |
| `window_start_us`, `window_end_us` | Monotonic sensor-node timestamps; end must not precede start. |
| `features` | Fixed, bounded, versioned feature vector agreed with Jun Yong. |
| `quality_flags` | At minimum: complete, timed-out, sensor fault, stale and degraded. |
| `rule_class` | Entry, exit, incomplete/turn-around, false/object or unavailable. |
| `model_version` | Stable identifier or explicit unavailable value. |

The wire format and flash-log format may differ, but they must preserve the same event identity and class semantics. No compiler-dependent struct image is sent or stored.

### 3.2 Implement ApplicationTask behaviour

The intended processing order is:

1. Validate the EventRecord version, length, time range, class/feature bounds and quality.
2. Check the bounded de-duplication set using the complete event ID.
3. Obtain the configured rule or ML result; apply the agreed fallback if ML is late, absent or invalid.
4. Apply one idempotent occupancy transition:
   - entry: `count + 1`;
   - exit: `max(0, count - 1)`;
   - incomplete/turn-around: unchanged;
   - false/object: unchanged.
5. Mark the event committed in de-duplication state.
6. Attempt to enqueue the classified event/count for MQTT.
7. Send latest-value display output without blocking the application path.
8. Offer the event to the dedicated D2 log queue if logging is enabled.
9. Record timing and outcome counters through BenchmarkTask.

An LCD, MQTT or flash failure is DEGRADED and must not undo or repeat an already committed occupancy transition.

### 3.3 Decide reset behaviour

The team must choose one policy during Tribunal/Zoom:

- **Volatile count:** reset count to zero and start a new `boot_id`; simplest, but loss after reset is explicit.
- **Persistent count:** atomically store count plus the last committed event identity, version and CRC; required if FR18 is interpreted across resets.
- **Authoritative external restore:** restore from a trusted retained service/log with a defined reconciliation protocol.

Cody recommends persistent count only if the requirement explicitly includes resets. Persistence adds flash wear, atomic-update and recovery tests and must not be assumed for free.

### 3.4 Define and interpret integrated evidence

Sitt and Hasif own the low-level hardware/runtime benchmarking implementation. Cody defines which measurements are needed for the application contract and interprets their consequences. For each full integration run Cody requires and assesses:

- event count and event rate;
- event-assembly to classification latency: mean, p95, p99 and maximum;
- ApplicationTask execution p95/p99/maximum;
- deadline misses and stale/duplicate rejections;
- `app_q`, `pub_q`, `bench_q` and D2 `log_q` current/high-water/drop counts;
- minimum task stack, free/minimum heap and PSRAM;
- resets, watchdog events and every DEGRADED/Critical fault.

Benchmark producers must remain non-blocking. If `bench_q` is full, the implementation must expose a dropped-measurement counter so Cody does not accept evidence that silently looks better than reality.

## 4. Changes Cody specifies but another owner implements

| Module | Cody specifies | Module owner implements |
|---|---|---|
| GPIO/HC-SR04 | Maximum acquisition latency, timeout result, timestamp and quality semantics. | Non-blocking edge/RMT/MCPWM capture and electrical test. |
| SchedulerTask | Observation-window states, trigger order and EventRecord hand-off requirement. | Scheduling mechanics, queueing and WCET-safe implementation. |
| UART/LD2450 | Presence event, expiry and degraded behaviour. | Continuous RX drain, buffer sizing, parser and overflow counter. |
| MQTT | Versioned event topic/payload, duplicate/replay behaviour and publish outcome required by ApplicationTask. | Routing, QoS, reconnect and validation. |
| InferenceTask | Event/result correlation, deadline, fallback and class/confidence contract. | Model feature mapping, invocation and model lifecycle. |
| SPI/W25Q32 | Which committed application fields are logged and how log failure affects occupancy. | Device commands, JEDEC validation, storage layout, CRC/readback and recovery. |
| Benchmarking | Application-level metrics, acceptance thresholds and required evidence metadata. | Sitt implements hardware/sensor measurements; Hasif implements RTOS/task/queue/platform measurements. |

## 5. Implementation and decision order

1. Team approves Cody as D6 application/integration owner and confirms the dependency owners above.
2. Freeze EventRecord v1, event identity and time basis with Brendan, Hasif and Jun Yong.
3. Decide classifier selection/fallback and the TSL2561/GY-511 role.
4. Decide de-duplication capacity and reset/persistence policy.
5. Implement and host-test the pure-C occupancy state machine and EventRecord validation before hardware integration.
6. Connect SchedulerTask/Inferences to a typed `app_q`; remove the anonymous 64-byte payload.
7. Add bounded MQTT/display/log requests and diagnostics.
8. Execute [D3_TASKS.md](D3_TASKS.md) for F5/F7 timing changes and coordinate the F8 radar service with its owners.
9. Have Sitt/Hasif run the agreed hardware/runtime measurements; Cody interprets WCET and EventRecord replay results before accepting the real entrance path.
10. Update `WEEK6_FEEDBACK_RESPONSE.md` statuses only from retained evidence.

## 6. Week 8 pre-Tribunal entry for Cody

**AI prompt used**

> When the embedded-system requirements change, trace the consequences through my ApplicationTask, BenchmarkTask, occupancy state, EventRecord, rule/ML selection, MQTT, LCD and SPI logging. Identify memory, processing, code-size, timing, concurrency, persistence and hardware trade-offs. Separate changes I own from interfaces that require another owner, and state the evidence needed before accepting the change.

**Useful AI ideas**

- Trace a requirement through contracts, resources, timing/concurrency, failure behaviour and tests rather than changing nearby code blindly.
- Use one fixed, versioned EventRecord and a stable event ID.
- Make occupancy transitions idempotent under MQTT duplicates and reconnect replay.
- Keep LCD, MQTT and flash work outside the time-critical sensor path.
- Report percentiles, maxima, deadline misses and queue/resource high-water marks rather than only averages.

**Weak or unrealistic AI ideas**

- Increasing every queue wastes RAM and can hide overload while adding latency.
- Adding persistence without a reset requirement creates flash-wear and recovery complexity.
- Moving sensor acquisition, UART parsing or ML ownership into ApplicationTask breaks the agreed module boundaries.
- Claiming a non-blocking design without target-board WCET, crosstalk and fault evidence is not defensible.

**Position before Zoom**

Cody accepts ownership of the D3 timing contract, the EventRecord/ApplicationTask/occupancy/output contract, and the acceptance decision for integrated behaviour. The sensor node should assemble the observation window and publish one identified event. ApplicationTask should validate and process it exactly once, update occupancy through a deterministic state machine and issue non-blocking outputs. Hardware capture, UART parsing, scheduling mechanics, MQTT/platform internals, model execution, W25Q32 commands and low-level benchmark implementation remain with their module owners. Cody coordinates those interfaces and interprets their evidence. No open item is marked complete until its stated test evidence exists.

## 7. Requirement-change answer

If a requirement changes, Cody will:

1. Clarify the metric, units, deadline, operating case and acceptance test.
2. Identify which EventRecord field, state transition, queue, output or persistence rule changes.
3. Recalculate fixed RAM, flash, CPU, queue and end-to-end timing budgets.
4. Check blocking, shared-bus, duplicate, reset, stale-data and partial-failure effects.
5. Change the smallest set of owned modules and negotiate explicit interface changes with other owners.
6. Reject or defer unsafe/unmeasurable requests with the limiting evidence and a bounded alternative.
7. Prove the decision on the target hardware using the updated test and feedback tracker.

