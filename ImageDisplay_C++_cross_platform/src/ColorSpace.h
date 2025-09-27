#pragma once
#include <vector>
#include <cstdint>
void rgbToYUV(const std::vector<uint8_t> &rgb, std::vector<float> &Y, std::vector<float> &U, std::vector<float> &V);
void yuvToRGB(const std::vector<float> &Y, const std::vector<float> &U, const std::vector<float> &V, std::vector<uint8_t> &rgbOut);
