#pragma once
#include <vector>
#include <string>
#include <cstdint>
void loadPlanarRGB(const std::string &path, int W, int H, std::vector<uint8_t> &interleaved);