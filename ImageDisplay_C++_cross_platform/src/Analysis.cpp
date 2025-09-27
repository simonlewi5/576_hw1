#include <cstdio>
#include <string>
#include <vector>
#include <array>
#include <algorithm>
#include <wx/image.h>
#include "ImageIO.h"
#include "ColorSpace.h"
#include "Quantizer.h"

using namespace std;

static bool savePng(const string &path, const vector<uint8_t> &rgb, int w, int h)
{
    wxInitAllImageHandlers();
    wxImage img(w, h, const_cast<unsigned char *>(rgb.data()), true);
    return img.SaveFile(path, wxBITMAP_TYPE_PNG);
}

static long long absErrRGB(const vector<uint8_t> &a, const vector<uint8_t> &b)
{
    long long s = 0;
    size_t n = a.size();
    for (size_t i = 0; i < n; i++)
    {
        s += llabs(int(a[i]) - int(b[i]));
    }
    return s;
}

int main(int argc, char **argv)
{
    if (argc < 4)
    {
        fprintf(stderr, "usage: Analysis image.rgb W H\n");
        return 1;
    }

    string path = argv[1];
    int W = stoi(argv[2]), H = stoi(argv[3]);

    vector<uint8_t> orig;
    loadPlanarRGB(path, W, H, orig);

    FILE *f = fopen("analysis.csv", "w");
    fprintf(f, "N,<C,M,Q1,Q2,Q3>,Output,Error\n");

    for (int N : {4, 6, 8})
    {
        vector<array<int, 3>> parts;
        for (int q1 = 1; q1 <= 8; q1++)
            for (int q2 = 1; q2 <= 8; q2++)
                for (int q3 = 1; q3 <= 8; q3++)
                    if (q1 + q2 + q3 == N)
                        parts.push_back({q1, q2, q3});

        for (int C = 1; C <= 2; C++)
        {
            for (int M = 1; M <= 2; M++)
            {
                for (auto q : parts)
                {
                    vector<uint8_t> proc(orig.size());

                    if (C == 1)
                    {
                        vector<uint8_t> R(W * H), G(W * H), B(W * H), Rq, Gq, Bq;
                        for (size_t i = 0; i < R.size(); i++)
                        {
                            R[i] = orig[3 * i];
                            G[i] = orig[3 * i + 1];
                            B[i] = orig[3 * i + 2];
                        }
                        if (M == 1)
                        {
                            vector<int> c;
                            quantizeUniformRGB(R, q[0], Rq, c);
                            quantizeUniformRGB(G, q[1], Gq, c);
                            quantizeUniformRGB(B, q[2], Bq, c);
                        }
                        else
                        {
                            vector<float> Rf(R.begin(), R.end()), Gf(G.begin(), G.end()), Bf(B.begin(), B.end());
                            vector<float> Rqo, Gqo, Bqo, ctr;
                            quantizeSmart(Rf, q[0], Rqo, ctr);
                            quantizeSmart(Gf, q[1], Gqo, ctr);
                            quantizeSmart(Bf, q[2], Bqo, ctr);
                            Rq.resize(R.size());
                            Gq.resize(G.size());
                            Bq.resize(B.size());
                            for (size_t i = 0; i < R.size(); i++)
                            {
                                Rq[i] = uint8_t(min(255.f, max(0.f, Rqo[i] + 0.5f)));
                                Gq[i] = uint8_t(min(255.f, max(0.f, Gqo[i] + 0.5f)));
                                Bq[i] = uint8_t(min(255.f, max(0.f, Bqo[i] + 0.5f)));
                            }
                        }
                        for (size_t i = 0; i < R.size(); i++)
                        {
                            proc[3 * i] = Rq[i];
                            proc[3 * i + 1] = Gq[i];
                            proc[3 * i + 2] = Bq[i];
                        }
                    }
                    else
                    {
                        vector<float> Y, U, V, qY, qU, qV, ctr;
                        rgbToYUV(orig, Y, U, V);
                        if (M == 1)
                        {
                            float minY = *min_element(Y.begin(), Y.end()), maxY = *max_element(Y.begin(), Y.end());
                            float minU = *min_element(U.begin(), U.end()), maxU = *max_element(U.begin(), U.end());
                            float minV = *min_element(V.begin(), V.end()), maxV = *max_element(V.begin(), V.end());
                            quantizeUniform(Y, q[0], minY, maxY, qY, ctr);
                            quantizeUniform(U, q[1], minU, maxU, qU, ctr);
                            quantizeUniform(V, q[2], minV, maxV, qV, ctr);
                        }
                        else
                        {
                            quantizeSmart(Y, q[0], qY, ctr);
                            quantizeSmart(U, q[1], qU, ctr);
                            quantizeSmart(V, q[2], qV, ctr);
                        }
                        yuvToRGB(qY, qU, qV, proc);
                    }

                    long long ae = absErrRGB(orig, proc);
                    string out = "";
                    if (N == 4)
                    {
                        char name[128];
                        snprintf(name, sizeof(name), "out_N%d_C%d_M%d_%d-%d-%d.png", N, C, M, q[0], q[1], q[2]);
                        savePng(name, proc, W, H);
                        out = name;
                    }
                    fprintf(f, "%d,\"<%d,%d,%d,%d,%d>\",%s,%lld\n",
                            N, C, M, q[0], q[1], q[2], out.c_str(), ae);
                }
            }
        }
    }
    fclose(f);
    return 0;
}
