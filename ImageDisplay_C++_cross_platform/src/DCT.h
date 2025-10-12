#pragma once
#include <vector>
#include <cstdint>

// DCT/IDCT functions for 8x8 blocks
void forwardDCT(const double block[8][8], double dct[8][8]);
void inverseDCT(const double dct[8][8], double block[8][8]);

// Quantization/Dequantization
void quantizeBlock(const double dct[8][8], int quantized[8][8], int N);
void dequantizeBlock(const int quantized[8][8], double dct[8][8], int N);

// Helper functions to extract/insert 8x8 blocks from image channels
void extractBlock(const std::vector<uint8_t> &channel, int width, int height,
                  int blockX, int blockY, double block[8][8]);
void insertBlock(std::vector<uint8_t> &channel, int width, int height,
                 int blockX, int blockY, const double block[8][8]);

// Storage structure for DCT coefficients
struct DCTCoefficients
{
    std::vector<std::vector<int>> blocks; // Each block has 64 coefficients (flattened from 8x8)
    int numBlocksX;
    int numBlocksY;

    DCTCoefficients() : numBlocksX(0), numBlocksY(0) {}
    DCTCoefficients(int bx, int by) : numBlocksX(bx), numBlocksY(by)
    {
        blocks.resize(bx * by, std::vector<int>(64, 0));
    }
};

// Encoding: RGB -> DCT coefficients for each channel
void encodeImage(const std::vector<uint8_t> &rgb, int width, int height, int quantLevel,
                 DCTCoefficients &R_dct, DCTCoefficients &G_dct, DCTCoefficients &B_dct);

// Decoding modes
enum DeliveryMode
{
    BASELINE = 1,           // Sequential block-by-block
    SPECTRAL_SELECTION = 2, // DC first, then add AC coefficients progressively
    SUCCESSIVE_BIT = 3      // All coefficients with successive bit approximation
};

// Decoder class to handle progressive decoding with latency simulation
class DCTDecoder
{
public:
    DCTDecoder(const DCTCoefficients &R, const DCTCoefficients &G, const DCTCoefficients &B,
               int width, int height, int quantLevel, DeliveryMode mode);

    // Advance one step in the decoding process
    // Returns true if there are more steps, false if complete
    bool decodeStep(std::vector<uint8_t> &outputRGB);

    // Get total number of steps for this delivery mode
    int getTotalSteps() const;

    // Reset decoder to start
    void reset();

private:
    void decodeBaseline(std::vector<uint8_t> &outputRGB);
    void decodeSpectralSelection(std::vector<uint8_t> &outputRGB);
    void decodeSuccessiveBit(std::vector<uint8_t> &outputRGB);

    void decodeChannel(const DCTCoefficients &dct, std::vector<uint8_t> &channel,
                       int startCoef, int endCoef, int bitMask);

    DCTCoefficients R_dct_, G_dct_, B_dct_;
    int width_, height_;
    int quantLevel_;
    DeliveryMode mode_;
    int currentStep_;
    int totalSteps_;
};
