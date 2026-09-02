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

## 0x03 GPU Performance Optimization Highlights
libHPC includes GPU-accelerated kernels optimized for high-throughput computation on NVIDIA CUDA-compatible devices:
- **Radix-Sort Kernel:** Processes 500M elements in ~360ms on an RTX 3080 Ti(laptop), sustaining ~1.39B elements/sec throughput.  
- **Warp-Synchronous & Tiled Memory Layouts:** Maximizes shared memory utilization and minimizes global memory latency.  
- **Concurrent GPU Pipelines:** Supports asynchronous kernel launches and stream-based scheduling for overlapping compute and memory operations.  
- **Profiling & Validation:** Includes tools for warp efficiency, memory access analysis, and synchronization correctness across GPU architectures.  
- **Realistic HPC Throughput:** Designed for bulk-parallel computation and scientific workloads, **not** real-time ultra-low-latency trading systems.
