# Group 15 pre-tribunal review

Reviewed 2026-10-18. Source: `/Users/tanyilin/Downloads/15_PreTri1_final.pdf` (11 pages, no embedded annotations). This addendum records repository review findings. It does not record new lecturer feedback, team agreement, live negotiation or a Version B/C outcome. The supplied PDF is unchanged.

## Corrections to the pre-tribunal wording

1. **Oversized messages (PDF pp. 4, 5, 10; P1).** Replace the blanket claim that oversized messages are refused with: "Malformed inputs have negative-test evidence; inbound text exceeding 2,047 bytes is currently truncated before queueing. Whole-message rejection and preservation of running state for an oversized valid prefix remain to be verified." Evidence: `libraries/EdgeS3/src/mqtt_s3.cpp`, inbound callback. Hasif and Brendan own the fix.
2. **Queue allocation (p. 7; P2).** Replace "queues statically sized" with "queues have fixed capacities and are allocated dynamically at startup using xQueueCreate." Add allocation-failure handling as an open runtime task. Fixed capacity alone does not establish reliable startup. Evidence: `libraries/EdgeS3/src/runtime_s3.cpp`, `runtime_start`. Hasif owns this.
3. **Inference ownership (pp. 5, 8; P3).** Label the InferenceTask interface as the target design. Today InferenceTask and ApplicationTask only receive/discard placeholder queue items; SchedulerTask owns active inference state and MQTTTask calls the model transfer module. Working model inference/upload does not prove the proposed asynchronous pipeline. Jun Yong, Hasif and Cody must validate the handover.
4. **Direction (p. 5; P4).** Replace "direction comes from which ultrasonic sensor fires first" with "direction is inferred from timestamped target detection or crossing order, subject to quality and ambiguity rules." Scheduled trigger order is controlled by software and cannot establish travel direction. Brendan/Sitt define capture and crosstalk behaviour; Cody validates entry/exit/reversal outcomes.
5. **Event format and reset integrity (pp. 8, 11; P5).** The proposed 32-byte SPI record is a compact result summary, not the full feature-bearing sensor EventRecord. It lacks the source node component of the proposed event identity. Freeze identity, dedup retention and rollover, atomic count/checkpoint recovery and torn-write behaviour before claiming occupancy survives a reset. Cody/Brendan and D2 owners share this contract.
6. **Benchmark interface (pp. 7-8; P6).** Add a Cody-owned BenchmarkTask provider/consumer row. Inputs should identify the event, stage, timestamps and clock domain; outputs should report latency distributions, missing/deadline counts and drops. Current `bench_msg_t` records name plus duration. Cross-node monotonic clocks require a correlation or synchronization method before calculating end-to-end latency.
7. **Window ownership (pp. 5, 8, 10; P7).** Use one statement throughout: "Sensor-node assembly is the proposed integration contract. Implementation and team ratification remain pending." Do not describe the location as both decided and undecided. Brendan builds the event; Cody owns integration integrity.
8. **Evidence scope (pp. 4-6, 9-11; P8).** Keep offline replay, event deduplication, occupancy persistence, W25Q32 logging and three-board end-to-end operation unverified. Reconnection, model CRC tests and SPI loopback are narrower evidence. FR21 still needs an explicit offline event policy.

## Timing and integration priorities

The current `measure_pulse_us` busy-waits for a rising edge and then for a falling edge. No-rise consumes one configured timeout; a late rise followed by stuck-high can approach two timeout periods plus overhead. The stuck-high path returns a clipped width, the known D-07 validity gap. Two no-echo sensors therefore do not establish the worst-case bound. Capture must become non-blocking, with invalid fault outcomes, before claiming the paired-sensor 50 ms budget fits.

Run the existing `parity/wcet_test.py` and `parity/timing_test.py`, then extend measurement to radar, paired real ultrasonics, I2C, inference, model upload, MQTT outages and external logging together. Record build, wiring, duration, p95/p99/max, deadline/skipped counts, queue high-water/drops and stack/heap. An observed maximum is not a proven WCET bound.

The LD2450 still needs a named continuous UART drainer, RX capacity, maximum service gap, parser recovery and a presence rule, verified on the actual radar. The light sensor is unavailable in the recorded kit; retaining it as a model feature requires either restored hardware or a versioned missing-feature contract and matching training. The 16/17-feature mismatch remains open.

Freeze reset and offline semantics before implementing occupancy persistence: event ID generation, bounded duplicate retention, sequence rollover, delayed events after reset, count/checkpoint consistency and recovery when storage is absent or corrupt. Avoid claiming exactly-once effects from QoS 1 delivery alone. Output/log failures must remain observable without blocking acquisition.

## Ownership and document updates

Group ID is 15, as stated on PDF p. 1. The historical deliverable split and tribunal subsystem split are different: Cody retains historical D3 accountability and owns integration/occupancy/outputs/BenchmarkTask; Brendan owns scheduler/configuration/sequence/event assembly; Sitt owns physical sensors and capture drivers; Jun Yong owns dataset/models/inference; Hasif owns platform/HAL/runtime/MQTT. No historical deliverable assignment was silently replaced.

`D3_TASKS.md` now lists eight open actions with owners and acceptance evidence. `WEEK6_FEEDBACK_RESPONSE.md` records P1-P8 separately from the original lecturer quotations. `D2_EVENT_RECORD_DESIGN.md` now distinguishes its compact summary from the full event identity. Original feedback statuses are unchanged because no hardware testing or implementation was performed in this review.

Use this review to correct Version A and prepare discussion. Human negotiation, the requirement-change response and subsequent gap-check outcomes must be recorded when they actually occur.
