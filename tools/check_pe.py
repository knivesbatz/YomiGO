#!/usr/bin/env python3
"""Mark the apps loadable on Windows 95 and fail the build if they import anything Windows 95 lacks."""
import struct, sys

WIN95 = {
    "KERNEL32.dll": """CloseHandle CopyFileA CreateDirectoryA CreateFileA CreateProcessA DeleteFileA ExitProcess FindClose FindFirstFileA
        FindNextFileA FlushFileBuffers GetCommandLineA GetCurrentProcessId GetFileAttributesA WaitForSingleObject GetLocalTime GetModuleFileNameA GetModuleHandleA GetProcAddress
        GetExitCodeProcess GetTickCount GetVersion GlobalAlloc GlobalLock GlobalSize GlobalUnlock LoadLibraryA MoveFileA ReadFile Sleep WriteFile""",
    "USER32.dll": """AdjustWindowRect BeginPaint CloseClipboard CreateWindowExA DefWindowProcA DestroyWindow DispatchMessageA EmptyClipboard EndPaint FillRect GetClipboardData GetDC
        GetMessageA GetSystemMetrics LoadCursorA OpenClipboard PostQuitMessage RegisterClassA ReleaseCapture ReleaseDC SetCapture SetClipboardData SetTimer SetWindowPos ShowCursor TranslateMessage""",
    "GDI32.dll": "GetStockObject SetStretchBltMode StretchDIBits",
    "WINMM.dll": "PlaySoundA joyGetDevCapsA joyGetNumDevs joyGetPosEx",
    "SHELL32.dll": "ShellExecuteA",
    "ADVAPI32.dll": "RegCloseKey RegEnumKeyExA RegOpenKeyExA",
}
ALLOWED = {k.upper(): set(v.split()) for k, v in WIN95.items()}

def rva_to_off(secs, rva):
    for va, size, raw in secs:
        if va <= rva < va + size: return rva - va + raw
    raise ValueError(hex(rva))

def cstr(d, o):
    return d[o:d.index(0, o)].decode()

def check(path):
    d = bytearray(open(path, "rb").read())
    pe = struct.unpack_from("<I", d, 0x3C)[0]
    assert d[pe:pe+4] == b"PE\0\0", path
    machine, nsec = struct.unpack_from("<HH", d, pe + 4)
    assert machine == 0x14C, f"{path}: must be 32-bit x86"
    optsize = struct.unpack_from("<H", d, pe + 20)[0]
    opt = pe + 24
    # Windows 95 refuses anything that asks for a newer OS than 4.0
    struct.pack_into("<HH", d, opt + 40, 4, 0)  # OS version
    struct.pack_into("<HH", d, opt + 48, 4, 0)  # subsystem version
    struct.pack_into("<I", d, opt + 64, 0)      # checksum no longer applies
    secs = []
    for i in range(nsec):
        s = opt + optsize + i * 40
        vsize, va, rawsize, raw = struct.unpack_from("<IIII", d, s + 8)
        secs.append((va, max(vsize, rawsize), raw))
    imp = struct.unpack_from("<I", d, opt + 96 + 8)[0]
    o = rva_to_off(secs, imp)
    bad = []
    while True:
        ilt, _, _, name, iat = struct.unpack_from("<IIIII", d, o)
        if not name: break
        dll = cstr(d, rva_to_off(secs, name))
        allow = ALLOWED.get(dll.upper())
        if allow is None: bad.append(f"{dll} (whole DLL)")
        t = rva_to_off(secs, ilt or iat)
        while True:
            e = struct.unpack_from("<I", d, t)[0]
            if not e: break
            fn = cstr(d, rva_to_off(secs, e) + 2)
            if allow is not None and fn not in allow: bad.append(f"{dll}!{fn}")
            t += 4
        o += 20
    if bad:
        print(path, "imports functions Windows 95 lacks:", ", ".join(bad)); sys.exit(1)
    open(path, "wb").write(d)
    print(f"{path}: subsystem 4.0, imports all on the Windows 95 list")

for p in sys.argv[1:]:
    check(p)
