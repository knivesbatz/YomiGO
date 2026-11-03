// YomiGoOCR: reads the text in an image with the OCR built into Windows 10 and 11.
// usage: YomiGoOCR in.bmp out.txt [language]
// exit 0 done, 1 bad arguments, 2 that language's OCR isn't installed, 3 couldn't read the image, 4 image too large
#include <windows.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Globalization.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Media.Ocr.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Streams.h>
#include <cstdio>
#include <string>

using namespace winrt;
using namespace Windows::Globalization;
using namespace Windows::Graphics::Imaging;
using namespace Windows::Media::Ocr;
using namespace Windows::Storage;

// Windows puts spaces between Japanese words; those aren't in the original text
static bool cjk(wchar_t c) { return (c >= 0x3000 && c <= 0x9FFF) || (c >= 0xFF00 && c <= 0xFFEF); }

int wmain(int argc, wchar_t **argv) {
    if (argc < 3) return 1;
    init_apartment();
    try {
        Language lang{argc > 3 ? argv[3] : L"ja"};
        if (!OcrEngine::IsLanguageSupported(lang)) return 2;
        OcrEngine engine = OcrEngine::TryCreateFromLanguage(lang);
        if (!engine) return 2;
        wchar_t full[MAX_PATH];
        if (!GetFullPathNameW(argv[1], MAX_PATH, full, nullptr)) return 3;
        StorageFile file = StorageFile::GetFileFromPathAsync(full).get();
        auto stream = file.OpenAsync(FileAccessMode::Read).get();
        BitmapDecoder decoder = BitmapDecoder::CreateAsync(stream).get();
        if (decoder.PixelWidth() > OcrEngine::MaxImageDimension() || decoder.PixelHeight() > OcrEngine::MaxImageDimension()) return 4;
        SoftwareBitmap bmp = decoder.GetSoftwareBitmapAsync(BitmapPixelFormat::Bgra8, BitmapAlphaMode::Premultiplied).get();
        OcrResult result = engine.RecognizeAsync(bmp).get();
        std::wstring out;
        for (auto const &line : result.Lines()) {
            std::wstring l;
            for (auto const &word : line.Words()) {
                std::wstring t{word.Text()};
                if (t.empty()) continue;
                if (!l.empty() && !(cjk(l.back()) && cjk(t.front()))) l += L' ';
                l += t;
            }
            out += l;
            out += L'\n';
        }
        int n = WideCharToMultiByte(CP_UTF8, 0, out.c_str(), (int)out.size(), nullptr, 0, nullptr, nullptr);
        std::string utf8(n, '\0');
        WideCharToMultiByte(CP_UTF8, 0, out.c_str(), (int)out.size(), utf8.data(), n, nullptr, nullptr);
        FILE *f = _wfopen(argv[2], L"wb");
        if (!f) return 3;
        fwrite(utf8.data(), 1, utf8.size(), f);
        fclose(f);
        return 0;
    } catch (...) {
        return 3;
    }
}
