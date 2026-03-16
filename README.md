# libHPC: High-Performance Computing Core

![License](https://img.shields.io/badge/License-NON--COMMERCIAL-red.svg)
![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20Windows-blue.svg)

libHPC is a high-performance computing library focused on Linux and Windows environments. It provides SIMD-optimized kernels, concurrent data structures, GPU utilities, and HPC-oriented memory management components.

---

## 🛑 STRICT LICENSE & USAGE TERMS / 授权与使用条款

**PLEASE READ CAREFULLY BEFORE PROCEEDING.**

1.  **INTERVIEW EVALUATION ONLY**: This codebase is provided **STRICTLY** for educational purposes or individual technical interview evaluation. Any other use is a violation of Intellectual Property.
2.  **NON-COMMERCIAL ONLY**: Any commercial use, including but not limited to integration into proprietary trading systems, HFT frameworks, or industrial HPC clusters, is **STRICTLY PROHIBITED**.
3.  **NO DERIVATIVE WORKS**: You may not modify, distribute, or create derivative works based on these SIMD/CUDA kernels for corporate gain.
4.  **MONITORING & ENFORCEMENT**: The author actively monitors repository access logs and visitor metadata (including LinkedIn referral tracking). Unauthorized commercial exploitation identified via logic-pattern matching or binary analysis will be met with **immediate legal action and public disclosure of the infringing entity.**

---

## 0x00 Platform Support

| Platform | Status |
| :--- | :--- |
| **Linux (x86_64 / CUDA)** | ✓ Supported |
| **Windows (MSVC / CUDA)** | ✓ Supported |
| **macOS (Intel)** | ✓ Supported (Legacy) |
| **macOS (Apple Silicon / ARM64)** | ✗ **NOT SUPPORTED** |

---

## 0x01 macOS Apple Silicon Notice

libHPC **does not support macOS ARM (Apple Silicon)**.

The reason is simple:
> **Apple’s recent macOS / Xcode toolchain updates introduced ABI changes in `libc++`, causing oneTBB and other HPC components to fail at link-time.**

These issues do not occur on Linux or Windows, and they did not occur on older macOS versions. Since the goal of libHPC is stable, reproducible high-performance computing, macOS ARM is excluded to avoid degraded reliability or performance.

---

## 0x02 macOS ARM Technical Post-Mortem

libHPC previously supported macOS ARM. However, recent Xcode toolchains explicitly mark several `libc++` ABI symbols as **FORBIDDEN** (Xcode displays a “prohibited symbol” icon). 

Specifically, **`std::__1::__hash_memory`**, a critical dependency for **oneTBB**, has been removed/hidden at the SDK level. 

Since this is a breaking change in the Apple SDK/Toolchain itself, it cannot be resolved within libHPC. As a result, macOS ARM support has been formally dropped to maintain the integrity of the HPC pipeline.

