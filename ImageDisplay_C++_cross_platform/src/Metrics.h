#pragma once
#include <vector>
#include <cstdint>
double mseRGB(const std::vector<uint8_t> &a, const std::vector<uint8_t> &b);
double psnrFromMse(double mse);
double mseChannelRGB(const std::vector<uint8_t> &a, const std::vector<uint8_t> &b, int channel);
double mseFloat(const std::vector<float> &a, const std::vector<float> &b);
long long absErrRGB(const std::vector<uint8_t> &a, const std::vector<uint8_t> &b);
