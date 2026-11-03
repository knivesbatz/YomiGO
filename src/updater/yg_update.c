/* YomiGoUpdate: fetches a release from GitHub over HTTPS, checks its signature with Windows' own crypto,
   swaps the program with rollback, and restarts it. Windows 7 or later.
   usage: YomiGoUpdate target.exe current_version parent_pid [--quiet]
          YomiGoUpdate --verify file sig   (exit 0 when the signature is good; used by the tests)
          YomiGoUpdate --swap target.exe new.exe  (the swap step alone; used by the tests)
          --yes after the first form accepts the update and skips the restart, for the end-to-end test. Signatures are still checked. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>
#include <wincrypt.h>
#include <shellapi.h>
#include "version_gen.h"
#include "pubkey.h"

#pragma function(memset, memcpy)
void *memset(void *d, int c, size_t n) { unsigned char *p = (unsigned char *)d; while (n--) *p++ = (unsigned char)c; return d; }
void *memcpy(void *d, const void *s, size_t n) { unsigned char *p = (unsigned char *)d; const unsigned char *q = (const unsigned char *)s; while (n--) *p++ = *q++; return d; }

#define MAX_DL (64 * 1024 * 1024)
static int quiet, assume_yes;
static char dir[MAX_PATH];

static void say(const char *msg) { if (!quiet) MessageBoxA(0, msg, "YomiGo Update", MB_OK | MB_ICONINFORMATION); }
static WCHAR *widen(const char *s, WCHAR *w, int cap) { MultiByteToWideChar(CP_UTF8, 0, s, -1, w, cap); return w; }

/* HTTPS GET to memory. Certificates are checked by WinHTTP, and redirects never drop to plain HTTP. */
static char *http_get(const char *host, const char *path, DWORD *out_len) {
    static WCHAR wh[256], wp[1024];
    HINTERNET s = 0, c = 0, r = 0; char *buf = 0; DWORD total = 0, cap = 1 << 20, avail = 0, got = 0, status = 0, sz = sizeof status;
    DWORD tls = 0x00000800 | 0x00002000; /* TLS 1.2 and 1.3 only */
    s = WinHttpOpen(L"YomiGoUpdate", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, 0, 0, 0);
    if (!s) goto fail;
    WinHttpSetOption(s, WINHTTP_OPTION_SECURE_PROTOCOLS, &tls, sizeof tls);
    c = WinHttpConnect(s, widen(host, wh, 256), INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!c) goto fail;
    r = WinHttpOpenRequest(c, L"GET", widen(path, wp, 1024), 0, 0, 0, WINHTTP_FLAG_SECURE);
    if (!r) goto fail;
    if (!WinHttpSendRequest(r, L"Accept: */*\r\n", (DWORD)-1, 0, 0, 0, 0) || !WinHttpReceiveResponse(r, 0)) goto fail;
    WinHttpQueryHeaders(r, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, 0, &status, &sz, 0);
    if (status != 200) goto fail;
    buf = (char *)VirtualAlloc(0, MAX_DL + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!buf) goto fail;
    (void)cap;
    for (;;) {
        if (!WinHttpQueryDataAvailable(r, &avail)) { VirtualFree(buf, 0, MEM_RELEASE); buf = 0; goto fail; }
        if (!avail) break;
        if (total + avail > MAX_DL) { VirtualFree(buf, 0, MEM_RELEASE); buf = 0; goto fail; }
        if (!WinHttpReadData(r, buf + total, avail, &got)) { VirtualFree(buf, 0, MEM_RELEASE); buf = 0; goto fail; }
        total += got;
    }
    buf[total] = 0;
    *out_len = total;
fail:
    if (r) WinHttpCloseHandle(r);
    if (c) WinHttpCloseHandle(c);
    if (s) WinHttpCloseHandle(s);
    return buf;
}

