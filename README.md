# Coreforge: High-Performance Computing Core
## Platform Support

Coreforge provides SIMD-optimized kernels, concurrent and sparse data
structures, GPU utilities, and HPC-oriented memory management components.

---

## 0x00 Platform Support
| Platform | Status |
|---------|--------|
| Linux (x86_64 / CUDA) | ✓ Supported |
| Windows (MSVC / CUDA) | ✓ Supported |
| macOS (Intel) | ✓ CPU modules; oneTBB/CUDA disabled |
| macOS (Apple Silicon / ARM64) | ✓ CPU modules; portable sparse backend |

---

## 0x01 macOS and Apple Silicon

Apple builds use standard C++ fallbacks for sparse containers and serial
execution when OpenMP is unavailable. oneTBB and CUDA are forcibly excluded
from Apple targets, even if a stale CMake cache attempts to enable them.

Tests and benchmarks default to off on Apple to keep the base CPU build free of
test-framework downloads. They can be requested explicitly with
`-DLIBHPC_BUILD_TESTING=ON`.

---

## 0x02 Optional Backends

- `LIBHPC_ENABLE_TBB`: enabled by default on non-Apple platforms.
- `LIBHPC_ENABLE_CUDA`: probes CUDA on non-Apple platforms when enabled.
- `LIBHPC_BUILD_TESTING`: builds tests and benchmarks; defaults to off on Apple.

---

## 0x03 Debug Sanitizer Builds

Coreforge supports separate Debug build directories for AddressSanitizer and
ThreadSanitizer on Linux and macOS. ASan and TSan cannot be combined, so use a
different build directory for each one.

`LIBHPC_SANITIZER` accepts the following values:

| Value | Debug instrumentation |
|-------|-----------------------|
| `address` | AddressSanitizer (`address,leak` on Linux; `address` on macOS) |
| `thread` | ThreadSanitizer |
| `none` | No sanitizer |

Sanitizer flags are applied only to the Debug configuration. Release,
RelWithDebInfo, and MinSizeRel builds remain uninstrumented so that benchmark
timings are not distorted.

Configure, build, and run the ASan tests:

```sh
cmake -S . -B build-asan \
  -DCMAKE_BUILD_TYPE=Debug \
  -DLIBHPC_BUILD_TESTING=ON \
  -DLIBHPC_SANITIZER=address
cmake --build build-asan -j
ctest --test-dir build-asan --output-on-failure
```

Configure, build, and run the TSan tests:

```sh
cmake -S . -B build-tsan \
  -DCMAKE_BUILD_TYPE=Debug \
  -DLIBHPC_BUILD_TESTING=ON \
  -DLIBHPC_SANITIZER=thread
cmake --build build-tsan -j
ctest --test-dir build-tsan --output-on-failure
```

For a Debug build without instrumentation, use
`-DLIBHPC_SANITIZER=none`. Sanitizer selection is not currently configured for
Windows builds.

---

## 0x04 GPU Performance Optimization Highlights
libHPC includes GPU-accelerated kernels optimized for high-throughput computation on NVIDIA CUDA-compatible devices:
- **Radix-Sort Kernel:** Processes 500M elements in ~360ms on an RTX 3080 Ti(laptop), sustaining ~1.39B elements/sec throughput.  
- **Warp-Synchronous & Tiled Memory Layouts:** Maximizes shared memory utilization and minimizes global memory latency.  
- **Concurrent GPU Pipelines:** Supports asynchronous kernel launches and stream-based scheduling for overlapping compute and memory operations.  
- **Profiling & Validation:** Includes tools for warp efficiency, memory access analysis, and synchronization correctness across GPU architectures.  
- **Realistic HPC Throughput:** Designed for bulk-parallel computation and scientific workloads, **not** real-time ultra-low-latency trading systems.
