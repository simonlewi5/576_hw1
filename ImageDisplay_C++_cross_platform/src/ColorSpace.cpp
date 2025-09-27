#include "ColorSpace.h"
#include <algorithm>
using namespace std;
void rgbToYUV(const vector<uint8_t> &rgb, vector<float> &Y, vector<float> &U, vector<float> &V)
{
    size_t n = rgb.size() / 3;
    Y.resize(n);
    U.resize(n);
    V.resize(n);
    for (size_t i = 0; i < n; i++)
    {
        float R = rgb[3 * i];
        float G = rgb[3 * i + 1];
        float B = rgb[3 * i + 2];
        Y[i] = 0.299f * R + 0.587f * G + 0.114f * B;
        U[i] = -0.147f * R - 0.289f * G + 0.436f * B;
        V[i] = 0.615f * R - 0.515f * G - 0.100f * B;
    }
}
void yuvToRGB(const vector<float> &Y, const vector<float> &U, const vector<float> &V, vector<uint8_t> &rgbOut)
{
    size_t n = Y.size();
    rgbOut.resize(n * 3);
    for (size_t i = 0; i < n; i++)
    {
        float y = Y[i];
        float u = U[i];
        float v = V[i];
        float R = y + 1.1398f * v;
        float G = y - 0.3946f * u - 0.5806f * v;
        float B = y + 2.0321f * u;
        R = R < 0 ? 0 : (R > 255 ? 255 : R);
        G = G < 0 ? 0 : (G > 255 ? 255 : G);
        B = B < 0 ? 0 : (B > 255 ? 255 : B);
        rgbOut[3 * i] = (uint8_t)(R + 0.5f);
        rgbOut[3 * i + 1] = (uint8_t)(G + 0.5f);
        rgbOut[3 * i + 2] = (uint8_t)(B + 0.5f);
    }
}
