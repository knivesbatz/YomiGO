// YomiGoListen: writes down what's said in an audio or video file, with whisper.cpp.
// usage: YomiGoListen model.bin out.txt progress.txt [file [language]]   with no file, asks for one
//        YomiGoListen --get model.bin progress.txt                         downloads the speech model
// exit 0 done, 1 bad arguments, 2 the model didn't load, 3 couldn't read the sound, 4 the processor lacks AVX2,
//      5 download failed, 6 no speech found, 7 nothing picked, 8 done but a subtitle file was already there
#include <windows.h>
#include <commdlg.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <thread>
#include "whisper.h"
#include "../helpers/hf_get.h"

// One revision of the model, checked against the SHA-256 Hugging Face lists for it
static const wchar_t MODEL_URL_PATH[] = L"/ggerganov/whisper.cpp/resolve/5359861c739e955e79d9a303bcbc70fb988958b1/ggml-large-v3-turbo-q5_0.bin";
static const char MODEL_SHA256[] = "394221709cd5ad1f40c46e6031ca61bce88931e6e088c188294c6d5a55ffa7e2";
static const unsigned long long MODEL_SIZE = 574041195;

// Windows decodes the file (MP3, M4A, WAV, MP4, MKV and others); this mixes it to the 16 kHz mono that whisper wants
static bool decode(const wchar_t *path, std::vector<float> &pcm) {
    IMFSourceReader *rd = 0; IMFMediaType *want = 0, *got = 0; bool ok = false;
    UINT32 rate = 0, ch = 0;
    const DWORD AUDIO = (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM;
    if (FAILED(MFCreateSourceReaderFromURL(path, 0, &rd))) return false;
    rd->SetStreamSelection((DWORD)MF_SOURCE_READER_ALL_STREAMS, FALSE);
    rd->SetStreamSelection(AUDIO, TRUE);
    if (SUCCEEDED(MFCreateMediaType(&want)) && SUCCEEDED(want->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio))
        && SUCCEEDED(want->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_Float)) && SUCCEEDED(rd->SetCurrentMediaType(AUDIO, 0, want))
        && SUCCEEDED(rd->GetCurrentMediaType(AUDIO, &got)) && SUCCEEDED(got->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &rate))
        && SUCCEEDED(got->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &ch)) && rate && ch) {
        // Each output sample averages the input samples it covers, which also keeps out aliasing
        double step = rate / 16000.0, edge = step, sum = 0; long long n = 0; int cnt = 0; float last = 0;
        for (;;) {
            DWORD flags = 0; IMFSample *smp = 0;
            if (FAILED(rd->ReadSample(AUDIO, 0, 0, &flags, 0, &smp))) break;
            if (smp) {
                IMFMediaBuffer *b = 0; BYTE *p = 0; DWORD len = 0;
                if (SUCCEEDED(smp->ConvertToContiguousBuffer(&b)) && SUCCEEDED(b->Lock(&p, 0, &len))) {
                    const float *x = (const float *)p;
                    for (DWORD i = 0, frames = len / 4 / ch; i < frames; i++) {
                        float m = 0;
                        for (UINT32 k = 0; k < ch; k++) m += x[i * ch + k];
                        sum += m / ch; cnt++; n++;
                        while (n >= edge) { float v = cnt ? (float)(sum / cnt) : last; pcm.push_back(v); last = v; sum = 0; cnt = 0; edge += step; }
                    }
                    b->Unlock();
                }
                if (b) b->Release();
                smp->Release();
            }
            if (flags & MF_SOURCE_READERF_ENDOFSTREAM) { ok = true; break; }
            if (flags & MF_SOURCE_READERF_ERROR) break;
        }
    }
    if (got) got->Release();
    if (want) want->Release();
    rd->Release();
    return ok && !pcm.empty();
}

// The model is read through _wfopen, so a YomiGo folder with Japanese in its path still works
static size_t ld_read(void *f, void *out, size_t n) { return fread(out, 1, n, (FILE *)f); }
static bool ld_eof(void *f) { return feof((FILE *)f) != 0; }
static void ld_close(void *f) { fclose((FILE *)f); }
static void no_log(enum ggml_log_level, const char *, void *) {}
static void on_progress(whisper_context *, whisper_state *, int p, void *) { progress(p); }

static void srt_time(FILE *f, long long cs) { // whisper times are in hundredths of a second
    fprintf(f, "%02lld:%02lld:%02lld,%03lld", cs / 360000, cs / 6000 % 60, cs / 100 % 60, cs % 100 * 10);
}

