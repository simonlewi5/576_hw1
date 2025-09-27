#include "ImageIO.h"
#include <fstream>
#include <stdexcept>
using namespace std;
void loadPlanarRGB(const string &path, int W, int H, vector<uint8_t> &interleaved)
{
    ifstream f(path, ios::binary);
    if (!f.is_open())
        throw runtime_error("Cannot open file");
    size_t plane = W * H;
    vector<uint8_t> R(plane), G(plane), B(plane);
    f.read((char *)R.data(), plane);
    f.read((char *)G.data(), plane);
    f.read((char *)B.data(), plane);
    if (!f)
        throw runtime_error("File read error");
    interleaved.resize(plane * 3);
    for (size_t i = 0; i < plane; i++)
    {
        interleaved[3 * i] = R[i];
        interleaved[3 * i + 1] = G[i];
        interleaved[3 * i + 2] = B[i];
    }
}