static int verify(const BYTE *data, DWORD len, const BYTE *sig, DWORD siglen) {
    HCRYPTPROV p = 0; HCRYPTKEY k = 0; HCRYPTHASH h = 0; int ok = 0; static BYTE rev[1024]; DWORD i;
    if (siglen == 0 || siglen > sizeof rev) return 0;
    for (i = 0; i < siglen; i++) rev[i] = sig[siglen - 1 - i]; /* Windows wants it little-endian */
    if (CryptAcquireContextA(&p, 0, 0, PROV_RSA_AES, CRYPT_VERIFYCONTEXT) &&
        CryptImportKey(p, YG_PUBKEY, sizeof YG_PUBKEY, 0, 0, &k) &&
        CryptCreateHash(p, CALG_SHA_256, 0, 0, &h) &&
        CryptHashData(h, data, len, 0) &&
        CryptVerifySignatureA(h, rev, siglen, k, 0, 0)) ok = 1;
    if (h) CryptDestroyHash(h);
    if (k) CryptDestroyKey(k);
    if (p) CryptReleaseContext(p, 0);
    return ok;
}
static char *read_all(const char *path, DWORD *len) {
    HANDLE f = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0); DWORD sz, got = 0; char *b;
    if (f == INVALID_HANDLE_VALUE) return 0;
    sz = GetFileSize(f, 0);
    b = (char *)VirtualAlloc(0, sz + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (b && !ReadFile(f, b, sz, &got, 0)) got = 0;
    CloseHandle(f);
    *len = got;
    return b;
}
static int write_all(const char *path, const char *b, DWORD len) {
    HANDLE f = CreateFileA(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0); DWORD put = 0; int ok;
    if (f == INVALID_HANDLE_VALUE) return 0;
    ok = WriteFile(f, b, len, &put, 0) && put == len && FlushFileBuffers(f);
    CloseHandle(f);
    return ok;
}
/* Old copy stays as .old; any failure puts it straight back */
static int swap_in(const char *target, const char *fresh) {
    char old[MAX_PATH]; int i;
    lstrcpyA(old, target); lstrcatA(old, ".old");
    DeleteFileA(old);
    for (i = 0; i < 40; i++) { if (MoveFileExA(target, old, MOVEFILE_REPLACE_EXISTING) || GetLastError() == ERROR_FILE_NOT_FOUND) break; Sleep(250); }
    if (i == 40) return 0;
    if (!MoveFileExA(fresh, target, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) { MoveFileExA(old, target, MOVEFILE_REPLACE_EXISTING); return 0; }
    return 1;
}
static int newer(const char *a, const char *b) { /* a > b, "1.2.3" */
    int i;
    for (i = 0; i < 3; i++) {
        int x = 0, y = 0;
        while (*a >= '0' && *a <= '9') x = x * 10 + (*a++ - '0');
        while (*b >= '0' && *b <= '9') y = y * 10 + (*b++ - '0');
        if (x != y) return x > y;
        if (*a == '.') a++;
        if (*b == '.') b++;
    }
    return 0;
}
/* Finds "tag_name":"v1.2.3" in GitHub's reply; nothing else in it is trusted or used */
static int find_tag(const char *json, char *out, int cap) {
    const char *p = json; static const char key[] = "\"tag_name\""; int n = sizeof key - 1, k = 0;
    while (*p) {
        int i = 0; while (i < n && p[i] == key[i]) i++;
        if (i == n) {
            p += n;
            while (*p == ':' || *p == ' ' || *p == '"') p++;
            if (*p == 'v') p++;
            while (((*p >= '0' && *p <= '9') || *p == '.') && k < cap - 1) out[k++] = *p++;
            out[k] = 0;
            return k > 0;
        }
        p++;
    }
    return 0;
}
static char *next_arg(char **pp, char *out, int cap) {
    char *p = *pp; int k = 0; char end = ' ';
    while (*p == ' ') p++;
    if (!*p) return 0;
    if (*p == '"') { end = '"'; p++; }
    while (*p && *p != end && k < cap - 1) out[k++] = *p++;
    out[k] = 0;
    if (*p == end) p++;
    *pp = p;
    return out;
}
static int fetch_verified(const char *tag, const char *asset, const char *dest) {
    char path[512]; DWORD a = 0, b = 0; char *data, *sig; int ok;
    wsprintfA(path, "/" YG_REPO "/releases/download/v%s/%s", tag, asset);
    if (!(data = http_get("github.com", path, &a))) return 0;
    wsprintfA(path, "/" YG_REPO "/releases/download/v%s/%s.sig", tag, asset);
    if (!(sig = http_get("github.com", path, &b))) { VirtualFree(data, 0, MEM_RELEASE); return 0; }
    ok = verify((BYTE *)data, a, (BYTE *)sig, b) && write_all(dest, data, a);
    VirtualFree(data, 0, MEM_RELEASE); VirtualFree(sig, 0, MEM_RELEASE);
    return ok;
}
void yg_entry(void) {
    static char target[MAX_PATH], cur[32], pid[32], flag[32], tag[32], full[MAX_PATH], fresh[MAX_PATH], me[MAX_PATH], freshme[MAX_PATH], msg[256], path[128];
    char *cl = GetCommandLineA(); int i; DWORD len = 0; char *json; HANDLE parent;
    { static char prog[MAX_PATH]; next_arg(&cl, prog, MAX_PATH); }
    GetModuleFileNameA(0, me, MAX_PATH);
    lstrcpyA(dir, me);
    for (i = lstrlenA(dir) - 1; i > 0 && dir[i] != '\\'; i--) {}
    dir[i] = 0;
    if (!next_arg(&cl, target, MAX_PATH)) { quiet = 0; say("Start this from YomiGo: Settings > Check for updates."); ExitProcess(2); }
    if (!lstrcmpA(target, "--verify")) {
        static char f[MAX_PATH], s[MAX_PATH]; DWORD a = 0, b = 0; char *d, *g;
        next_arg(&cl, f, MAX_PATH); next_arg(&cl, s, MAX_PATH);
        d = read_all(f, &a); g = read_all(s, &b);
        ExitProcess(d && g && verify((BYTE *)d, a, (BYTE *)g, b) ? 0 : 1);
    }
    if (!lstrcmpA(target, "--swap")) {
        static char t[MAX_PATH], n[MAX_PATH];
        next_arg(&cl, t, MAX_PATH); next_arg(&cl, n, MAX_PATH);
        ExitProcess(swap_in(t, n) ? 0 : 1);
    }
    next_arg(&cl, cur, 32); next_arg(&cl, pid, 32);
    while (next_arg(&cl, flag, 32)) { if (!lstrcmpA(flag, "--quiet")) quiet = 1; else if (!lstrcmpA(flag, "--yes")) { assume_yes = 1; quiet = 1; } }
    /* Only our own programs can be updated, whatever the command line says */
    if (lstrcmpA(target, "YomiGo.exe") && lstrcmpA(target, "YomiGoHTPC.exe")) ExitProcess(2);

    wsprintfA(path, "/repos/" YG_REPO "/releases/latest");
    json = http_get("api.github.com", path, &len);
    if (!json || !find_tag(json, tag, 32)) { say("Couldn't reach GitHub. Check your connection and try later. Your copy of YomiGo is unchanged."); ExitProcess(3); }
    if (!newer(tag, cur)) { say("You already have the latest version."); ExitProcess(0); }
    wsprintfA(msg, "YomiGo %s is available (you have %s).\n\nInstall it now? YomiGo will close and restart. Your saves stay as they are.", tag, cur);
    if (!assume_yes && MessageBoxA(0, msg, "YomiGo Update", MB_YESNO | MB_ICONQUESTION) != IDYES) ExitProcess(0);

    wsprintfA(full, "%s\\%s", dir, target);
    wsprintfA(fresh, "%s\\%s.new", dir, target);
    wsprintfA(freshme, "%s\\YomiGoUpdate.exe.new", dir);
    if (!fetch_verified(tag, target, fresh)) {
        DeleteFileA(fresh);
        if (!assume_yes) quiet = 0;
        say("The download failed, or its signature did not check out. Nothing was changed.");
        ExitProcess(4);
    }
    /* Ask the program to close, so it can finish saving */
    { HWND w = 0; while ((w = FindWindowExA(0, w, "YomiGoWnd", 0)) != 0) { DWORD owner = 0; GetWindowThreadProcessId(w, &owner); { DWORD want = 0; const char *p = pid; while (*p >= '0' && *p <= '9') want = want * 10 + (DWORD)(*p++ - '0'); if (owner == want) PostMessageA(w, WM_CLOSE, 0, 0); } } }
    { DWORD want = 0; const char *p = pid; while (*p >= '0' && *p <= '9') want = want * 10 + (DWORD)(*p++ - '0');
      parent = OpenProcess(SYNCHRONIZE, FALSE, want);
      if (parent) { WaitForSingleObject(parent, 15000); CloseHandle(parent); } }
    if (!swap_in(full, fresh)) { DeleteFileA(fresh); quiet = 0; say("Couldn't replace the program, so the old version is still in place."); ExitProcess(5); }
    /* A running exe can be renamed, so the updater can refresh itself too. If this fails, the old updater still works. */
    if (fetch_verified(tag, "YomiGoUpdate.exe", freshme)) { if (!swap_in(me, freshme)) DeleteFileA(freshme); }
    /* The text reader's helper comes along too; YomiGo works without it */
    { static char ocr[MAX_PATH], ocrnew[MAX_PATH];
      wsprintfA(ocr, "%s\\YomiGoOCR.exe", dir); wsprintfA(ocrnew, "%s\\YomiGoOCR.exe.new", dir);
      if (fetch_verified(tag, "YomiGoOCR.exe", ocrnew)) { if (!swap_in(ocr, ocrnew)) DeleteFileA(ocrnew); } }
    if (!assume_yes) ShellExecuteA(0, 0, full, 0, dir, SW_SHOWNORMAL);
    ExitProcess(0);
}
