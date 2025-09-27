#include <wx/wx.h>
#include <wx/dcbuffer.h>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <cstdint>
#include "Config.h"
#include "ImageIO.h"
#include "ColorSpace.h"
#include "Quantizer.h"
#include "Metrics.h"
using namespace std;

class MyApp : public wxApp
{
public:
  bool OnInit() override;
};

class MyFrame : public wxFrame
{
public:
  MyFrame(const wxString &title, const vector<uint8_t> &orig, const vector<uint8_t> &proc, int w, int h);

private:
  void OnPaint(wxPaintEvent &);
  wxImage origImg, procImg;
  wxScrolledWindow *sc;
  int w, h;
};

static void printYuvStats(const vector<uint8_t> &rgb)
{
  vector<float> Y, U, V;
  rgbToYUV(rgb, Y, U, V);
  auto mm = [&](const vector<float> &a)
  { auto p=minmax_element(a.begin(),a.end()); return pair<float,float>(*p.first,*p.second); };
  auto [ymin, ymax] = mm(Y);
  auto [umin, umax] = mm(U);
  auto [vmin, vmax] = mm(V);
  cout << "Y:[" << ymin << "," << ymax << "] U:[" << umin << "," << umax << "] V:[" << vmin << "," << vmax << "]\n";
}

static void checkRoundtrip(const vector<uint8_t> &rgb)
{
  vector<float> Y, U, V;
  rgbToYUV(rgb, Y, U, V);
  vector<uint8_t> rt;
  yuvToRGB(Y, U, V, rt);
  double m = mseRGB(rgb, rt);
  cout << "Roundtrip MSE=" << m << " PSNR=" << psnrFromMse(m) << "\n";
}

