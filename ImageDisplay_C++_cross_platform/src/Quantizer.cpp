#include "Quantizer.h"
#include <algorithm>
#include <cmath>
using namespace std;
void quantizeUniform(const vector<float> &in, int bits, float minVal, float maxVal, vector<float> &out, vector<float> &centers)
{
    if (bits == 8)
    {
        out = in;
        centers.clear();
        return;
    }
    int L = 1 << bits;
    out.resize(in.size());
    centers.resize(L);
    float range = maxVal - minVal;
    if (range == 0)
    {
        for (size_t i = 0; i < in.size(); i++)
            out[i] = in[i];
        centers.assign(L, minVal);
        return;
    }
    float step = range / L;
    for (int k = 0; k < L; k++)
        centers[k] = minVal + (k + 0.5f) * step;
    for (size_t i = 0; i < in.size(); i++)
    {
        float v = in[i];
        float idxf = (v - minVal) / step;
        int idx = (int)floor(idxf);
        if (idx < 0)
            idx = 0;
        if (idx >= L)
            idx = L - 1;
        out[i] = centers[idx];
    }
}
void quantizeUniformRGB(const vector<uint8_t> &in, int bits, vector<uint8_t> &out, vector<int> &centers)
{
    if (bits == 8)
    {
        out = in;
        centers.clear();
        return;
    }
    int L = 1 << bits;
    out.resize(in.size());
    centers.resize(L);
    float step = 256.0f / L;
    for (int k = 0; k < L; k++)
    {
        int c = (int)floor(step * k + step / 2 + 0.5f);
        if (c < 0)
            c = 0;
        if (c > 255)
            c = 255;
        centers[k] = c;
    }
    for (size_t i = 0; i < in.size(); i++)
    {
        int idx = (int)(in[i] / step);
        if (idx >= L)
            idx = L - 1;
        out[i] = (uint8_t)centers[idx];
    }
}
void quantizeSmart(const vector<float> &in, int bits, vector<float> &out, vector<float> &centers, int maxIter)
{
    if (bits == 8)
    {
        out = in;
        centers.clear();
        return;
    }
    int L = 1 << bits;
    out.resize(in.size());
    size_t n = in.size();
    float minV = *min_element(in.begin(), in.end());
    float maxV = *max_element(in.begin(), in.end());
    if (minV == maxV)
    {
        centers.assign(L, minV);
        for (size_t i = 0; i < n; i++)
            out[i] = minV;
        return;
    }
    int bins = 256;
    vector<int> hist(bins, 0);
    float range = maxV - minV;
    for (float v : in)
    {
        int b = (int)floor((v - minV) / range * (bins - 1));
        if (b < 0)
            b = 0;
        if (b >= bins)
            b = bins - 1;
        hist[b]++;
    }
    vector<float> levelValues;
    levelValues.reserve(bins);
    for (int b = 0; b < bins; b++)
    {
        if (hist[b] > 0)
            levelValues.push_back(minV + (range * (b + 0.5f)) / bins);
    }
    if (levelValues.empty())
        levelValues.push_back((minV + maxV) / 2);
    centers.resize(L);
    long long total = n;
    double target = (double)total / L;
    double accum = 0;
    int k = 0;
    double running = 0;
    for (int b = 0; b < bins && k < L; b++)
    {
        running += hist[b];
        double threshold = (k + 0.5) * target;
        if (running >= threshold)
        {
            centers[k] = minV + (range * (b + 0.5f)) / bins;
            k++;
        }
    }
    while (k < L)
    {
        centers[k] = minV + (range * (k + 0.5f)) / L;
        k++;
    }
    for (int iter = 0; iter < maxIter; iter++)
    {
        bool changed = false;
        vector<float> newCenters(L, 0);
        vector<long long> counts(L, 0);
        for (int b = 0; b < bins; b++)
        {
            if (hist[b] == 0)
                continue;
            float v = minV + (range * (b + 0.5f)) / bins;
            int ci = 0;
            float bestDist = fabs(v - centers[0]);
            for (int j = 1; j < L; j++)
            {
                float d = fabs(v - centers[j]);
                if (d < bestDist)
                {
                    bestDist = d;
                    ci = j;
                }
            }
            newCenters[ci] += v * hist[b];
            counts[ci] += hist[b];
        }
        for (int j = 0; j < L; j++)
        {
            if (counts[j] > 0)
            {
                float nc = newCenters[j] / counts[j];
                if (fabs(nc - centers[j]) > 0.25f)
                    changed = true;
                centers[j] = nc;
            }
        }
        if (!changed)
            break;
        sort(centers.begin(), centers.end());
    }
    vector<float> boundaries(L - 1);
    for (int j = 0; j < L - 1; j++)
        boundaries[j] = (centers[j] + centers[j + 1]) * 0.5f;
    for (size_t i = 0; i < n; i++)
    {
        float v = in[i];
        int ci = 0;
        while (ci < L - 1 && v > boundaries[ci])
            ci++;
        out[i] = centers[ci];
    }
}
