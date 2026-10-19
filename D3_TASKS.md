# D3 timing and integration tasks

Updated 2026-10-18 from `15_PreTri1_final.pdf` and the current repository. This is an actionable work list, not implementation evidence. Historical D3 deliverable owner: Cody. Tribunal subsystem lead: Brendan for scheduler/configuration/event assembly; Sitt for capture drivers and physical sensors; Cody for integration and acceptance.

| ID | Action | Owner | Completion evidence |
|---|---|---|---|
| D3-01 | Replace blocking echo waits with timestamped capture/edge notifications; report no-rise and stuck-high as invalid, addressing D-07. Keep shared application logic in C behind the HAL. | Brendan + Sitt | Real pulse, no-rise, late-rise/stuck-high, disconnect and timeout logs; SchedulerTask continues servicing other work. |
| D3-02 | Specify trigger arbitration, minimum spacing, maximum range/timeouts and per-sensor effective rate. Distinguish scheduled trigger order from observed crossing order; do not promise two valid readings every 50 ms until measured. | Brendan + Sitt | Agreed timing diagram plus paired-sensor entry/exit/reversal and crosstalk measurements. |
| D3-03 | Assign LD2450 UART drain owner, RX capacity, maximum service gap and parser/presence rules. Generic UART loopback is not radar evidence. | Sitt + Brendan | Actual 256 kbaud frame-loss/overflow, recovery and presence-latency logs under load. |
| D3-04 | Freeze periodic, one-shot, event-driven and logical-sequence semantics: start/cancel/retrigger, deadlines, timeout, invalid input and configuration replacement. | Brendan; Cody acceptance | Deterministic replay traces for each mode including cancellation and overload. |
| D3-05 | Build observation windows on the sensor node; freeze event schema/version, node/boot/sequence identity, time units, feature order and validity flags. Verify serialized size fits queue slots or resize typed queues explicitly. | Brendan + Cody + Jun Yong | Versioned C header, byte vectors/size assertions and duplicate/delayed/missing-sample replay. Existing infer_q/app_q items are only 64 bytes. |
| D3-06 | Reject oversized config messages before parsing and define atomic validation/commit/rollback for multi-field changes. | Brendan + Hasif | Oversized valid-prefix, malformed update and failed-device-init cases preserve the prior running state. |
| D3-07 | Run existing wcet_test.py and timing_test.py, then full entrance load including late-rise/stuck-high, radar, I2C, model activity, network faults and external flash. | Brendan + Hasif; Cody system evidence | Raw logs with p95/p99/max, skipped/deadline counts, queue high-water/drops, stack/heap, build/config and wiring. Observed max is not a proven WCET bound. |
| D3-08 | Define event-correlated benchmark interface, per-stage timestamps and clock domains. Do not subtract unsynchronised node uptimes. | Cody + Hasif | Producer/consumer map; trace one event through acquisition, inference, application and output; report end-to-end percentiles and loss. |

All tasks above are open. Existing per-sensor timing and WCET instrumentation are partial foundations, not full-load acceptance.

Cody's dependent integration decisions: dedup retention/rollover, count recovery after reset, late/invalid ML fallback, FR21 offline policy and LCD/log failures. Keep these with [the feedback tracker](WEEK6_FEEDBACK_RESPONSE.md) F9/F10/R5; do not silently choose team reset or data-loss semantics.

See [PRETRI1_REVIEW.md](PRETRI1_REVIEW.md) for the PDF corrections and [D2_EVENT_RECORD_DESIGN.md](D2_EVENT_RECORD_DESIGN.md) for the separate provisional flash summary.
