#include "Config.h"
#include <stdexcept>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <regex>
using namespace std;

static void inferSize(QuantConfig &c)
{
    smatch m;
    regex re(".*_(\\d+)x(\\d+)\\.rgb$", regex::icase);
    string s = c.path;
    if (regex_match(s, m, re))
    {
        c.width = stoi(m[1]);
        c.height = stoi(m[2]);
    }
    else
    {
        long long sz = (long long)filesystem::file_size(c.path);
        struct WH
        {
            int w, h;
        };
        WH cand[] = {{512, 512}, {352, 288}, {640, 480}, {256, 256}, {320, 240}, {384, 288}};
        bool ok = false;
        for (auto a : cand)
        {
            if (sz == 1LL * a.w * a.h * 3)
            {
                c.width = a.w;
                c.height = a.h;
                ok = true;
                break;
            }
        }
        if (!ok)
            throw runtime_error("Cannot infer WxH; name as *_WxH.rgb or size match required");
    }
    long long sz = (long long)filesystem::file_size(c.path);
    if (sz != 1LL * c.width * c.height * 3)
        throw runtime_error("Size mismatch: bytes != W*H*3");
}

QuantConfig parseArgs(int argc, char **argv)
{
    if (argc != 7)
        throw runtime_error("Usage: MyImageApplication <imagePath> <C|1|rgb|yuv> <M|1|uniform|smart> Q1 Q2 Q3");
    QuantConfig c;
    c.path = argv[1];
    if (!filesystem::exists(c.path))
        throw runtime_error("Image file not found");
    string cMode = argv[2], qMode = argv[3];
    auto toLower = [](string &s)
    { transform(s.begin(), s.end(), s.begin(), [](unsigned char ch)
                { return char(tolower(ch)); }); };
    toLower(cMode);
    toLower(qMode);
    try
    {
        c.colorMode = stoi(cMode);
    }
    catch (...)
    {
        if (cMode == "1" || cMode == "rgb")
            c.colorMode = 1;
        else if (cMode == "2" || cMode == "yuv")
            c.colorMode = 2;
        else
            throw runtime_error("Unrecognized color mode (use 1|rgb or 2|yuv)");
    }
    if (c.colorMode != 1 && c.colorMode != 2)
        throw runtime_error("Color mode must be 1|rgb or 2|yuv");
    try
    {
        c.quantMode = stoi(qMode);
    }
    catch (...)
    {
        if (qMode == "1" || qMode == "uniform" || qMode == "u")
            c.quantMode = 1;
        else if (qMode == "2" || qMode == "smart" || qMode == "s")
            c.quantMode = 2;
        else
            throw runtime_error("Unrecognized quant mode (use 1|uniform or 2|smart)");
    }
    if (c.quantMode != 1 && c.quantMode != 2)
        throw runtime_error("Quant mode must be 1|uniform or 2|smart");
    for (int i = 0; i < 3; i++)
    {
        c.q[i] = stoi(argv[4 + i]);
        if (c.q[i] <= 0 || c.q[i] > 8)
            throw runtime_error("Qi out of range");
    }
    inferSize(c);
    return c;
}