int wmain(int argc, wchar_t **argv) {
    if (argc < 4) return 1;
    pct_path = argv[3];
    progress(0);
    if (!wcscmp(argv[1], L"--get")) return get_model(argv[2], MODEL_URL_PATH, MODEL_SIZE, MODEL_SHA256);
    if (!has_avx2()) return 4;
    wchar_t media[MAX_PATH * 2] = L"";
    if (argc > 4) wcsncpy(media, argv[4], MAX_PATH * 2 - 1);
    else {
        OPENFILENAMEW ofn = {sizeof ofn};
        ofn.lpstrFilter = L"Audio and video\0*.mp3;*.m4a;*.aac;*.wav;*.wma;*.flac;*.mp4;*.m4v;*.mkv;*.mov;*.wmv;*.avi\0All files\0*.*\0";
        ofn.lpstrFile = media; ofn.nMaxFile = MAX_PATH * 2;
        ofn.lpstrTitle = L"YomiGo: pick something to transcribe";
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
        if (!GetOpenFileNameW(&ofn)) return 7;
    }
    std::vector<float> pcm;
    CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(MFStartup(MF_VERSION, MFSTARTUP_LITE))) return 3;
    bool decoded = decode(media, pcm);
    MFShutdown();
    if (!decoded) return 3;

    whisper_log_set(no_log, 0);
    FILE *mf = _wfopen(argv[1], L"rb");
    if (!mf) return 2;
    whisper_model_loader loader = {mf, ld_read, ld_eof, ld_close};
    whisper_context_params cp = whisper_context_default_params();
    cp.use_gpu = false;
    whisper_context *ctx = whisper_init_with_params(&loader, cp);
    if (!ctx) return 2;
    whisper_full_params wp = whisper_full_default_params(WHISPER_SAMPLING_BEAM_SEARCH);
    std::string lang = argc > 5 ? utf8(argv[5]) : "ja";
    wp.language = lang.c_str();
    wp.n_threads = (int)(std::thread::hardware_concurrency() / 2); // leave room for everything else
    if (wp.n_threads < 1) wp.n_threads = 1;
    if (wp.n_threads > 8) wp.n_threads = 8;
    wp.beam_search.beam_size = 5;
    wp.print_progress = wp.print_realtime = wp.print_timestamps = false;
    wp.progress_callback = on_progress;
    if (whisper_full(ctx, wp, pcm.data(), (int)pcm.size())) { whisper_free(ctx); return 3; }

    // Subtitles go beside the file, as name.ja.srt, which players such as VLC and mpv pick up. An existing one is kept.
    std::wstring srt = media;
    size_t dot = srt.find_last_of(L'.'), slash = srt.find_last_of(L"\\/");
    if (dot != std::wstring::npos && (slash == std::wstring::npos || dot > slash)) srt.resize(dot);
    srt += L"." + std::wstring(lang.begin(), lang.end()) + L".srt";
    FILE *sf = GetFileAttributesW(srt.c_str()) == INVALID_FILE_ATTRIBUTES ? _wfopen(srt.c_str(), L"wb") : 0;
    std::vector<std::string> lines;
    for (int i = 0, n = whisper_full_n_segments(ctx); i < n; i++) {
        std::string t = whisper_full_get_segment_text(ctx, i);
        t.erase(0, t.find_first_not_of(' '));
        t.erase(t.find_last_not_of(' ') + 1);
        if (t.empty() || (!lines.empty() && t == lines.back())) continue; // whisper sometimes repeats itself over music
        lines.push_back(t);
        if (sf) {
            fprintf(sf, "%zu\r\n", lines.size());
            srt_time(sf, whisper_full_get_segment_t0(ctx, i)); fputs(" --> ", sf); srt_time(sf, whisper_full_get_segment_t1(ctx, i));
            fprintf(sf, "\r\n%s\r\n\r\n", t.c_str());
        }
    }
    whisper_free(ctx);
    if (sf) fclose(sf);
    if (lines.empty()) return 6;
    // Newest first, because the Text Reader puts each line it adds on top; this way the first line ends up first
    FILE *of = _wfopen(argv[2], L"wb");
    if (!of) return 3;
    for (size_t i = lines.size(); i-- > 0;) fprintf(of, "%s\n", lines[i].c_str());
    fclose(of);
    progress(100);
    return sf ? 0 : 8;
}
