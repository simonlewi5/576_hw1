#include <wx/wx.h>
#include <wx/dcbuffer.h>
#include <vector>
#include <string>
#include <iostream>
#include <thread>
#include <chrono>
#include "ImageIO.h"
#include "DCT.h"

using namespace std;

// Application class
class DCTApp : public wxApp
{
public:
    bool OnInit() override;
};

// Frame class for displaying images
class DCTFrame : public wxFrame
{
public:
    DCTFrame(const wxString &title, const vector<uint8_t> &original,
             DCTDecoder *decoder, int width, int height, int latencyMs);
    ~DCTFrame();

private:
    void OnPaint(wxPaintEvent &event);
    void OnTimer(wxTimerEvent &event);
    void OnClose(wxCloseEvent &event);

    void UpdateDecodedImage();

    wxImage origImg, decodedImg;
    wxScrolledWindow *scrollWin;
    int width_, height_;
    int latencyMs_;
    DCTDecoder *decoder_;
    vector<uint8_t> currentDecoded_;
    wxTimer *timer_;
    bool decodingComplete_;

    wxDECLARE_EVENT_TABLE();
};

wxBEGIN_EVENT_TABLE(DCTFrame, wxFrame)
    EVT_TIMER(wxID_ANY, DCTFrame::OnTimer)
        EVT_CLOSE(DCTFrame::OnClose)
            wxEND_EVENT_TABLE()

                bool DCTApp::OnInit()
{
    wxInitAllImageHandlers();

    try
    {
        // Parse command line arguments
        if (argc != 5)
        {
            wxMessageBox("Usage: DCTApp InputImage quantizationLevel DeliveryMode Latency\n"
                         "  InputImage: path to .rgb file\n"
                         "  quantizationLevel: 0-7 (0=no quantization)\n"
                         "  DeliveryMode: 1=baseline, 2=spectral selection, 3=successive bit\n"
                         "  Latency: milliseconds delay between decode steps",
                         "Invalid Arguments", wxOK | wxICON_ERROR);
            return false;
        }

        string inputPath = argv[1].ToStdString();
        int quantLevel = wxAtoi(argv[2]);
        int deliveryMode = wxAtoi(argv[3]);
        int latency = wxAtoi(argv[4]);

        // Validate arguments
        if (quantLevel < 0 || quantLevel > 7)
        {
            wxMessageBox("Quantization level must be between 0 and 7", "Error", wxOK | wxICON_ERROR);
            return false;
        }
        if (deliveryMode < 1 || deliveryMode > 3)
        {
            wxMessageBox("Delivery mode must be 1, 2, or 3", "Error", wxOK | wxICON_ERROR);
            return false;
        }
        if (latency < 0)
        {
            wxMessageBox("Latency must be non-negative", "Error", wxOK | wxICON_ERROR);
            return false;
        }

        // Fixed dimensions for this assignment (352x288)
        int width = 352;
        int height = 288;

        cout << "Loading image: " << inputPath << endl;
        cout << "Quantization level: " << quantLevel << " (2^" << quantLevel << ")" << endl;
        cout << "Delivery mode: " << deliveryMode << " (";
        if (deliveryMode == 1)
            cout << "Baseline";
        else if (deliveryMode == 2)
            cout << "Spectral Selection";
        else
            cout << "Successive Bit Approximation";
        cout << ")" << endl;
        cout << "Latency: " << latency << " ms" << endl;

        // Load original image
        vector<uint8_t> original;
        try
        {
            loadPlanarRGB(inputPath, width, height, original);
        }
        catch (const exception &e)
        {
            wxMessageBox(wxString::Format("Failed to load image: %s", e.what()),
                         "Error", wxOK | wxICON_ERROR);
            return false;
        }

        cout << "Image loaded successfully (" << original.size() << " bytes)" << endl;

        // Encode the image
        cout << "Encoding image..." << endl;
        DCTCoefficients R_dct, G_dct, B_dct;
        encodeImage(original, width, height, quantLevel, R_dct, G_dct, B_dct);
        cout << "Encoding complete. " << R_dct.blocks.size() << " blocks per channel." << endl;

        // Create decoder
        DeliveryMode mode = static_cast<DeliveryMode>(deliveryMode);
        DCTDecoder *decoder = new DCTDecoder(R_dct, G_dct, B_dct, width, height, quantLevel, mode);
        cout << "Decoder initialized. Total steps: " << decoder->getTotalSteps() << endl;

        // Create and show frame
        wxString title = wxString::Format("DCT Compression - Q:%d Mode:%d Latency:%dms",
                                          quantLevel, deliveryMode, latency);
        DCTFrame *frame = new DCTFrame(title, original, decoder, width, height, latency);
        frame->Show(true);

        return true;
    }
    catch (const exception &e)
    {
        wxMessageBox(wxString::Format("Error: %s", e.what()), "Error", wxOK | wxICON_ERROR);
        return false;
    }
}

