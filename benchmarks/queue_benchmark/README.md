# Queue latency benchmark

This benchmark reports per-element latency distributions separately from the
existing saturation-throughput benchmark. A whole 100M-item run has one wall
time and one throughput value; it does **not** have 100M latency observations
unless individual elements carry timestamps.

## Latency definitions

### SPSC handoff latency

One producer and one consumer keep exactly one element in flight. The producer
timestamps the element immediately before `push()`; the consumer records the
time immediately after the matching successful `pop()`.

This interval includes enqueue, publication, cross-core visibility, consumer
scheduling after publication, and dequeue. It does not include time before the
producer begins the enqueue attempt.

### 4P4C open-loop latency

Four producers generate a combined offered load of 1M handoffs/s. Producer
phases are staggered by 1 us, avoiding an artificial four-producer burst against
the same tail cache line every 4 us. A sampled element carries its scheduled
arrival time, not the time at which its producer happened to run. Therefore
producer scheduling delay, enqueue delay, queue residence, and dequeue all
remain visible instead of being hidden by coordinated omission.

The measured 1M/s is the configured workload rate, **not** the queue's maximum
throughput. Maximum throughput belongs to the separate saturation benchmark.

## Sampling and validation

Each test transfers 100,000,000 elements. After warm-up, one of every 100
handoffs is timestamped, producing 999,000 samples. Storing and sorting all
100M timestamps would consume roughly 800 MB before sort overhead and would
materially perturb the benchmark.

The reported p50/p90/p99/p99.9 values use the exact nearest-rank element after
sorting the 999,000 sampled latencies. The 4P4C run also validates total count,
sum, XOR, sample count, and completion of every producer and consumer.

## Reproduction

```sh
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DLIBHPC_BUILD_BENCHMARK=ON \
  -DLIBHPC_BUILD_TESTING=OFF
cmake --build build --target queue_benchmark -j

./build/benchmarks/queue_benchmark/queue_benchmark \
  --benchmark_filter='BM_Queue_(SPSC|4P4C_OpenLoop)_HandoffLatency'
```

For a quick correctness smoke test without changing the default 100M workload:

```sh
COREFORGE_QUEUE_LATENCY_ITEMS=1000000 \
./build/benchmarks/queue_benchmark/queue_benchmark \
  --benchmark_filter='BM_Queue_(SPSC|4P4C_OpenLoop)_HandoffLatency'
```

## Apple M5 Pro result — 2026-10-01

Environment: 15-core Apple M5 Pro, 24 GB memory, Release build, macOS,
unpinned threads. Google Benchmark could not read `hw.cpufrequency` on Apple
Silicon; its displayed `15 X 24 MHz` metadata is not the processor frequency
and does not affect the measured `steady_clock` intervals.

| Workload | Queue | p50 | p90 | p99 | p99.9 | max | Observed rate |
|---|---|---:|---:|---:|---:|---:|---:|
| SPSC, one in flight | bounded circular | 167 ns | 208 ns | **209 ns** | 375 ns | 43.958 us | 4.154M/s |
| SPSC, one in flight | reference-counted linked | 542 ns | 584 ns | **709 ns** | 1.000 us | 78.459 us | 1.515M/s |
| 4P4C open-loop, 1M/s | bounded circular | 500 ns | 542 ns | **667 ns** | 64.834 us | 4.386 ms | 0.99999M/s |
| 4P4C open-loop, 1M/s | reference-counted linked | 1.667 us | 2.417 us | **3.625 us** | 439.500 us | 5.560 ms | 0.99999M/s |

The SPSC row is the cleanest implementation-to-implementation handoff
comparison. The open-loop 4P4C row answers a different question: tail latency
under the same arrival rate and producer/consumer contention. Neither should be
relabelled as the latency of the entire queue run.

