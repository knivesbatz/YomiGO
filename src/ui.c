#include "yg.h"
#include "ui.h"
#include "font_gen.h"

unsigned FB[FBW * FBH];
Input IN;

void fill(int x, int y, int w, int h, unsigned c) {
    int i, j;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > FBW) w = FBW - x;
    if (y + h > FBH) h = FBH - y;
    for (j = 0; j < h; j++) { unsigned *p = FB + (y + j) * FBW + x; for (i = 0; i < w; i++) p[i] = c; }
}
void frame(int x, int y, int w, int h, unsigned c) { fill(x, y, w, 1, c); fill(x, y + h - 1, w, 1, c); fill(x, y, 1, h, c); fill(x + w - 1, y, 1, h, c); }

/* Ordered dithering, as period 16-bit-colour multimedia titles did their gradients */
void gradient(int x, int y, int w, int h, unsigned a, unsigned b) {
    static const unsigned char B[4][4] = {{0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};
    int i, j;
    for (j = 0; j < h; j++) for (i = 0; i < w; i++) {
        int X = x + i, Y = y + j, t, r, g, bl, d;
        if (X < 0 || Y < 0 || X >= FBW || Y >= FBH) continue;
        t = h > 1 ? j * 256 / (h - 1) : 0;
        d = B[Y & 3][X & 3];
        r = ((a >> 16 & 255) * (256 - t) + (b >> 16 & 255) * t) / 256;
        g = ((a >> 8 & 255) * (256 - t) + (b >> 8 & 255) * t) / 256;
        bl = ((a & 255) * (256 - t) + (b & 255) * t) / 256;
        r = (r + d / 2) & 0xF8; g = (g + d / 2) & 0xF8; bl = (bl + d / 2) & 0xF8;
        FB[Y * FBW + X] = (unsigned)r << 16 | (unsigned)g << 8 | (unsigned)bl;
    }
}
void bevel(int x, int y, int w, int h, unsigned hi, unsigned face, unsigned lo) {
    fill(x, y, w, h, face);
    fill(x, y, w, 1, hi); fill(x, y, 1, h, hi);
    fill(x + 1, y + 1, w - 2, 1, hi); fill(x + 1, y + 1, 1, h - 2, hi);
    fill(x, y + h - 1, w, 1, lo); fill(x + w - 1, y, 1, h, lo);
    fill(x + 1, y + h - 2, w - 2, 1, lo); fill(x + w - 2, y + 1, 1, h - 2, lo);
}
/* rows of '.' (clear) and '0'-'9' (palette) */
void bitmap(int x, int y, const char *const *rows, int n, const unsigned *pal, int s) {
    int j, i;
    for (j = 0; j < n; j++) for (i = 0; rows[j][i]; i++) {
        char ch = rows[j][i];
        if (ch != '.') fill(x + i * s, y + j * s, s, s, pal[ch - '0']);
    }
}
void washi(int x, int y, int w, int h, unsigned base, unsigned fleck) {
    int i, j;
    fill(x, y, w, h, base);
    for (j = 0; j < h; j += 3) for (i = (j * 7) % 11; i < w; i += 11) {
        unsigned hsh = (unsigned)(i * 73856093u) ^ (unsigned)(j * 19349663u);
        if ((hsh >> 7 & 3) == 0 && x + i < FBW && y + j < FBH) FB[(y + j) * FBW + x + i] = fleck;
    }
}

static int glyph_index(int cp) {
    int lo = 0, hi = YG_NGLYPHS - 1;
    while (lo <= hi) {
        int m = (lo + hi) / 2, v = YG_GLYPH_CP[m];
        if (v == cp) return m;
        if (v < cp) lo = m + 1; else hi = m - 1;
    }
    return -1;
}
static int draw_glyph(int x, int y, int cp, unsigned c, int s) {
    int g = glyph_index(cp), j, i, w;
    if (g < 0) {
        w = cp < 0x100 ? 8 : 16;
        frame(x + 1, y + 2, (w - 2) * s, 12 * s, c); /* a visible box, so a missing glyph is never silent */
        return w * s;
    }
    w = YG_GLYPH_W[g];
    for (j = 0; j < 16; j++) {
        unsigned bits = YG_GLYPH_BITS[g * 16 + j];
        if (!bits) continue;
        for (i = 0; i < w; i++) if (bits & (0x8000u >> i)) {
            int X = x + i * s, Y = y + j * s;
            if (s == 1) { if (X >= 0 && Y >= 0 && X < FBW && Y < FBH) FB[Y * FBW + X] = c; }
            else fill(X, Y, s, s, c);
        }
    }
    return w * s;
}
int text_w(const char *s) {
    int c, w = 0;
    while ((c = utf8_next(&s)) != 0) { int g = glyph_index(c); w += g < 0 ? (c < 0x100 ? 8 : 16) : YG_GLYPH_W[g]; }
    return w;
}
int text(int x, int y, const char *s, unsigned col) { int c, x0 = x; while ((c = utf8_next(&s)) != 0) x += draw_glyph(x, y, c, col, 1); return x - x0; }
int text2(int x, int y, const char *s, unsigned col) { int c, x0 = x; while ((c = utf8_next(&s)) != 0) x += draw_glyph(x, y, c, col, 2); return x - x0; }
void text_c(int cx, int y, const char *s, unsigned c) { text(cx - text_w(s) / 2, y, s, c); }

/* Wraps at spaces for Latin text and anywhere in Japanese, keeping closing punctuation off a line start. Returns line count. */
static int no_line_start(int c) { return c == 0x3001 || c == 0x3002 || c == 0xFF01 || c == 0xFF1F || c == 0x3063 || c == 0x3083 || c == 0x3085 || c == 0x3087 || c == 0x30FC || c == 0xFF09 || c == 0x300D || c == ')' || c == ',' || c == '.' || c == '!' || c == '?'; }
int wrap(int x, int y, int w, const char *s, unsigned col, int lh, int skip, int maxl) {
    int line = 0;
    while (*s) {
        const char *p = s, *brk = 0, *nx; int used = 0, c;
        while ((nx = p, c = utf8_next(&nx)) != 0) {
            int g = glyph_index(c), cw = g < 0 ? (c < 0x100 ? 8 : 16) : YG_GLYPH_W[g];
            if (c == '\n') { brk = p; break; }
            if (used + cw > w) { if (!brk || c >= 0x80) { if (!no_line_start(c)) brk = p; else if (!brk) brk = p; } break; }
            if (c == ' ') brk = nx;
            else if (c >= 0x3000) { const char *q = nx; int nc = utf8_next(&q); if (!no_line_start(nc)) brk = nx; }
            used += cw; p = nx;
        }
        if (!c) brk = p;
        if (!brk || brk == s) brk = (p != s) ? p : nx;
        if (line >= skip && (maxl <= 0 || line - skip < maxl)) {
            const char *t = s; int X = x, Y = y + (line - skip) * lh;
            while (t < brk) { int d = utf8_next(&t); if (d != '\n') X += draw_glyph(X, Y, d, col, 1); }
        }
        s = brk;
        if (*s == '\n') s++;
        line++;
    }
    return line;
}

void line(int x0, int y0, int x1, int y1, unsigned c, int thick) {
    int dx = x1 > x0 ? x1 - x0 : x0 - x1, dy = y1 > y0 ? y0 - y1 : y1 - y0;
    int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1, err = dx + dy;
    for (;;) {
        int e2;
        fill(x0, y0, thick, thick, c);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

/* ---- flat-shaded triangles for the HTPC ---- */
void tri3d(int x0, int y0, int x1, int y1, int x2, int y2, unsigned c) {
    int y, t;
#define SWAP(a, b) { t = a; a = b; b = t; }
    if (y1 < y0) { SWAP(y0, y1) SWAP(x0, x1) }
    if (y2 < y0) { SWAP(y0, y2) SWAP(x0, x2) }
    if (y2 < y1) { SWAP(y1, y2) SWAP(x1, x2) }
#undef SWAP
    if (y2 == y0) return;
    for (y = y0; y <= y2; y++) {
        int xa = x0 + (x2 - x0) * (y - y0) / (y2 - y0), xb;
        if (y < y1) xb = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
        else if (y2 != y1) xb = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
        else xb = x1;
        if (xa > xb) { t = xa; xa = xb; xb = t; }
        fill(xa, y, xb - xa + 1, 1, c);
    }
}
static const short SIN_Q[65] = {0,6,13,19,25,31,38,44,50,56,62,68,74,80,86,92,98,104,109,115,121,126,132,137,142,147,152,157,162,167,172,177,181,185,190,194,198,202,206,209,213,216,220,223,226,229,231,234,237,239,241,243,245,247,248,250,251,252,253,254,255,255,256,256,256};
int isin(int a) {
    a &= 255;
    if (a <= 64) return SIN_Q[a];
    if (a <= 128) return SIN_Q[128 - a];
    if (a <= 192) return -SIN_Q[a - 128];
    return -SIN_Q[256 - a];
}
int icos(int a) { return isin(a + 64); }

/* ---- immediate-mode focus ---- */
#define MAX_HIT 96
static struct { short x, y, w, h; } hits[MAX_HIT], last[MAX_HIT];
static int nhit, nlast, focus = 0, this_focus;
void ui_reset_focus(void) { focus = 0; nlast = 0; }
void ui_set_focus(int id) { focus = id; }
void ui_begin(void) { nhit = 0; }
int ui_hit(int x, int y, int w, int h) {
    int id = nhit;
    if (nhit < MAX_HIT) { hits[nhit].x = (short)x; hits[nhit].y = (short)y; hits[nhit].w = (short)w; hits[nhit].h = (short)h; nhit++; }
    this_focus = 0;
    if (IN.mx >= x && IN.my >= y && IN.mx < x + w && IN.my < y + h && (IN.mclick || IN.mdown == 2)) focus = id;
    if (id == focus) this_focus = 1;
    if (IN.mclick && IN.mx >= x && IN.my >= y && IN.mx < x + w && IN.my < y + h) return 1;
    if (id == focus && (IN.pressed & A_OK)) return 1;
    return 0;
}
int ui_focused(void) { return this_focus; }
int ui_hover_any(void) { return nhit; }
/* Spatial navigation, so a grid or a side rail behaves as it looks */
void ui_end(void) {
    int dir = IN.pressed & (A_UP | A_DOWN | A_LEFT | A_RIGHT), i, best = -1, bs = 0x7FFFFFFF;
    nlast = nhit;
    for (i = 0; i < nhit; i++) last[i] = hits[i];
    if (focus >= nhit) focus = nhit ? nhit - 1 : 0;
    if (!dir || !nhit) return;
    {
        int cx = hits[focus].x + hits[focus].w / 2, cy = hits[focus].y + hits[focus].h / 2;
        for (i = 0; i < nhit; i++) {
            int x = hits[i].x + hits[i].w / 2, y = hits[i].y + hits[i].h / 2, dx = x - cx, dy = y - cy, s;
            if (i == focus) continue;
            if (dir & A_UP) { if (dy >= 0) continue; s = -dy * 2 + (dx < 0 ? -dx : dx) * 3; }
            else if (dir & A_DOWN) { if (dy <= 0) continue; s = dy * 2 + (dx < 0 ? -dx : dx) * 3; }
            else if (dir & A_LEFT) { if (dx >= 0) continue; s = -dx * 2 + (dy < 0 ? -dy : dy) * 3; }
            else { if (dx <= 0) continue; s = dx * 2 + (dy < 0 ? -dy : dy) * 3; }
            if (s < bs) { bs = s; best = i; }
        }
        if (best >= 0) { focus = best; plat_sound(0); }
    }
}
