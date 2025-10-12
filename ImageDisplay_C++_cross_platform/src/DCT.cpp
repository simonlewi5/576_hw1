#include "DCT.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Forward DCT: spatial domain -> frequency domain
void forwardDCT(const double block[8][8], double dct[8][8])
{
    for (int u = 0; u < 8; u++)
    {
        for (int v = 0; v < 8; v++)
        {
            double sum = 0.0;
            for (int x = 0; x < 8; x++)
            {
                for (int y = 0; y < 8; y++)
                {
                    double cosX = cos((2.0 * x + 1.0) * u * M_PI / 16.0);
                    double cosY = cos((2.0 * y + 1.0) * v * M_PI / 16.0);
                    sum += block[x][y] * cosX * cosY;
                }
            }
            double Cu = (u == 0) ? (1.0 / sqrt(2.0)) : 1.0;
            double Cv = (v == 0) ? (1.0 / sqrt(2.0)) : 1.0;
            dct[u][v] = 0.25 * Cu * Cv * sum;
        }
    }
}

// Inverse DCT: frequency domain -> spatial domain
void inverseDCT(const double dct[8][8], double block[8][8])
{
    for (int x = 0; x < 8; x++)
    {
        for (int y = 0; y < 8; y++)
        {
            double sum = 0.0;
            for (int u = 0; u < 8; u++)
            {
                for (int v = 0; v < 8; v++)
                {
                    double Cu = (u == 0) ? (1.0 / sqrt(2.0)) : 1.0;
                    double Cv = (v == 0) ? (1.0 / sqrt(2.0)) : 1.0;
                    double cosX = cos((2.0 * x + 1.0) * u * M_PI / 16.0);
                    double cosY = cos((2.0 * y + 1.0) * v * M_PI / 16.0);
                    sum += Cu * Cv * dct[u][v] * cosX * cosY;
                }
            }
            block[x][y] = 0.25 * sum;
        }
    }
}

// Quantize DCT coefficients: F'[u,v] = round(F[u,v] / 2^N)
void quantizeBlock(const double dct[8][8], int quantized[8][8], int N)
{
    double divisor = pow(2.0, N);
    for (int u = 0; u < 8; u++)
    {
        for (int v = 0; v < 8; v++)
        {
            quantized[u][v] = static_cast<int>(round(dct[u][v] / divisor));
        }
    }
}

// Dequantize DCT coefficients: F[u,v] = F'[u,v] * 2^N
void dequantizeBlock(const int quantized[8][8], double dct[8][8], int N)
{
    double multiplier = pow(2.0, N);
    for (int u = 0; u < 8; u++)
    {
        for (int v = 0; v < 8; v++)
        {
            dct[u][v] = quantized[u][v] * multiplier;
        }
    }
}

// Extract 8x8 block from image channel
void extractBlock(const std::vector<uint8_t> &channel, int width, int height,
                  int blockX, int blockY, double block[8][8])
{
    for (int x = 0; x < 8; x++)
    {
        for (int y = 0; y < 8; y++)
        {
            int imgX = blockX * 8 + x;
            int imgY = blockY * 8 + y;
            if (imgX < width && imgY < height)
            {
                // Center the pixel values around 0 by subtracting 128
                block[x][y] = static_cast<double>(channel[imgY * width + imgX]) - 128.0;
            }
            else
            {
                block[x][y] = 0.0; // Padding for incomplete blocks
            }
        }
    }
}

// Insert 8x8 block back into image channel
void insertBlock(std::vector<uint8_t> &channel, int width, int height,
                 int blockX, int blockY, const double block[8][8])
{
    for (int x = 0; x < 8; x++)
    {
        for (int y = 0; y < 8; y++)
        {
            int imgX = blockX * 8 + x;
            int imgY = blockY * 8 + y;
            if (imgX < width && imgY < height)
            {
                // Add 128 back and clamp to [0, 255]
                double val = block[x][y] + 128.0;
                val = std::max(0.0, std::min(255.0, val));
                channel[imgY * width + imgX] = static_cast<uint8_t>(val);
            }
        }
    }
}

