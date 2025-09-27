#include "Metrics.h"
#include <cmath>
#include <cstdint>
using namespace std;
double mseRGB(const vector<uint8_t> &a, const vector<uint8_t> &b)
{
    size_t n = a.size();
    double s = 0;
    for (size_t i = 0; i < n; i++)
    {
        double d = (double)a[i] - b[i];
        s += d * d;
    }
    return s / (n);
}
double psnrFromMse(double mse)
{
    if (mse == 0)
        return 1e9;
    double maxv = 255.0;
    return 10.0 * log10((maxv * maxv) / mse);
}
double mseChannelRGB(const vector<uint8_t> &a, const vector<uint8_t> &b, int channel)
{
    size_t pixels = a.size() / 3;
    double s = 0;
    for (size_t i = 0; i < pixels; i++)
    {
        double d = (double)a[3 * i + channel] - b[3 * i + channel];
        s += d * d;
    }
    return s / pixels;
}
double mseFloat(const vector<float> &a, const vector<float> &b)
{
    size_t n = a.size();
    double s = 0;
    for (size_t i = 0; i < n; i++)
    {
        double d = (double)a[i] - b[i];
        s += d * d;
    }
    return s / n;
}
long long absErrRGB(const vector<uint8_t> &a, const vector<uint8_t> &b)
{
    size_t n = a.size();
    long long s = 0;
    for (size_t i = 0; i < n; i++)
    {
        int d = int(a[i]) - int(b[i]);
        s += d < 0 ? -d : d;
    }
    return s;
}
