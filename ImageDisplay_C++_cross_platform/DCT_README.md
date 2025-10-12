# DCT-Based Image Compression Application

This application implements a DCT-based image encoder/decoder that simulates JPEG-like compression with progressive delivery modes.

## Building

```bash
cd ImageDisplay_C++_cross_platform
cmake --build build --target DCTApp
```

## Usage

```bash
./build/DCTApp InputImage quantizationLevel DeliveryMode Latency
```

### Parameters

- **InputImage**: Path to the .rgb input image file (352x288 format)
- **quantizationLevel** (N): Integer from 0-7 controlling compression
  - 0 = no quantization (lossless)
  - Higher values = more compression (2^N quantization)
- **DeliveryMode** (M): Integer 1, 2, or 3
  - 1 = Baseline (sequential block-by-block)
  - 2 = Progressive Spectral Selection (DC first, then AC coefficients)
  - 3 = Progressive Successive Bit Approximation (bit-plane refinement)
- **Latency** (L): Milliseconds delay between decode steps
  - 0 = instant (no latency simulation)
  - Higher values = slower progressive display

## Example Commands

### No quantization, baseline mode, instant display

```bash
./build/DCTApp ../lake-forest_352x288.rgb 0 1 0
```

### Medium quantization, baseline with latency

```bash
./build/DCTApp ../lake-forest_352x288.rgb 3 1 100
```

### Low quantization, spectral selection with latency

```bash
./build/DCTApp ../lake-forest_352x288.rgb 1 2 100
```

### Medium quantization, successive bit approximation with latency

```bash
./build/DCTApp ../lake-forest_352x288.rgb 2 3 50
```

## How It Works

### Encoder

1. Separates image into R, G, B channels
2. Divides each channel into 8x8 blocks
3. Applies DCT to each block (spatial → frequency domain)
4. Quantizes coefficients using uniform quantization table (all entries = 2^N)
5. Stores quantized DCT coefficients

### Decoder

Depending on the delivery mode:

**Mode 1 - Baseline (Sequential)**

- Decodes one block at a time, left-to-right, top-to-bottom
- Each latency period = one block decoded

**Mode 2 - Spectral Selection (Progressive)**

- First pass: DC coefficients only (all blocks)
- Subsequent passes: Add AC coefficients progressively (AC1, AC2, ... AC63)
- Each latency period = one coefficient added for all blocks

**Mode 3 - Successive Bit Approximation (Progressive)**

- Decodes all blocks using all coefficients
- First pass: Most significant bit only
- Subsequent passes: Add next bit of precision
- Each latency period = one additional bit of precision

### Display

- Left side: Original image
- Right side: Decoded image (updates progressively with latency > 0)

## Implementation Details

- DCT follows standard formula with cosine basis functions
- Quantization: F'[u,v] = round(F[u,v] / 2^N)
- Dequantization: F[u,v] = F'[u,v] \* 2^N
- No entropy coding (RLE/Huffman) - focuses on DCT/quantization effects
- No chroma subsampling - operates on RGB channels equally

## Files

- `src/DCT.h` - DCT encoder/decoder interface
- `src/DCT.cpp` - DCT implementation with forward/inverse DCT and progressive decoding
- `src/DCTApp.cpp` - wxWidgets GUI application with command-line interface
- `CMakeLists.txt` - Build configuration (updated to include DCTApp target)
