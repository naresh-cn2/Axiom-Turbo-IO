# Axiom Turbo: High-Performance Systems Engine 🧱🔥

A custom-engineered C-engine designed to abolish the "Python Object Tax" in high-volume data pipelines. By moving ingestion to the metal, Axiom achieves near-theoretical throughput limits of NVMe hardware.

## 🚀 The 3.0 GB/s Barrier Broken

Latest benchmark run on consumer-grade hardware (**Acer Nitro 16 | Ryzen 7 7840HS**).

| Metric | Python (Standard) | Axiom Turbo (C) | Speedup |
| :--- | :--- | :--- | :--- |
| **Throughput** | ~0.16 GB/s | **3.06 GB/s** | **19.1x** |
| **10M Row Latency** | 0.87s | **0.19s** | **78.1% reduction** |
| **RAM Footprint** | ~1.9 GB | **~2 MB** | **99.9% reduction** |

## 🏗️ Technical Architecture

- **SIMD Vectorization:** Utilizes `memchr` for 32-byte chunk processing per CPU cycle via AVX2/AVX-512.
- **Parallel mmap:** Direct-to-kernel memory mapping to eliminate user-space copy overhead.
- **Hardware Alignment:** Distributed workload across 16 logical threads to saturate the memory bus.
- **Boundary Hardening:** Thread-safe Skip-and-Overlap logic to ensure zero data loss during parallel ingestion.

## 🛠️ Build & Run

```bash
# Extreme Optimization Compile
gcc -O3 -march=native -funroll-loops axiom_turbo.c -o axiom_turbo -lpthread

# Execute Benchmark
./axiom_turbo your_data.csv