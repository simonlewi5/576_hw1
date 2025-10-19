# DCT-Based Image Coder/Decoder (CS 576 A2)

Implements JPEG-like DCT encoding/decoding with three delivery modes:

1. Baseline (sequential)
2. Progressive – Spectral Selection (DC, AC1…AC63 in zig-zag order)
3. Progressive – Successive Bit Approximation (bit-plane refinement)

## Build

```
cmake -S . -B build
cmake --build build --target DCTApp -j
```

Requires: CMake, C++17, wxWidgets (core, base).

## Run

```
./build/DCTApp InputImage quantizationLevel DeliveryMode Latency
```

- InputImage: path to planar .rgb (352x288, 3 bytes/pixel, R-plane then G-plane then B-plane)
- quantizationLevel (N): 0–7; F'[u,v] = round(F[u,v] / 2^N)
  - N=0 ⇒ no quantization loss (tiny numeric diffs from DCT/IDCT may still occur)
- DeliveryMode (M): 1=Baseline, 2=Spectral Selection (zig-zag), 3=Successive Bit
- Latency (ms): delay between decode steps (0 = instant)

Examples:

```
./build/DCTApp ../lake-forest_352x288.rgb 0 1 0
./build/DCTApp ../lake-forest_352x288.rgb 3 1 100
./build/DCTApp ../lake-forest_352x288.rgb 1 2 100
./build/DCTApp ../lake-forest_352x288.rgb 2 3 50
```