DCTFrame::DCTFrame(const wxString &title, const vector<uint8_t> &original,
                   DCTDecoder *decoder, int width, int height, int latencyMs)
    : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(800, 400)),
      width_(width), height_(height), latencyMs_(latencyMs), decoder_(decoder),
      decodingComplete_(false)
{
    // Create original image
    origImg.Create(width_, height_);
    unsigned char *origData = origImg.GetData();
    for (int i = 0; i < width_ * height_; i++)
    {
        origData[3 * i] = original[3 * i];
        origData[3 * i + 1] = original[3 * i + 1];
        origData[3 * i + 2] = original[3 * i + 2];
    }

    // Create decoded image (initially black)
    decodedImg.Create(width_, height_);
    currentDecoded_.resize(width_ * height_ * 3, 0);

    // Create scrolled window for display
    scrollWin = new wxScrolledWindow(this, wxID_ANY);
    scrollWin->SetScrollbars(10, 10, width_ * 2 + 20, height_);
    scrollWin->SetVirtualSize(width_ * 2 + 20, height_);
    scrollWin->SetDoubleBuffered(true);
    scrollWin->SetBackgroundColour(*wxBLACK);

    // Bind paint event to the scrolled window
    scrollWin->Bind(wxEVT_PAINT, &DCTFrame::OnPaint, this);

    // Set client size to fit both images
    SetClientSize(width_ * 2 + 20, height_);

    // Set up timer for progressive decoding
    timer_ = new wxTimer(this, wxID_ANY);

    // Start decoding immediately
    if (latencyMs_ == 0)
    {
        // Decode all at once if no latency
        do
        {
            // Keep decoding until complete
        } while (decoder_->decodeStep(currentDecoded_));
        decodingComplete_ = true;
        UpdateDecodedImage();
        // Force initial paint
        scrollWin->Refresh();
        scrollWin->Update();
    }
    else
    {
        // Start timer for progressive decoding
        timer_->Start(latencyMs_);
    }

    Centre();
}

DCTFrame::~DCTFrame()
{
    if (timer_->IsRunning())
    {
        timer_->Stop();
    }
    delete timer_;
    delete decoder_;
}

void DCTFrame::OnClose(wxCloseEvent &event)
{
    if (timer_->IsRunning())
    {
        timer_->Stop();
    }
    event.Skip();
}

void DCTFrame::OnTimer(wxTimerEvent &event)
{
    if (decodingComplete_)
    {
        timer_->Stop();
        return;
    }

    // Perform one decode step
    bool hasMore = decoder_->decodeStep(currentDecoded_);
    UpdateDecodedImage();

    if (!hasMore)
    {
        decodingComplete_ = true;
        timer_->Stop();
        cout << "Decoding complete!" << endl;
    }

    // Force repaint
    scrollWin->Refresh();
}

void DCTFrame::UpdateDecodedImage()
{
    unsigned char *decodedData = decodedImg.GetData();
    for (int i = 0; i < width_ * height_; i++)
    {
        decodedData[3 * i] = currentDecoded_[3 * i];
        decodedData[3 * i + 1] = currentDecoded_[3 * i + 1];
        decodedData[3 * i + 2] = currentDecoded_[3 * i + 2];
    }
}

void DCTFrame::OnPaint(wxPaintEvent &event)
{
    wxBufferedPaintDC dc(scrollWin);
    scrollWin->DoPrepareDC(dc);

    dc.SetBackground(*wxBLACK_BRUSH);
    dc.Clear();

    // Draw original image on left
    wxBitmap origBmp(origImg);
    dc.DrawBitmap(origBmp, 0, 0, false);

    // Draw text label
    dc.SetTextForeground(*wxWHITE);
    dc.DrawText("Original", 5, 5);

    // Draw decoded image on right
    wxBitmap decodedBmp(decodedImg);
    dc.DrawBitmap(decodedBmp, width_ + 10, 0, false);

    // Draw text label
    dc.DrawText(decodingComplete_ ? "Decoded (Complete)" : "Decoded (In Progress)",
                width_ + 15, 5);
}

wxIMPLEMENT_APP(DCTApp);