// Encode entire image: RGB -> DCT coefficients
void encodeImage(const std::vector<uint8_t> &rgb, int width, int height, int quantLevel,
                 DCTCoefficients &R_dct, DCTCoefficients &G_dct, DCTCoefficients &B_dct)
{
    // Calculate number of 8x8 blocks needed
    int numBlocksX = (width + 7) / 8;
    int numBlocksY = (height + 7) / 8;

    R_dct = DCTCoefficients(numBlocksX, numBlocksY);
    G_dct = DCTCoefficients(numBlocksX, numBlocksY);
    B_dct = DCTCoefficients(numBlocksX, numBlocksY);

    // Separate RGB channels
    std::vector<uint8_t> R(width * height), G(width * height), B(width * height);
    for (int i = 0; i < width * height; i++)
    {
        R[i] = rgb[3 * i];
        G[i] = rgb[3 * i + 1];
        B[i] = rgb[3 * i + 2];
    }

    // Process each block
    for (int by = 0; by < numBlocksY; by++)
    {
        for (int bx = 0; bx < numBlocksX; bx++)
        {
            int blockIdx = by * numBlocksX + bx;

            // Process R channel
            double rBlock[8][8], rDCT[8][8];
            int rQuant[8][8];
            extractBlock(R, width, height, bx, by, rBlock);
            forwardDCT(rBlock, rDCT);
            quantizeBlock(rDCT, rQuant, quantLevel);
            // Flatten to 1D (row-major order)
            for (int u = 0; u < 8; u++)
            {
                for (int v = 0; v < 8; v++)
                {
                    R_dct.blocks[blockIdx][u * 8 + v] = rQuant[u][v];
                }
            }

            // Process G channel
            double gBlock[8][8], gDCT[8][8];
            int gQuant[8][8];
            extractBlock(G, width, height, bx, by, gBlock);
            forwardDCT(gBlock, gDCT);
            quantizeBlock(gDCT, gQuant, quantLevel);
            for (int u = 0; u < 8; u++)
            {
                for (int v = 0; v < 8; v++)
                {
                    G_dct.blocks[blockIdx][u * 8 + v] = gQuant[u][v];
                }
            }

            // Process B channel
            double bBlock[8][8], bDCT[8][8];
            int bQuant[8][8];
            extractBlock(B, width, height, bx, by, bBlock);
            forwardDCT(bBlock, bDCT);
            quantizeBlock(bDCT, bQuant, quantLevel);
            for (int u = 0; u < 8; u++)
            {
                for (int v = 0; v < 8; v++)
                {
                    B_dct.blocks[blockIdx][u * 8 + v] = bQuant[u][v];
                }
            }
        }
    }
}

// DCTDecoder implementation
DCTDecoder::DCTDecoder(const DCTCoefficients &R, const DCTCoefficients &G, const DCTCoefficients &B,
                       int width, int height, int quantLevel, DeliveryMode mode)
    : R_dct_(R), G_dct_(G), B_dct_(B), width_(width), height_(height),
      quantLevel_(quantLevel), mode_(mode), currentStep_(0)
{

    // Calculate total steps based on mode
    int numBlocks = R.numBlocksX * R.numBlocksY;
    switch (mode_)
    {
    case BASELINE:
        totalSteps_ = numBlocks; // One step per block
        break;
    case SPECTRAL_SELECTION:
        totalSteps_ = 64; // 64 DCT coefficients (DC + 63 AC)
        break;
    case SUCCESSIVE_BIT:
        // Find maximum bit depth needed
        totalSteps_ = 16; // Assuming up to 16 bits is sufficient
        break;
    }
}

int DCTDecoder::getTotalSteps() const
{
    return totalSteps_;
}

void DCTDecoder::reset()
{
    currentStep_ = 0;
}

bool DCTDecoder::decodeStep(std::vector<uint8_t> &outputRGB)
{
    if (currentStep_ >= totalSteps_)
    {
        return false; // Already complete
    }

    switch (mode_)
    {
    case BASELINE:
        decodeBaseline(outputRGB);
        break;
    case SPECTRAL_SELECTION:
        decodeSpectralSelection(outputRGB);
        break;
    case SUCCESSIVE_BIT:
        decodeSuccessiveBit(outputRGB);
        break;
    }

    currentStep_++;
    return currentStep_ < totalSteps_; // Returns true if more steps remain
}

void DCTDecoder::decodeBaseline(std::vector<uint8_t> &outputRGB)
{
    // Decode one block at a time (left-to-right, top-to-bottom)
    int blockIdx = currentStep_;
    int numBlocks = R_dct_.numBlocksX * R_dct_.numBlocksY;

    if (blockIdx >= numBlocks)
        return;

    int bx = blockIdx % R_dct_.numBlocksX;
    int by = blockIdx / R_dct_.numBlocksX;

    // Initialize output if first step
    if (currentStep_ == 0)
    {
        outputRGB.resize(width_ * height_ * 3, 0);
    }

    // Separate channels
    std::vector<uint8_t> R(width_ * height_), G(width_ * height_), B(width_ * height_);
    for (int i = 0; i < width_ * height_; i++)
    {
        R[i] = outputRGB[3 * i];
        G[i] = outputRGB[3 * i + 1];
        B[i] = outputRGB[3 * i + 2];
    }

    // Decode this block for each channel
    for (int ch = 0; ch < 3; ch++)
    {
        const DCTCoefficients &dct = (ch == 0) ? R_dct_ : (ch == 1) ? G_dct_
                                                                    : B_dct_;
        std::vector<uint8_t> &channel = (ch == 0) ? R : (ch == 1) ? G
                                                                  : B;

        // Unflatten coefficients
        int quant[8][8];
        for (int u = 0; u < 8; u++)
        {
            for (int v = 0; v < 8; v++)
            {
                quant[u][v] = dct.blocks[blockIdx][u * 8 + v];
            }
        }

        // Dequantize and inverse DCT
        double dctBlock[8][8], spatialBlock[8][8];
        dequantizeBlock(quant, dctBlock, quantLevel_);
        inverseDCT(dctBlock, spatialBlock);
        insertBlock(channel, width_, height_, bx, by, spatialBlock);
    }

    // Recombine channels
    for (int i = 0; i < width_ * height_; i++)
    {
        outputRGB[3 * i] = R[i];
        outputRGB[3 * i + 1] = G[i];
        outputRGB[3 * i + 2] = B[i];
    }
}

