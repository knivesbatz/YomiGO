// Shared by the 64-bit helpers: downloading a pinned model, and checking the processor first.
#pragma once
#include <windows.h>
#include <winhttp.h>
#include <bcrypt.h>
#include <intrin.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

static const wchar_t *pct_path;
static void progress(int pct) {
    FILE *f = _wfopen(pct_path, L"wb");
    if (f) { fprintf(f, "%d", pct); fclose(f); }
}

// One request for whatever's missing from the .part file. Returns the bytes it now holds.
static unsigned long long fetch_rest(const std::wstring &part, const wchar_t *url_path, unsigned long long have, unsigned long long want_size) {
    std::vector<unsigned char> buf(1 << 16);
    HINTERNET s = 0, c = 0, r = 0; FILE *f = 0; int shown = -1;
    DWORD status = 0, sz = sizeof status, tls = 0x800 | 0x2000; // TLS 1.2 and 1.3
    wchar_t range[64];
    swprintf(range, 64, L"Range: bytes=%llu-\r\n", have);
    if (!(s = WinHttpOpen(L"YomiGo", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, 0, 0, 0))) goto done;
    WinHttpSetOption(s, WINHTTP_OPTION_SECURE_PROTOCOLS, &tls, sizeof tls);
    // WinHTTP follows the redirect to Hugging Face's file servers by itself
    if (!(c = WinHttpConnect(s, L"huggingface.co", INTERNET_DEFAULT_HTTPS_PORT, 0))
        || !(r = WinHttpOpenRequest(c, L"GET", url_path, 0, 0, 0, WINHTTP_FLAG_SECURE))
        || !WinHttpSendRequest(r, have ? range : 0, have ? (DWORD)-1 : 0, 0, 0, 0, 0) || !WinHttpReceiveResponse(r, 0)
        || !WinHttpQueryHeaders(r, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, 0, &status, &sz, 0)) goto done;
    if (status == 200) have = 0; // the server ignored the range, so start over
    else if (status != 206) goto done;
    if (!(f = _wfopen(part.c_str(), have ? L"ab" : L"wb"))) goto done;
    for (;;) {
        DWORD avail = 0, n = 0;
        if (!WinHttpQueryDataAvailable(r, &avail) || !avail) break;
        if (!WinHttpReadData(r, buf.data(), avail < buf.size() ? avail : (DWORD)buf.size(), &n) || !n) break;
        if (have + n > want_size || fwrite(buf.data(), 1, n, f) != n) break;
        have += n;
        if ((int)(have * 100 / want_size) != shown) progress(shown = (int)(have * 100 / want_size));
    }
done:
    if (f) fclose(f);
    if (r) WinHttpCloseHandle(r);
    if (c) WinHttpCloseHandle(c);
    if (s) WinHttpCloseHandle(s);
    return have;
}

static bool sha256_file(const std::wstring &path, char *hex) {
    BCRYPT_ALG_HANDLE alg = 0; BCRYPT_HASH_HANDLE hash = 0; unsigned char digest[32]; bool ok = false;
    std::vector<unsigned char> buf(1 << 20); size_t n;
    FILE *f = _wfopen(path.c_str(), L"rb");
    if (f && !BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, 0, 0) && !BCryptCreateHash(alg, &hash, 0, 0, 0, 0, 0)) {
        ok = true;
        while (ok && (n = fread(buf.data(), 1, buf.size(), f)) > 0) ok = !BCryptHashData(hash, buf.data(), (ULONG)n, 0);
        ok = ok && !BCryptFinishHash(hash, digest, 32, 0);
        for (int i = 0; ok && i < 32; i++) sprintf(hex + 2 * i, "%02x", digest[i]);
    }
    if (hash) BCryptDestroyHash(hash);
    if (alg) BCryptCloseAlgorithmProvider(alg, 0);
    if (f) fclose(f);
    return ok;
}

// Downloads https://huggingface.co<url_path> to dest, keeping it only if its size and SHA-256 match. 0 done, 5 failed.
// A dropped connection resumes where it stopped, a few times, and a half-done file waits for the next try.
static int get_model(const wchar_t *dest, const wchar_t *url_path, unsigned long long want_size, const char *want_sha256) {
    std::wstring part = std::wstring(dest) + L".part";
    WIN32_FILE_ATTRIBUTE_DATA fa; unsigned long long have = 0; char hex[65] = "";
    if (GetFileAttributesExW(part.c_str(), GetFileExInfoStandard, &fa)) have = ((unsigned long long)fa.nFileSizeHigh << 32) | fa.nFileSizeLow;
    if (have > want_size) have = 0;
    for (int attempt = 0; attempt < 5 && have < want_size; attempt++) {
        if (attempt) Sleep(2000);
        have = fetch_rest(part, url_path, have, want_size);
    }
    if (have < want_size) return 5;
    bool ok = sha256_file(part, hex) && !strcmp(hex, want_sha256) && MoveFileExW(part.c_str(), dest, MOVEFILE_REPLACE_EXISTING);
    if (!ok) DeleteFileW(part.c_str()); // complete but wrong: never keep it
    return ok ? 0 : 5;
}

// The engines are built for AVX2, which an older processor would crash on
static bool has_avx2() {
    int r[4];
    __cpuid(r, 0);
    if (r[0] < 7) return false;
    __cpuid(r, 1);
    if (!(r[2] & (1 << 27)) || !(r[2] & (1 << 12)) || !(r[2] & (1 << 29)) || (_xgetbv(0) & 6) != 6) return false; // OS saves AVX state, FMA, F16C
    __cpuidex(r, 7, 0);
    return (r[1] & (1 << 5)) != 0;
}

static std::string utf8(const wchar_t *w) {
    int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, 0, 0, 0, 0);
    std::string s(n > 0 ? n - 1 : 0, '\0');
    if (n > 1) WideCharToMultiByte(CP_UTF8, 0, w, -1, s.data(), n, 0, 0);
    return s;
}
