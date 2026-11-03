/* Windows layer. Links only what Windows 95 has; anything newer is looked up at run time. */
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0400
#define WINVER 0x0400
#include <windows.h>
#include <mmsystem.h>
#include <shellapi.h>
#undef IN /* windows.h defines it as nothing */
#include "yg.h"
#include "ui.h"
#include "version_gen.h"

/* No C runtime: the compiler still emits calls to these two */
#pragma function(memset, memcpy)
void *memset(void *d, int c, size_t n) { unsigned char *p = (unsigned char *)d; while (n--) *p++ = (unsigned char)c; return d; }
void *memcpy(void *d, const void *s, size_t n) { unsigned char *p = (unsigned char *)d; const unsigned char *q = (const unsigned char *)s; while (n--) *p++ = *q++; return d; }

static HWND wnd;
static char dir[MAX_PATH], root[MAX_PATH];
static int headless, is_nt, quit_now, crt, sound_on = 1;
static char last_day_checked[16];

/* ---- files ---- */
static void path_of(char *out, const char *name) {
    int i, n;
    ys_cpy(out, root, MAX_PATH);
    n = ys_len(out);
    out[n++] = '\\'; out[n] = 0;
    /* the only names passed in are ours or sanitised learner names, but a path escape must never work */
    for (i = 0; name[i]; i++) if (name[i] == '\\' || name[i] == '/' || name[i] == ':') { out[n] = 0; ys_cat(out, "bad", MAX_PATH); return; }
    ys_cat(out, name, MAX_PATH);
}
static int read_abs(const char *p, char *buf, int cap) {
    HANDLE h = CreateFileA(p, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    DWORD got = 0;
    if (h == INVALID_HANDLE_VALUE) return -1;
    if (!ReadFile(h, buf, (DWORD)cap, &got, 0)) got = 0;
    CloseHandle(h);
    return (int)got;
}
int plat_read(const char *name, char *buf, int cap) { char p[MAX_PATH]; path_of(p, name); return read_abs(p, buf, cap); }
/* Write beside the file, then swap, keeping one old copy: a crash mid-save can't lose progress */
int plat_write(const char *name, const char *buf, int len) {
    char p[MAX_PATH], t[MAX_PATH], b[MAX_PATH]; HANDLE h; DWORD put = 0; int ok;
    path_of(p, name);
    ys_cpy(t, p, MAX_PATH); ys_cat(t, ".tmp", MAX_PATH);
    ys_cpy(b, p, MAX_PATH); ys_cat(b, ".bak", MAX_PATH);
    h = CreateFileA(t, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if (h == INVALID_HANDLE_VALUE) return 0;
    ok = WriteFile(h, buf, (DWORD)len, &put, 0) && (int)put == len;
    ok = FlushFileBuffers(h) && ok;
    CloseHandle(h);
    if (!ok) { DeleteFileA(t); return 0; }
    DeleteFileA(b);
    MoveFileA(p, b);
    if (!MoveFileA(t, p)) { MoveFileA(b, p); return 0; }
    return 1;
}
int plat_list_saves(char names[][32], int max) {
    char pat[MAX_PATH]; WIN32_FIND_DATAA f; HANDLE h; int n = 0;
    path_of(pat, "*.sav");
    h = FindFirstFileA(pat, &f);
    if (h == INVALID_HANDLE_VALUE) return 0;
    do {
        int l = ys_len(f.cFileName);
        if (n < max && l > 4 && l - 4 < 32) { ys_cpy(names[n], f.cFileName, l - 3); n++; }
    } while (FindNextFileA(h, &f));
    FindClose(h);
    return n;
}
void plat_copy_backup(const char *name) {
    char s[MAX_PATH], d[MAX_PATH], t[16]; SYSTEMTIME st;
    path_of(s, name); ys_cat(s, ".sav", MAX_PATH);
    ys_cpy(d, root, MAX_PATH); ys_cat(d, "\\backup", MAX_PATH);
    CreateDirectoryA(d, 0);
    ys_cat(d, "\\", MAX_PATH); ys_cat(d, name, MAX_PATH); ys_cat(d, "-", MAX_PATH);
    GetLocalTime(&st);
    ys_itoa(st.wYear * 10000 + st.wMonth * 100 + st.wDay, t); ys_cat(d, t, MAX_PATH); ys_cat(d, ".sav", MAX_PATH);
    CopyFileA(s, d, FALSE);
}
const char *plat_exe_dir(void) { return dir; }

static int day_offset;
void plat_test_next_day(void) { day_offset++; }
int plat_today(void) {
    SYSTEMTIME st; int y, m, d;
    GetLocalTime(&st);
    y = st.wYear; m = st.wMonth; d = st.wDay;
    /* days since a fixed epoch, from the civil calendar */
    if (m <= 2) { y--; m += 12; }
    return 365 * y + y / 4 - y / 100 + y / 400 + (153 * (m - 3) + 2) / 5 + d + day_offset;
}
int plat_ticks(void) { return (int)GetTickCount(); }
void plat_quit(void) { quit_now = 1; }

/* ---- UTF-8 and wide paths, with the wide calls looked up at run time ---- */
typedef int (WINAPI *MBWC)(UINT, DWORD, LPCSTR, int, LPWSTR, int);
static int to_wide(const char *u, WCHAR *w, int cap) {
    int n = 0, c;
    while ((c = utf8_next(&u)) != 0 && n < cap - 1) w[n++] = (WCHAR)c;
    w[n] = 0;
    return n;
}
static int from_wide(const WCHAR *w, char *u, int cap) {
    int n = 0;
    while (*w && n < cap - 4) n += utf8_put(*w++, u + n);
    u[n] = 0;
    return n;
}
/* Clipboard. 9x has no Unicode clipboard, so it carries plain ASCII there. */
static char test_clip[512];
int plat_clip_get(char *out, int cap) {
    HANDLE h; int n = 0;
    out[0] = 0;
    if (headless) { ys_cpy(out, test_clip, cap); return ys_len(out); }
    if (!OpenClipboard(wnd)) return 0;
    h = GetClipboardData(is_nt ? CF_UNICODETEXT : CF_TEXT);
    if (h) {
        void *m = GlobalLock(h);
        if (m) {
            if (is_nt) n = from_wide((const WCHAR *)m, out, cap);
            else { const char *c = (const char *)m; while (*c && n < cap - 1) { if ((unsigned char)*c < 0x80) out[n++] = *c; c++; } out[n] = 0; }
            GlobalUnlock(h);
        }
    }
    CloseClipboard();
    return n;
}
void plat_clip_set(const char *u) {
    static WCHAR w[1024]; int n, i; HGLOBAL g; void *m;
    if (headless) { ys_cpy(test_clip, u, sizeof test_clip); return; }
    n = to_wide(u, w, 1024);
    g = GlobalAlloc(GMEM_MOVEABLE, (DWORD)(n + 1) * 2);
    if (!g) return;
    m = GlobalLock(g);
    if (!m) return;
    if (is_nt) for (i = 0; i <= n; i++) ((WCHAR *)m)[i] = w[i];
    else { int k = 0; for (i = 0; i < n; i++) if (w[i] < 0x80) ((char *)m)[k++] = (char)w[i]; ((char *)m)[k] = 0; }
    GlobalUnlock(g);
    if (OpenClipboard(wnd)) {
        EmptyClipboard();
        /* the clipboard owns the memory once this succeeds */
        SetClipboardData(is_nt ? CF_UNICODETEXT : CF_TEXT, g);
        CloseClipboard();
    }
}
int plat_open(const char *u) {
    if (headless) return 1;
    if (is_nt) {
        typedef HINSTANCE (WINAPI *SEW)(HWND, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, INT);
        SEW f = (SEW)GetProcAddress(GetModuleHandleA("shell32.dll"), "ShellExecuteW");
        if (f) { static WCHAR w[1024]; to_wide(u, w, 1024); return (INT_PTR)f(wnd, 0, w, 0, 0, SW_SHOWNORMAL) > 32; }
    }
    return (INT_PTR)ShellExecuteA(wnd, 0, u, 0, 0, SW_SHOWNORMAL) > 32;
}

/* ---- material folder scan ---- */
#define MAX_MAT 800
static char *mat_names[MAX_MAT], *mat_full[MAX_MAT];
static int mat_count = -1;
static char mat_pool[300000];
static int mat_used;
static char *pool_add(const char *s) {
    int n = ys_len(s) + 1; char *r;
    if (mat_used + n > (int)sizeof mat_pool) return 0;
    r = mat_pool + mat_used; ys_cpy(r, s, n); mat_used += n; return r;
}
static int want_ext(const char *f) {
    static const char *const ex[] = {".mp3", ".mp4", ".pdf", ".epub", ".doc"};
    int i, l = ys_len(f);
    for (i = 0; i < 5; i++) { int e = ys_len(ex[i]); if (l > e && ys_contains_ci(f + l - e, ex[i])) return 1; }
    return 0;
}
typedef HANDLE (WINAPI *FFW)(LPCWSTR, WIN32_FIND_DATAW *);
typedef BOOL (WINAPI *FNW)(HANDLE, WIN32_FIND_DATAW *);
static FFW ffw; static FNW fnw;
static void scan(const char *base, const char *rel, int depth) {
    static WCHAR wp[1024]; char sub[700], full[700];
    WIN32_FIND_DATAW fw; WIN32_FIND_DATAA fa; HANDLE h;
    if (depth > 5 || mat_count >= MAX_MAT) return;
    ys_cpy(full, base, 700); if (*rel) { ys_cat(full, "\\", 700); ys_cat(full, rel, 700); }
    ys_cpy(sub, full, 700); ys_cat(sub, "\\*", 700);
    if (ffw) { to_wide(sub, wp, 1024); h = ffw(wp, &fw); } else h = FindFirstFileA(sub, &fa);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        char nm[300], r2[700]; int isdir;
        if (ffw) { from_wide(fw.cFileName, nm, 300); isdir = fw.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY; }
        else { ys_cpy(nm, fa.cFileName, 300); isdir = fa.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY; }
        if (nm[0] == '.') continue;
        ys_cpy(r2, rel, 700); if (*rel) ys_cat(r2, "\\", 700); ys_cat(r2, nm, 700);
        if (isdir) scan(base, r2, depth + 1);
        else if (want_ext(nm) && mat_count < MAX_MAT) {
            char f2[700]; char *a, *b;
            ys_cpy(f2, base, 700); ys_cat(f2, "\\", 700); ys_cat(f2, r2, 700);
            a = pool_add(r2); b = pool_add(f2);
            if (a && b) { mat_names[mat_count] = a; mat_full[mat_count] = b; mat_count++; }
        }
    } while (ffw ? fnw(h, &fw) : FindNextFileA(h, &fa));
    FindClose(h);
}
int plat_scan_material(void) {
    char b[MAX_PATH];
    if (mat_count >= 0) return mat_count;
    mat_count = 0;
    if (is_nt) {
        HMODULE k = GetModuleHandleA("kernel32.dll");
        ffw = (FFW)GetProcAddress(k, "FindFirstFileW"); fnw = (FNW)GetProcAddress(k, "FindNextFileW");
    }
    ys_cpy(b, root, MAX_PATH); ys_cat(b, "\\Material", MAX_PATH); scan(b, "", 0);
    if (!mat_count) { ys_cpy(b, root, MAX_PATH); ys_cat(b, "\\..\\Material", MAX_PATH); scan(b, "", 0); }
    if (!mat_count) scan("C:\\YomiGO\\Material", "", 0);
    return mat_count;
}
const char *plat_material(int i) { return mat_names[i]; }
const char *plat_material_full(int i) { return mat_full[i]; }

/* ---- sound: square-wave blips made at start-up, played by PlaySound ---- */
static char wav[5][12000];
static void make_wav(int k, int freq, int freq2, int ms) {
    char *w = wav[k]; int rate = 11025, n = rate * ms / 1000, i, per, total;
    if (n > 11900) n = 11900;
    total = n + 36;
    memcpy(w, "RIFF", 4); *(int *)(w + 4) = total; memcpy(w + 8, "WAVEfmt ", 8); *(int *)(w + 16) = 16;
    *(short *)(w + 20) = 1; *(short *)(w + 22) = 1; *(int *)(w + 24) = rate; *(int *)(w + 28) = rate; *(short *)(w + 32) = 1; *(short *)(w + 34) = 8;
    memcpy(w + 36, "data", 4); *(int *)(w + 40) = n;
    for (i = 0; i < n; i++) {
        int f = i < n / 2 ? freq : freq2, vol = 50 * (n - i) / n;
        per = rate / f;
        w[44 + i] = (char)(128 + ((i % per) < per / 2 ? vol : -vol));
    }
}
void plat_sound(int k) {
    if (headless || !sound_on) return;
    if (k < 0 || k > 4) return;
    PlaySoundA(wav[k], 0, SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
}

/* ---- speech: uses SAPI when a Japanese voice is installed, looked up at run time ---- */
typedef struct ISpVoiceVtbl_ { void *q, *a; ULONG (STDMETHODCALLTYPE *Release)(void *); void *pad[17]; HRESULT (STDMETHODCALLTYPE *Speak)(void *, const WCHAR *, DWORD, ULONG *); } ISpVoiceVtbl_;
typedef struct { const ISpVoiceVtbl_ *v; } ISpVoice_;
static ISpVoice_ *voice;
static int tts_state; /* 0 unknown, 1 yes, 2 no */
int plat_has_tts(void) {
    typedef HRESULT (WINAPI *CCI)(REFCLSID, void *, DWORD, REFIID, void **);
    typedef HRESULT (WINAPI *CI)(void *);
    static const GUID CLSID_SpVoice_ = {0x96749377, 0x3391, 0x11D2, {0x9E, 0xE3, 0x00, 0xC0, 0x4F, 0x79, 0x73, 0x96}};
    static const GUID IID_ISpVoice_ = {0x6C44DF74, 0x72B9, 0x4992, {0xA1, 0xEC, 0xEF, 0x99, 0x6E, 0x04, 0x22, 0xD4}};
    HMODULE ole; CCI cci; CI ci; HKEY key;
    if (tts_state) return tts_state == 1;
    tts_state = 2;
    if (headless || !is_nt) return 0;
    /* Only offer it if Windows has a Japanese voice. Otherwise it would read Japanese as silence. */
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Speech_OneCore\\Voices\\Tokens\\MSTTS_V110_jaJP_HarukaM", 0, KEY_READ, &key) == 0) RegCloseKey(key);
    else if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Speech\\Voices\\Tokens\\TTS_MS_JA-JP_HARUKA_11.0", 0, KEY_READ, &key) == 0) RegCloseKey(key);
    else return 0;
    ole = LoadLibraryA("ole32.dll");
    if (!ole) return 0;
    ci = (CI)GetProcAddress(ole, "CoInitialize"); cci = (CCI)GetProcAddress(ole, "CoCreateInstance");
    if (!ci || !cci) return 0;
    ci(0);
    if (cci(&CLSID_SpVoice_, 0, 23 /* CLSCTX_ALL */, &IID_ISpVoice_, (void **)&voice) == 0 && voice) tts_state = 1;
    return tts_state == 1;
}
void plat_speak(const char *u) {
    static WCHAR w[512], x[600]; static const char pre[] = "<voice required=\"language=411\">"; int i, n = 0;
    if (!plat_has_tts()) return;
    for (i = 0; pre[i]; i++) x[n++] = (WCHAR)pre[i];
    to_wide(u, w, 512);
    for (i = 0; w[i] && n < 590; i++) x[n++] = w[i];
    x[n] = 0;
    voice->v->Speak(voice, x, 1 | 8, 0); /* SPF_ASYNC | SPF_IS_XML */
}

/* ---- updates live in a separate helper, so this program stays free of modern dependencies ---- */
int plat_can_update(void) {
    DWORD v = GetVersion(); int maj = v & 255, min = (v >> 8) & 255;
    return is_nt && (maj > 6 || (maj == 6 && min >= 1));
}
void plat_update_check(int quiet) {
    char cmd[MAX_PATH * 2], num[16]; STARTUPINFOA si; PROCESS_INFORMATION pi; int i; char *p = (char *)&si;
    if (headless || !plat_can_update()) return;
    for (i = 0; i < (int)sizeof si; i++) p[i] = 0;
    si.cb = sizeof si;
    ys_cpy(cmd, "\"", sizeof cmd); ys_cat(cmd, dir, sizeof cmd); ys_cat(cmd, "\\YomiGoUpdate.exe\" ", sizeof cmd);
#ifdef YG_HTPC
    ys_cat(cmd, "YomiGoHTPC.exe ", sizeof cmd);
#else
    ys_cat(cmd, "YomiGo.exe ", sizeof cmd);
#endif
    ys_cat(cmd, YG_VERSION " ", sizeof cmd);
    ys_itoa((int)GetCurrentProcessId(), num); ys_cat(cmd, num, sizeof cmd);
    if (quiet) ys_cat(cmd, " --quiet", sizeof cmd);
    if (CreateProcessA(0, cmd, 0, 0, FALSE, 0, 0, dir, &si, &pi)) { CloseHandle(pi.hProcess); CloseHandle(pi.hThread); }
}
static void daily_update_check(void) {
    char t[16]; int n, today = plat_today();
    if (headless || !plat_can_update()) return;
    n = plat_read("update.day", last_day_checked, 15);
    if (n > 0) { last_day_checked[n] = 0; if (ys_atoi(last_day_checked) == today) return; }
    n = ys_itoa(today, t);
    plat_write("update.day", t, n);
    plat_update_check(1);
}

/* ---- controllers: XInput where it exists, the Windows 95 joystick API for everything else ---- */
typedef struct { DWORD pk; WORD buttons; BYTE lt, rt; SHORT lx, ly, rx, ry; } XIS;
typedef DWORD (WINAPI *XIGS)(DWORD, XIS *);
static XIGS xi_get;
static int pad_id = -1, xi_id = -1, style;
static int pad_rescan;
static int binding = -1;
static int deadzone = 25;
/* Button numbers for: OK, Back, Extra, Speak, L, R, Start. Alternates let the C buttons work too, as in the N64 launcher. */
static int binds[7] = {1, 2, 0, 3, 4, 5, 9}; /* A/Cross, B/Circle, X/Square, Y/Triangle, L, R, Start */
static int alts[7] = {-1, -1, -1, -1, -1, -1, -1};
static int prompt_mode, custom, n64_known;
static void set_layout(const int *b, const int *a) { int i; for (i = 0; i < 7; i++) { if (!custom) binds[i] = b[i]; alts[i] = a[i]; } }
static void pad_layout(void) {
    static const int none[7] = {-1, -1, -1, -1, -1, -1, -1};
    static const int std_[7] = {1, 2, 0, 3, 4, 5, 9};
    /* From the launcher: USB N64 pad 0E6D:111D, then other replicas. A, B, Z, C-Up, L, R, Start; C-Down, C-Left, C-Right as alternates. */
    static const int known[7] = {2, 1, 6, 9, 4, 5, 12}, known_alt[7] = {-1, -1, 3, -1, 0, 8, -1};
    static const int raw[7] = {0, 1, 6, 3, 4, 5, 9}, raw_alt[7] = {-1, -1, 2, -1, 7, 8, -1};
    /* An Xbox pad showing N64 prompts: triggers are Z, right stick is the C buttons */
    static const int xi[7] = {1, 2, 6, 3, 4, 5, 9}, xi_alt[7] = {-1, -1, 18, -1, 16, 17, -1};
    if (n64_known) set_layout(known, known_alt);
    else if (prompt_mode == 1) { if (xi_id >= 0) set_layout(xi, xi_alt); else set_layout(raw, raw_alt); }
    else set_layout(std_, none);
}
static unsigned prev_raw;
static int prev_dirs;

int pad_connected(void) { return pad_id >= 0 || xi_id >= 0; }
int pad_style(void) {
    if (prompt_mode == 1) return 4;
    if (prompt_mode == 2) return 1;
    if (prompt_mode == 3) return 2;
    return pad_connected() ? (n64_known ? 4 : style) : 0;
}
int pad_mode(void) { return prompt_mode; }
void pad_set_mode(int m) { prompt_mode = m; pad_layout(); pad_save(); }
void pad_begin_bind(int a) { binding = a; custom = 1; }
int pad_binding(void) { return binding >= 0; }
int pad_bound(int a) { return binds[a]; }
void pad_set_deadzone(int d) { deadzone = d; }
int pad_deadzone(void) { return deadzone; }
int crt_mode(void) { return crt; }
void crt_set(int on) { crt = on; pad_save(); }
void pad_save(void) {
    char b[128]; int n = 0, i;
    for (i = 0; i < 7; i++) { n += ys_itoa(binds[i], b + n); b[n++] = ' '; }
    n += ys_itoa(deadzone, b + n); b[n++] = ' ';
    n += ys_itoa(crt, b + n); b[n++] = ' ';
    n += ys_itoa(prompt_mode, b + n); b[n++] = ' ';
    n += ys_itoa(custom, b + n); b[n++] = '\n'; b[n] = 0;
    plat_write("input.cfg", b, n);
}
static void pad_load(void) {
    char b[128]; int n = plat_read("input.cfg", b, 127), i; const char *p = b;
    if (n <= 0) return;
    b[n] = 0;
    for (i = 0; i < 11; i++) {
        int v = ys_atoi(p);
        if (i < 7) { if (v >= 0 && v < 32) binds[i] = v; }
        else if (i == 7) deadzone = (v >= 0 && v <= 80) ? v : 25;
        else if (i == 8) crt = v != 0;
        else if (i == 9) prompt_mode = (v >= 0 && v <= 3) ? v : 0;
        else custom = v != 0;
        while (*p && *p != ' ') p++;
        if (*p) p++;
    }
}
static void pad_find(void) {
    int i, n;
    pad_id = -1; xi_id = -1; n64_known = 0;
    if (xi_get) for (i = 0; i < 4; i++) { XIS s; if (xi_get((DWORD)i, &s) == 0) { xi_id = i; style = 1; pad_layout(); return; } }
    n = (int)joyGetNumDevs();
    for (i = 0; i < n && i < 16; i++) {
        JOYINFOEX j; JOYCAPSA c;
        j.dwSize = sizeof j; j.dwFlags = JOY_RETURNALL;
        if (joyGetPosEx((UINT)i, &j) == JOYERR_NOERROR && joyGetDevCapsA((UINT)i, &c, sizeof c) == JOYERR_NOERROR) {
            pad_id = i;
            style = c.wMid == 0x054C ? 2 : 3; /* Sony's vendor ID means PlayStation prompts */
            n64_known = c.wMid == 0x0E6D && c.wPid == 0x111D;
            pad_layout();
            return;
        }
    }
}
/* Fold whatever the pad reports into menu actions, with key repeat left to the press edges */
static int pad_poll(void) {
    unsigned raw = 0; int dirs = 0, act = 0, i;
    if (--pad_rescan <= 0 || !pad_connected()) {
        if (pad_rescan <= 0) { pad_find(); pad_rescan = 60; }
    }
    if (xi_id >= 0 && xi_get) {
        XIS s; int dz = deadzone * 32767 / 100; WORD b;
        if (xi_get((DWORD)xi_id, &s)) { xi_id = -1; return 0; }
        b = s.buttons;
        if (b & 0x1000) raw |= 1u << 1;   /* A */
        if (b & 0x2000) raw |= 1u << 2;   /* B */
        if (b & 0x4000) raw |= 1u << 0;   /* X */
        if (b & 0x8000) raw |= 1u << 3;   /* Y */
        if (b & 0x0100) raw |= 1u << 4;   /* LB */
        if (b & 0x0200) raw |= 1u << 5;   /* RB */
        if (b & 0x0010) raw |= 1u << 9;   /* Start */
        if (b & 0x0020) raw |= 1u << 8;   /* Back */
        if (s.lt > 100 || s.rt > 100) raw |= 1u << 6;
        if (s.rx < -16000) raw |= 1u << 16;
        if (s.rx > 16000) raw |= 1u << 17;
        if (s.ry < -16000) raw |= 1u << 18;
        if ((b & 1) || s.ly > dz) dirs |= A_UP;
        if ((b & 2) || s.ly < -dz) dirs |= A_DOWN;
        if ((b & 4) || s.lx < -dz) dirs |= A_LEFT;
        if ((b & 8) || s.lx > dz) dirs |= A_RIGHT;
    } else if (pad_id >= 0) {
        JOYINFOEX j; int dz = deadzone * 32767 / 100;
        j.dwSize = sizeof j; j.dwFlags = JOY_RETURNALL;
        if (joyGetPosEx((UINT)pad_id, &j) != JOYERR_NOERROR) { pad_id = -1; return 0; }
        raw = j.dwButtons;
        if ((int)j.dwYpos - 32767 < -dz) dirs |= A_UP;
        if ((int)j.dwYpos - 32767 > dz) dirs |= A_DOWN;
        if ((int)j.dwXpos - 32767 < -dz) dirs |= A_LEFT;
        if ((int)j.dwXpos - 32767 > dz) dirs |= A_RIGHT;
        if (j.dwPOV != 0xFFFF) {
            int p = (int)j.dwPOV;
            if (p > 27000 || p < 9000) dirs |= A_UP;
            if (p > 9000 && p < 27000) dirs |= A_DOWN;
            if (p > 0 && p < 18000) dirs |= A_RIGHT;
            if (p > 18000) dirs |= A_LEFT;
        }
    } else return 0;
    if (binding >= 0) {
        for (i = 0; i < 32; i++) if ((raw & (1u << i)) && !(prev_raw & (1u << i))) { binds[binding] = i; binding = -1; break; }
        prev_raw = raw; prev_dirs = dirs;
        return 0;
    }
    { static const int m[7] = {A_OK, A_BACK, A_X, A_Y, A_L, A_R, A_START};
      for (i = 0; i < 7; i++) {
          if ((raw & (1u << binds[i])) && !(prev_raw & (1u << binds[i]))) act |= m[i];
          if (alts[i] >= 0 && (raw & (1u << alts[i])) && !(prev_raw & (1u << alts[i]))) act |= m[i];
      } }
    act |= dirs & ~prev_dirs;
    prev_raw = raw; prev_dirs = dirs;
    return act;
}

/* ---- window ---- */
static BITMAPINFO bmi;
static Input pending;
static void add_text(int cp) {
    int n = ys_len(pending.text);
    /* no surrogates: the font has nothing beyond the basic plane */
    if (n < 248 && cp >= 32 && (cp < 0xD800 || cp > 0xDFFF)) n += utf8_put(cp, pending.text + n), pending.text[n] = 0;
}
static int cw = FBW, ch = FBH;

/* Whole-number scaling keeps every font pixel crisp, with bars filling what's left.
   A TV stretches to fill, as a console would. So does a window too small for the picture, as on a 640x480 screen. */
static void dest_rect(int *ox, int *oy, int *sx, int *sy) {
    int scale = cw / FBW;
    if (ch / FBH < scale) scale = ch / FBH;
#ifdef YG_HTPC
    scale = 0;
#endif
    if (scale < 1) { *ox = *oy = 0; *sx = cw ? cw : 1; *sy = ch ? ch : 1; return; }
    *sx = FBW * scale; *sy = FBH * scale;
    *ox = (cw - *sx) / 2; *oy = (ch - *sy) / 2;
}
static void present(HDC dc) {
    int sx, sy, ox, oy;
    dest_rect(&ox, &oy, &sx, &sy);
#ifdef YG_HTPC
    if (crt) { int i; for (i = 0; i < FBW * FBH; i++) { unsigned c = FB[i]; unsigned r = c >> 16 & 255, g = c >> 8 & 255, b = c & 255; if (r > 224) r = 224; if (g > 224) g = 224; if (b > 224) b = 224; FB[i] = r << 16 | g << 8 | b; } }
#endif
    if (ox > 0 || oy > 0) {
        HBRUSH br = (HBRUSH)GetStockObject(BLACK_BRUSH); RECT r;
        r.left = 0; r.top = 0; r.right = cw; r.bottom = oy; FillRect(dc, &r, br);
        r.top = oy + sy; r.bottom = ch; FillRect(dc, &r, br);
        r.top = oy; r.bottom = oy + sy; r.right = ox; FillRect(dc, &r, br);
        r.left = ox + sx; r.right = cw; FillRect(dc, &r, br);
    }
    SetStretchBltMode(dc, COLORONCOLOR);
    StretchDIBits(dc, ox, oy, sx, sy, 0, 0, FBW, FBH, FB, &bmi, DIB_RGB_COLORS, SRCCOPY);
}
/* Japanese keyboard. The apps draw everything themselves, so the text being composed goes in the answer box,
   and the candidate list opens beside it. imm32 is looked up at run time, because 95 may not have it. */
typedef struct { DWORD style; POINT pt; RECT area; } CompForm_;
typedef struct { DWORD index, style; POINT pt; RECT area; } CandForm_;
static void *(WINAPI *imm_get)(HWND);
static BOOL (WINAPI *imm_rel)(HWND, void *);
static LONG (WINAPI *imm_str)(void *, DWORD, void *, DWORD);
static void *(WINAPI *imm_assoc)(HWND, void *);
static BOOL (WINAPI *imm_cand)(void *, CandForm_ *);
static BOOL (WINAPI *imm_comp)(void *, CompForm_ *);
static char ime_comp[256];
static int ime_x, ime_y, ime_on = 1;
static void *ime_saved;
static void ime_load(void) {
    HMODULE m = is_nt ? LoadLibraryA("imm32.dll") : 0;
    if (!m) return;
    *(FARPROC *)&imm_get = GetProcAddress(m, "ImmGetContext");
    *(FARPROC *)&imm_rel = GetProcAddress(m, "ImmReleaseContext");
    *(FARPROC *)&imm_str = GetProcAddress(m, "ImmGetCompositionStringW");
    *(FARPROC *)&imm_assoc = GetProcAddress(m, "ImmAssociateContext");
    *(FARPROC *)&imm_cand = GetProcAddress(m, "ImmSetCandidateWindow");
    *(FARPROC *)&imm_comp = GetProcAddress(m, "ImmSetCompositionWindow");
    if (!imm_get || !imm_rel || !imm_cand || !imm_comp) imm_str = 0;
}
static void ime_place(void) {
    void *c; int ox, oy, sx, sy; CandForm_ cf; CompForm_ co; int i;
    if (!imm_str || !wnd || !(c = imm_get(wnd))) return;
    dest_rect(&ox, &oy, &sx, &sy);
    for (i = 0; i < (int)sizeof cf; i++) ((char *)&cf)[i] = 0;
    for (i = 0; i < (int)sizeof co; i++) ((char *)&co)[i] = 0;
    cf.style = 0x40; /* CFS_CANDIDATEPOS */
    cf.pt.x = ox + ime_x * sx / FBW; cf.pt.y = oy + (ime_y + 26) * sy / FBH;
    imm_cand(c, &cf);
    co.style = 2; /* CFS_POINT */
    co.pt = cf.pt;
    imm_comp(c, &co);
    imm_rel(wnd, c);
}
static void ime_composition(LPARAM l) {
    static WCHAR w[256]; void *c; LONG n; int i;
    if (!(c = imm_get(wnd))) return;
    if (l & 0x800) { /* GCS_RESULTSTR: finished text */
        n = imm_str(c, 0x800, w, sizeof w - 2);
        if (n > 0) { n /= 2; w[n] = 0; for (i = 0; i < n; i++) add_text(w[i]); }
        ime_comp[0] = 0;
    }
    if (l & 0x8) { /* GCS_COMPSTR: still being typed */
        n = imm_str(c, 0x8, w, sizeof w - 2);
        if (n < 0) n = 0;
        n /= 2; w[n] = 0;
        from_wide(w, ime_comp, sizeof ime_comp);
    }
    imm_rel(wnd, c);
    ime_place();
}
const char *plat_ime_comp(void) { return ime_comp; }
/* With no text box showing, the keyboard is switched off, so menu keys can't vanish into an invisible composition */
void plat_ime_field(int on, int x, int y) {
    if (on) { ime_x = x; ime_y = y; }
    if (on == ime_on || !imm_assoc || !wnd) { ime_on = on; return; }
    ime_on = on;
    if (on) { imm_assoc(wnd, ime_saved); ime_place(); }
    else { ime_saved = imm_assoc(wnd, 0); ime_comp[0] = 0; }
}
static void map_mouse(LPARAM lp) {
    int x = (short)LOWORD(lp), y = (short)HIWORD(lp), ox, oy, sx, sy;
    dest_rect(&ox, &oy, &sx, &sy);
    pending.mx = (x - ox) * FBW / sx; pending.my = (y - oy) * FBH / sy;
}
static LRESULT CALLBACK proc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_DESTROY: PostQuitMessage(0); return 0;
    case WM_SIZE: cw = LOWORD(l); ch = HIWORD(l); return 0;
    case WM_PAINT: { PAINTSTRUCT ps; HDC dc = BeginPaint(h, &ps); present(dc); EndPaint(h, &ps); return 0; }
    case WM_ERASEBKGND: return 1;
    case 0x0281: l &= ~(LPARAM)0x80000000; break; /* WM_IME_SETCONTEXT: YomiGo draws the composition itself */
    case 0x010D: if (imm_str) { ime_place(); return 0; } break; /* WM_IME_STARTCOMPOSITION */
    case 0x010E: if (imm_str) { ime_comp[0] = 0; return 0; } break; /* WM_IME_ENDCOMPOSITION */
    case 0x010F: if (imm_str) { ime_composition(l); return 0; } break; /* WM_IME_COMPOSITION */
    case WM_MOUSEMOVE: map_mouse(l); if (pending.mdown != 1) pending.mdown = 2; return 0;
    case WM_LBUTTONDOWN: map_mouse(l); pending.mdown = 1; SetCapture(h); return 0;
    case WM_LBUTTONUP: map_mouse(l); pending.mdown = 0; pending.mclick = 1; ReleaseCapture(); return 0;
    case WM_KEYDOWN:
        switch (w) {
        case VK_UP: pending.pressed |= A_UP; break;
        case VK_DOWN: pending.pressed |= A_DOWN; break;
        case VK_LEFT: pending.pressed |= A_LEFT; break;
        case VK_RIGHT: pending.pressed |= A_RIGHT; break;
        case VK_RETURN: pending.pressed |= A_OK; break;
        case VK_ESCAPE: pending.pressed |= A_BACK; break;
        case VK_BACK: pending.backspace++; break;
        case VK_F1: pending.pressed |= A_Y; break;
        case VK_PRIOR: pending.pressed |= A_L; break;
        case VK_NEXT: pending.pressed |= A_R; break;
        case VK_F2: pending.pressed |= A_START; break;
        case VK_F3: pending.pressed |= A_X; break;
#ifdef YG_HTPC
        case VK_SPACE: pending.pressed |= A_START; break;
#endif
        }
        return 0;
    case WM_CHAR:
        if (w == 22) { pending.paste = 1; return 0; } /* Ctrl+V */
        if (w == 3) { pending.copy = 1; return 0; }   /* Ctrl+C */
        if (w == 8 || w == 13 || w == 27) return 0;
#ifdef YG_HTPC
        if (w == ' ') return 0; /* Space is Start here */
#endif
        if (w >= '1' && w <= '4') pending.num = (int)w - '0';
        add_text((int)w);
        return 0;
    case WM_TIMER:
        pending.pressed |= pad_poll();
        IN = pending;
        pending.pressed = 0; pending.mclick = 0; pending.text[0] = 0; pending.backspace = 0; pending.num = 0; pending.paste = 0; pending.copy = 0;
        app_frame();
        { HDC dc = GetDC(h); present(dc); ReleaseDC(h, dc); }
        if (quit_now) DestroyWindow(h);
        return 0;
    }
    /* looked up at run time: the wide version is only meaningful on NT */
    { typedef LRESULT (WINAPI *DWPW)(HWND, UINT, WPARAM, LPARAM); static DWPW dw;
      if (is_nt && !dw) dw = (DWPW)GetProcAddress(GetModuleHandleA("user32.dll"), "DefWindowProcW");
      return is_nt && dw ? dw(h, m, w, l) : DefWindowProcA(h, m, w, l); }
}

