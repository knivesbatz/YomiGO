#!/usr/bin/env python3
"""Prints the SHA-256 of the certificate that signed an APK (APK Signature Scheme v2 or v3), lowercase, no colons:
the same value apksigner verify --print-certs shows as "certificate SHA-256 digest". No Android SDK needed.
usage: python tools/apk_cert.py app.apk [expected-sha256]   exit 1 if unsigned, or not the expected certificate"""
import hashlib, struct, sys

def certs(path):
    data = open(path, "rb").read()
    eocd = data.rfind(b"PK\x05\x06")
    cd_off = struct.unpack_from("<I", data, eocd + 16)[0]
    if data[cd_off - 16:cd_off] != b"APK Sig Block 42":
        return []
    size = struct.unpack_from("<Q", data, cd_off - 24)[0]
    block = data[cd_off - size - 8:cd_off - 24]
    pairs, i, out = block[8:], 0, []
    while i + 12 <= len(pairs):
        ln, pid = struct.unpack_from("<QI", pairs, i)
        val = pairs[i + 12:i + 8 + ln]
        if pid in (0x7109871A, 0xF05368C0):  # v2, v3
            def seq(b, o):  # a length-prefixed sequence of length-prefixed items
                n = struct.unpack_from("<I", b, o)[0]; o += 4; end = o + n; items = []
                while o < end:
                    m = struct.unpack_from("<I", b, o)[0]; items.append(b[o + 4:o + 4 + m]); o += 4 + m
                return items
            for signer in seq(val, 0):
                signed = struct.unpack_from("<I", signer, 0)[0]
                sd = signer[4:4 + signed]
                digests_len = struct.unpack_from("<I", sd, 0)[0]
                for c in seq(sd, 4 + digests_len):
                    out.append(hashlib.sha256(c).hexdigest())
        i += 8 + ln
    return sorted(set(out))

if __name__ == "__main__":
    found = certs(sys.argv[1])
    print("\n".join(found) or "unsigned")
    if not found or (len(sys.argv) > 2 and found != [sys.argv[2].lower().replace(":", "")]):
        sys.exit(1)