void DCTDecoder::decodeSpectralSelection(std::vector<uint8_t> &outputRGB)
{
    // Decode all blocks using coefficients from DC to current step
    int numCoefs = currentStep_ + 1; // 0 = DC only, 1 = DC + AC0, etc.

    outputRGB.resize(width_ * height_ * 3, 0);

    // Separate channels
    std::vector<uint8_t> R(width_ * height_), G(width_ * height_), B(width_ * height_);

    int numBlocks = R_dct_.numBlocksX * R_dct_.numBlocksY;

    for (int blockIdx = 0; blockIdx < numBlocks; blockIdx++)
    {
        int bx = blockIdx % R_dct_.numBlocksX;
        int by = blockIdx / R_dct_.numBlocksX;

        // Decode each channel
        for (int ch = 0; ch < 3; ch++)
        {
            const DCTCoefficients &dct = (ch == 0) ? R_dct_ : (ch == 1) ? G_dct_
                                                                        : B_dct_;
            std::vector<uint8_t> &channel = (ch == 0) ? R : (ch == 1) ? G
                                                                      : B;

            // Use only first numCoefs coefficients (in zig-zag order, but we'll use row-major for simplicity)
            int quant[8][8] = {0};
            for (int i = 0; i < numCoefs && i < 64; i++)
            {
                int u = i / 8;
                int v = i % 8;
                quant[u][v] = dct.blocks[blockIdx][i];
            }

            // Dequantize and inverse DCT
            double dctBlock[8][8], spatialBlock[8][8];
            dequantizeBlock(quant, dctBlock, quantLevel_);
            inverseDCT(dctBlock, spatialBlock);
            insertBlock(channel, width_, height_, bx, by, spatialBlock);
        }
    }

    // Recombine channels
    for (int i = 0; i < width_ * height_; i++)
    {
        outputRGB[3 * i] = R[i];
        outputRGB[3 * i + 1] = G[i];
        outputRGB[3 * i + 2] = B[i];
    }
}

void DCTDecoder::decodeSuccessiveBit(std::vector<uint8_t> &outputRGB)
{
    // Use only the most significant (currentStep + 1) bits of each coefficient
    int numBits = currentStep_ + 1;

    outputRGB.resize(width_ * height_ * 3, 0);

    // Separate channels
    std::vector<uint8_t> R(width_ * height_), G(width_ * height_), B(width_ * height_);

    int numBlocks = R_dct_.numBlocksX * R_dct_.numBlocksY;

    for (int blockIdx = 0; blockIdx < numBlocks; blockIdx++)
    {
        int bx = blockIdx % R_dct_.numBlocksX;
        int by = blockIdx / R_dct_.numBlocksX;

        // Decode each channel
        for (int ch = 0; ch < 3; ch++)
        {
            const DCTCoefficients &dct = (ch == 0) ? R_dct_ : (ch == 1) ? G_dct_
                                                                        : B_dct_;
            std::vector<uint8_t> &channel = (ch == 0) ? R : (ch == 1) ? G
                                                                      : B;

            // Mask coefficients to use only most significant bits
            int quant[8][8];
            for (int u = 0; u < 8; u++)
            {
                for (int v = 0; v < 8; v++)
                {
                    int coef = dct.blocks[blockIdx][u * 8 + v];

                    // Extract sign and magnitude
                    int sign = (coef < 0) ? -1 : 1;
                    int mag = abs(coef);

                    // Keep only most significant numBits bits
                    // Find highest bit position
                    int highBit = 0;
                    for (int b = 15; b >= 0; b--)
                    {
                        if (mag & (1 << b))
                        {
                            highBit = b;
                            break;
                        }
                    }

                    // Create mask for numBits starting from highBit
                    int maskedMag = 0;
                    for (int b = 0; b < numBits && (highBit - b) >= 0; b++)
                    {
                        if (mag & (1 << (highBit - b)))
                        {
                            maskedMag |= (1 << (highBit - b));
                        }
                    }

                    quant[u][v] = sign * maskedMag;
                }
            }

            // Dequantize and inverse DCT
            double dctBlock[8][8], spatialBlock[8][8];
            dequantizeBlock(quant, dctBlock, quantLevel_);
            inverseDCT(dctBlock, spatialBlock);
            insertBlock(channel, width_, height_, bx, by, spatialBlock);
        }
    }

    // Recombine channels
    for (int i = 0; i < width_ * height_; i++)
    {
        outputRGB[3 * i] = R[i];
        outputRGB[3 * i + 1] = G[i];
        outputRGB[3 * i + 2] = B[i];
    }
}