/* ---- headless driver for tests: renders frames with no window, so nothing steals focus ---- */
static void write_bmp(const char *name) {
    char p[MAX_PATH]; HANDLE h; DWORD put; BITMAPFILEHEADER fh; BITMAPINFOHEADER ih;
    ys_cpy(p, root, MAX_PATH); ys_cat(p, "\\", MAX_PATH); ys_cat(p, name, MAX_PATH); ys_cat(p, ".bmp", MAX_PATH);
    fh.bfType = 0x4D42; fh.bfSize = sizeof fh + sizeof ih + FBW * FBH * 4; fh.bfReserved1 = fh.bfReserved2 = 0; fh.bfOffBits = sizeof fh + sizeof ih;
    ih = bmi.bmiHeader; ih.biHeight = FBH;
    h = CreateFileA(p, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if (h == INVALID_HANDLE_VALUE) return;
    WriteFile(h, &fh, sizeof fh, &put, 0); WriteFile(h, &ih, sizeof ih, &put, 0);
    { int y; for (y = FBH - 1; y >= 0; y--) WriteFile(h, FB + y * FBW, FBW * 4, &put, 0); }
    CloseHandle(h);
}
static int step_paste, step_copy;
/* Win95-style command lines are ANSI, so test scripts spell Japanese as \uXXXX */
static void unescape(char *b) {
    char *r = b, *w = b;
    while (*r) {
        if (r[0] == '\\' && r[1] == 'u') {
            int i, v = 0;
            for (i = 2; i < 6; i++) { char c = r[i]; v = v * 16 + (c >= 'a' ? c - 'a' + 10 : c >= 'A' ? c - 'A' + 10 : c - '0'); }
            w += utf8_put(v, w); r += 6;
        } else *w++ = *r++;
    }
    *w = 0;
}
static void step(int actions, const char *txt, int num, int back) {
    int i;
    for (i = 0; i < (int)sizeof IN; i++) ((char *)&IN)[i] = 0;
    IN.mx = IN.my = -1;
    IN.pressed = actions; IN.num = num; IN.backspace = back;
    IN.paste = step_paste; IN.copy = step_copy; step_paste = step_copy = 0;
    if (txt) ys_cpy(IN.text, txt, sizeof IN.text);
    app_frame();
}
static void run_script(const char *s) {
    /* keys: u d l r o(ok) b(back) s(start) y x(extra) V(paste) C(copy); 1-4 pick; "t<text>;" type; "k<text>;" composing; "w<frames>;" wait; "p<name>;" screenshot */
    char buf[128];
    step(0, 0, 0, 0);
    while (*s) {
        char c = *s++; int n = 0;
        if (c == 't' || c == 'w' || c == 'p' || c == 'k') { while (*s && *s != ';' && n < 127) buf[n++] = *s++; buf[n] = 0; if (*s) s++; unescape(buf); }
        switch (c) {
        case 'u': step(A_UP, 0, 0, 0); break; case 'd': step(A_DOWN, 0, 0, 0); break;
        case 'l': step(A_LEFT, 0, 0, 0); break; case 'r': step(A_RIGHT, 0, 0, 0); break;
        case 'o': step(A_OK, 0, 0, 0); break; case 'b': step(A_BACK, 0, 0, 0); break;
        case 's': step(A_START, 0, 0, 0); break; case 'y': step(A_Y, 0, 0, 0); break;
        case 'x': step(A_X, 0, 0, 0); break;
        case 'V': step_paste = 1; step(0, 0, 0, 0); break;
        case 'C': step_copy = 1; step(0, 0, 0, 0); break;
        case 'R': step(A_R, 0, 0, 0); break; case 'L': step(A_L, 0, 0, 0); break;
        case '1': case '2': case '3': case '4': step(0, 0, c - '0', 0); break;
        case 't': step(0, buf, 0, 0); break;
        case 'w': { int k = ys_atoi(buf); while (k-- > 0) step(0, 0, 0, 0); break; }
        case 'p': step(0, 0, 0, 0); write_bmp(buf); break;
        case 'k': ys_cpy(ime_comp, buf, sizeof ime_comp); break; /* as if the keyboard were composing this */
        case 'G': { /* play on until the course is complete, or give up after a very long time */
            int k = 200000;
            while (!app_test_done() && k-- > 0) { step(0, 0, 0, 0); app_test_auto(); app_frame(); }
            if (!app_test_done()) ExitProcess(9);
            break; }
        default: break;
        }
    }
}

/* Win95 gives a plain command line, so this finds --flags by hand */
static const char *arg_value(const char *cl, const char *flag, char *out, int cap) {
    int i, n = ys_len(flag);
    for (i = 0; cl[i]; i++) if (!ys_ncmp(cl + i, flag, n)) {
        const char *p = cl + i + n; int k = 0; char end = ' ';
        while (*p == ' ') p++;
        if (*p == '"') { end = '"'; p++; }
        while (*p && *p != end && k < cap - 1) out[k++] = *p++;
        out[k] = 0;
        return out;
    }
    return 0;
}
void yg_entry(void) {
    DWORD ver = GetVersion();
    const char *cl = GetCommandLineA();
    static char script[8192];
    int i; HINSTANCE inst = GetModuleHandleA(0);
    is_nt = !(ver & 0x80000000u);
    GetModuleFileNameA(0, dir, MAX_PATH);
    for (i = ys_len(dir) - 1; i > 0 && dir[i] != '\\'; i--) {}
    dir[i] = 0;
    if (!arg_value(cl, "--root", root, MAX_PATH)) { ys_cpy(root, dir, MAX_PATH); ys_cat(root, "\\saves", MAX_PATH); }
    CreateDirectoryA(root, 0);
    bmi.bmiHeader.biSize = sizeof bmi.bmiHeader; bmi.bmiHeader.biWidth = FBW; bmi.bmiHeader.biHeight = -FBH;
    bmi.bmiHeader.biPlanes = 1; bmi.bmiHeader.biBitCount = 32; bmi.bmiHeader.biCompression = BI_RGB;
    pad_load();
    if (arg_value(cl, "--script", script, sizeof script)) {
        headless = 1;
        yg_seed(12345);
        app_init();
        run_script(script);
        ExitProcess(0);
    }
    make_wav(0, 880, 880, 25); make_wav(1, 660, 990, 60); make_wav(2, 784, 1175, 120); make_wav(3, 523, 1046, 300); make_wav(4, 196, 147, 180);
    ime_load();
    { HMODULE x = LoadLibraryA("xinput1_4.dll"); if (!x) x = LoadLibraryA("xinput1_3.dll"); if (!x) x = LoadLibraryA("xinput9_1_0.dll");
      if (x) xi_get = (XIGS)GetProcAddress(x, "XInputGetState"); }
    pad_find();
    app_init();
    {
        DWORD st = WS_OVERLAPPEDWINDOW; RECT r; int ww, wh;
#ifdef YG_HTPC
        /* A TV app: borderless, at the size of the screen */
        st = WS_POPUP; r.left = r.top = 0; r.right = GetSystemMetrics(SM_CXSCREEN); r.bottom = GetSystemMetrics(SM_CYSCREEN);
#else
        r.left = r.top = 0; r.right = FBW; r.bottom = FBH;
        AdjustWindowRect(&r, st, FALSE);
#endif
        ww = r.right - r.left; wh = r.bottom - r.top;
        /* Unicode window on NT so the Japanese keyboard's text arrives intact; RegisterClassW is absent or a stub on 9x */
        if (is_nt) {
            typedef ATOM (WINAPI *RCW)(const WNDCLASSW *);
            typedef HWND (WINAPI *CWW)(DWORD, LPCWSTR, LPCWSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);
            HMODULE u = GetModuleHandleA("user32.dll");
            RCW rc = (RCW)GetProcAddress(u, "RegisterClassW"); CWW cw_ = (CWW)GetProcAddress(u, "CreateWindowExW");
            WNDCLASSW wc; WCHAR cn[] = {'Y','o','m','i','G','o','W','n','d',0}; WCHAR tt[] = {'Y','o','m','i','G','o',0};
            for (i = 0; i < (int)sizeof wc; i++) ((char *)&wc)[i] = 0;
            wc.lpfnWndProc = (WNDPROC)proc; wc.hInstance = inst; wc.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW); wc.lpszClassName = cn; wc.style = CS_DBLCLKS;
            rc(&wc);
            wnd = cw_(0, cn, tt, st | WS_VISIBLE, st == WS_POPUP ? 0 : CW_USEDEFAULT, st == WS_POPUP ? 0 : CW_USEDEFAULT, ww, wh, 0, 0, inst, 0);
        } else {
            WNDCLASSA wc;
            for (i = 0; i < (int)sizeof wc; i++) ((char *)&wc)[i] = 0;
            wc.lpfnWndProc = (WNDPROC)proc; wc.hInstance = inst; wc.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW); wc.lpszClassName = "YomiGoWnd";
            RegisterClassA(&wc);
            wnd = CreateWindowExA(0, "YomiGoWnd", "YomiGo", st | WS_VISIBLE, st == WS_POPUP ? 0 : CW_USEDEFAULT, st == WS_POPUP ? 0 : CW_USEDEFAULT, ww, wh, 0, 0, inst, 0);
        }
    }
    if (!wnd) ExitProcess(1);
#ifdef YG_HTPC
    ShowCursor(FALSE);
#endif
    daily_update_check();
    SetTimer(wnd, 1, 33, 0);
    {
        MSG msg;
        if (is_nt) {
            typedef BOOL (WINAPI *GMW)(LPMSG, HWND, UINT, UINT);
            typedef LRESULT (WINAPI *DMW)(const MSG *);
            HMODULE u = GetModuleHandleA("user32.dll");
            GMW gm = (GMW)GetProcAddress(u, "GetMessageW"); DMW dm = (DMW)GetProcAddress(u, "DispatchMessageW");
            while (gm(&msg, 0, 0, 0) > 0) { TranslateMessage(&msg); dm(&msg); }
        } else {
            while (GetMessageA(&msg, 0, 0, 0) > 0) { TranslateMessage(&msg); DispatchMessageA(&msg); }
        }
    }
    ExitProcess(0);
}
