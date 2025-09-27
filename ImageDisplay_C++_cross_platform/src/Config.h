#pragma once
#include <string>
struct QuantConfig
{
    std::string path;
    int colorMode;
    int quantMode;
    int q[3];
    int width;
    int height;
};
QuantConfig parseArgs(int argc, char **argv);