bool MyApp::OnInit()
{
  wxInitAllImageHandlers();
  try
  {
    vector<string> argStr(wxApp::argc);
    vector<char *> args(wxApp::argc);
    for (int i = 0; i < wxApp::argc; ++i)
    {
      argStr[i] = wxApp::argv[i].ToStdString();
      args[i] = argStr[i].data();
    }
    QuantConfig cfg = parseArgs(wxApp::argc, args.data());

    vector<uint8_t> original;
    loadPlanarRGB(cfg.path, cfg.width, cfg.height, original);
    printYuvStats(original);
    checkRoundtrip(original);

    vector<uint8_t> processed(original.size());
    vector<float> Y, U, V, qY, qU, qV, centersY, centersU, centersV;
    vector<int> centersRi, centersGi, centersBi;
    vector<uint8_t> Rq, Gq, Bq;

    if (cfg.colorMode == 1)
    {
      vector<uint8_t> R(cfg.width * cfg.height), G(cfg.width * cfg.height), B(cfg.width * cfg.height);
      for (int i = 0; i < cfg.width * cfg.height; i++)
      {
        R[i] = original[3 * i];
        G[i] = original[3 * i + 1];
        B[i] = original[3 * i + 2];
      }
      if (cfg.quantMode == 1)
      {
        quantizeUniformRGB(R, cfg.q[0], Rq, centersRi);
        quantizeUniformRGB(G, cfg.q[1], Gq, centersGi);
        quantizeUniformRGB(B, cfg.q[2], Bq, centersBi);
      }
      else
      {
        vector<float> Rf(R.begin(), R.end()), Gf(G.begin(), G.end()), Bf(B.begin(), B.end()), Rqf, Gqf, Bqf, cR, cG, cB;
        quantizeSmart(Rf, cfg.q[0], Rqf, cR);
        quantizeSmart(Gf, cfg.q[1], Gqf, cG);
        quantizeSmart(Bf, cfg.q[2], Bqf, cB);
        Rq.resize(Rqf.size());
        Gq.resize(Gqf.size());
        Bq.resize(Bqf.size());
        for (size_t i = 0; i < Rqf.size(); i++)
        {
          Rq[i] = uint8_t(min(255.f, max(0.f, Rqf[i] + 0.5f)));
          Gq[i] = uint8_t(min(255.f, max(0.f, Gqf[i] + 0.5f)));
          Bq[i] = uint8_t(min(255.f, max(0.f, Bqf[i] + 0.5f)));
        }
        if ((1 << cfg.q[0]) <= 16)
        {
          cout << "R centers:";
          for (float v : cR)
            cout << " " << v;
          cout << "\n";
        }
        if ((1 << cfg.q[1]) <= 16)
        {
          cout << "G centers:";
          for (float v : cG)
            cout << " " << v;
          cout << "\n";
        }
        if ((1 << cfg.q[2]) <= 16)
        {
          cout << "B centers:";
          for (float v : cB)
            cout << " " << v;
          cout << "\n";
        }
      }
      for (int i = 0; i < cfg.width * cfg.height; i++)
      {
        processed[3 * i] = Rq[i];
        processed[3 * i + 1] = Gq[i];
        processed[3 * i + 2] = Bq[i];
      }
      double mR = mseChannelRGB(original, processed, 0), mG = mseChannelRGB(original, processed, 1), mB = mseChannelRGB(original, processed, 2);
      cout << "Channel MSE R=" << mR << " G=" << mG << " B=" << mB << "\n";
    }
    else
    {
      rgbToYUV(original, Y, U, V);
      if (cfg.quantMode == 1)
      {
        float minY = *min_element(Y.begin(), Y.end()), maxY = *max_element(Y.begin(), Y.end());
        float minU = *min_element(U.begin(), U.end()), maxU = *max_element(U.begin(), U.end());
        float minV = *min_element(V.begin(), V.end()), maxV = *max_element(V.begin(), V.end());
        quantizeUniform(Y, cfg.q[0], minY, maxY, qY, centersY);
        quantizeUniform(U, cfg.q[1], minU, maxU, qU, centersU);
        quantizeUniform(V, cfg.q[2], minV, maxV, qV, centersV);
      }
      else
      {
        quantizeSmart(Y, cfg.q[0], qY, centersY);
        quantizeSmart(U, cfg.q[1], qU, centersU);
        quantizeSmart(V, cfg.q[2], qV, centersV);
        if ((1 << cfg.q[0]) <= 16)
        {
          cout << "Y centers:";
          for (float v : centersY)
            cout << " " << v;
          cout << "\n";
        }
        if ((1 << cfg.q[1]) <= 16)
        {
          cout << "U centers:";
          for (float v : centersU)
            cout << " " << v;
          cout << "\n";
        }
        if ((1 << cfg.q[2]) <= 16)
        {
          cout << "V centers:";
          for (float v : centersV)
            cout << " " << v;
          cout << "\n";
        }
      }
      yuvToRGB(qY, qU, qV, processed);
      double mY = mseFloat(Y, qY), mU = mseFloat(U, qU), mV = mseFloat(V, qV);
      cout << "Component MSE Y=" << mY << " U=" << mU << " V=" << mV << "\n";
    }

    double m = mseRGB(original, processed), p = psnrFromMse(m);
    cout << "MSE=" << m << " PSNR=" << p << "\n";

    MyFrame *f = new MyFrame("Image Display", original, processed, cfg.width, cfg.height);
    f->Show(true);
  }
  catch (const exception &e)
  {
    cerr << e.what() << "\n";
    return false;
  }
  return true;
}

MyFrame::MyFrame(const wxString &title, const vector<uint8_t> &orig, const vector<uint8_t> &proc, int w_, int h_)
    : wxFrame(NULL, wxID_ANY, title), w(w_), h(h_)
{
  unsigned char *o = (unsigned char *)malloc(w * h * 3);
  unsigned char *p = (unsigned char *)malloc(w * h * 3);
  for (int i = 0; i < w * h * 3; i++)
  {
    o[i] = orig[i];
    p[i] = proc[i];
  }
  origImg.SetData(o, w, h, false);
  procImg.SetData(p, w, h, false);
  sc = new wxScrolledWindow(this, wxID_ANY);
  sc->SetScrollbars(10, 10, w * 2, h);
  sc->SetVirtualSize(w * 2, h);
  sc->SetDoubleBuffered(true);
  sc->Bind(wxEVT_PAINT, &MyFrame::OnPaint, this);
  SetClientSize(w * 2, h);
}

void MyFrame::OnPaint(wxPaintEvent &)
{
  wxBufferedPaintDC dc(sc);
  sc->DoPrepareDC(dc);
  wxBitmap b1(origImg), b2(procImg);
  dc.DrawBitmap(b1, 0, 0, false);
  dc.DrawBitmap(b2, w, 0, false);
  dc.SetTextForeground(*wxWHITE);
  dc.DrawText("Original", 5, 5);
  dc.DrawText("Processed", w + 5, 5);
}

wxIMPLEMENT_APP(MyApp);
