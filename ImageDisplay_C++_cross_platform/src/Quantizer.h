#pragma once
#include <vector>
#include <cstdint>
#include <cmath>
void quantizeUniform(const std::vector<float> &in, int bits, float minVal, float maxVal, std::vector<float> &out, std::vector<float> &centers);
void quantizeUniformRGB(const std::vector<uint8_t> &in, int bits, std::vector<uint8_t> &out, std::vector<int> &centers);
void quantizeSmart(const std::vector<float> &in, int bits, std::vector<float> &out, std::vector<float> &centers, int maxIter = 30);
