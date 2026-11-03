/* Software-drawn UI: one framebuffer, one bitmap font, immediate-mode widgets. */
#ifndef UI_H
#define UI_H
#ifdef YG_HTPC
#define FBW 320
#define FBH 240
#else
#define FBW 640
#define FBH 480
#endif
extern unsigned FB[FBW * FBH];

enum { A_UP = 1, A_DOWN = 2, A_LEFT = 4, A_RIGHT = 8, A_OK = 16, A_BACK = 32, A_X = 64, A_Y = 128, A_L = 256, A_R = 512, A_START = 1024 };
typedef struct {
    int pressed;              /* actions pressed this frame */
    int mx, my, mdown, mclick;
    char text[256];           /* typed text, UTF-8 */
    int backspace;
    int num;                  /* 1-4 pressed, 0 otherwise */
    int paste, copy;          /* Ctrl+V and Ctrl+C */
} Input;
extern Input IN;

void fill(int x, int y, int w, int h, unsigned c);
void frame(int x, int y, int w, int h, unsigned c);
void gradient(int x, int y, int w, int h, unsigned top, unsigned bottom);
void bevel(int x, int y, int w, int h, unsigned hi, unsigned face, unsigned lo);
void bitmap(int x, int y, const char *const *rows, int n, const unsigned *pal, int scale);
void washi(int x, int y, int w, int h, unsigned base, unsigned fleck);
int  text(int x, int y, const char *s, unsigned c);
int  text2(int x, int y, const char *s, unsigned c);
int  text_w(const char *s);
void text_c(int cx, int y, const char *s, unsigned c);
int  wrap(int x, int y, int w, const char *s, unsigned c, int lh, int skip, int maxlines);
void line(int x0, int y0, int x1, int y1, unsigned c, int thick);
void tri3d(int x0, int y0, int x1, int y1, int x2, int y2, unsigned c);
int  isin(int a);  /* a in 0..255 per turn, returns -256..256 */
int  icos(int a);

void ui_begin(void);
void ui_end(void);
void ui_reset_focus(void);
void ui_set_focus(int id);
int  ui_hit(int x, int y, int w, int h);   /* registers a focusable area; 1 when activated */
int  ui_focused(void);                     /* the area just registered has focus */
int  ui_hover_any(void);

/* Windows layer */
int  plat_today(void);
int  plat_ticks(void);
int  plat_read(const char *name, char *buf, int cap);
int  plat_write(const char *name, const char *buf, int len);
int  plat_list_saves(char names[][32], int max);
int  plat_open(const char *utf8);
int  plat_clip_get(char *utf8, int cap);
void plat_clip_set(const char *utf8);
int  plat_clip_seq(void);                      /* changes whenever the clipboard does */
int  plat_clip_image(const char *name);        /* saves a copied picture as a BMP; 0 if there isn't one */
int  plat_ocr_start(const char *in, const char *out);
int  plat_ocr_poll(void);                      /* -1 nothing running, -2 still reading, else the reader's exit code */
void plat_topmost(int on);
const char *plat_ime_comp(void);              /* what the Japanese keyboard is still composing */
void plat_ime_field(int on, int x, int y);    /* a text box is showing, and where */
int  plat_has_tts(void);
void plat_speak(const char *utf8);
void plat_sound(int kind);
void plat_quit(void);
void plat_update_check(int quiet);
int  plat_can_update(void);
int  plat_scan_material(void);
const char *plat_material(int i);
const char *plat_material_full(int i);
void plat_copy_backup(const char *name);
const char *plat_exe_dir(void);
/* Controllers */
int  pad_connected(void);
int  pad_style(void);        /* 0 keyboard, 1 Xbox, 2 PlayStation, 3 generic, 4 N64 */
int  pad_mode(void);         /* prompts and layout: 0 auto, 1 N64, 2 Xbox, 3 PlayStation */
void pad_set_mode(int m);
void pad_begin_bind(int action);
int  pad_binding(void);
int  pad_bound(int action);
void pad_set_deadzone(int d);
int  pad_deadzone(void);
void pad_save(void);
int  crt_mode(void);
void crt_set(int on);

void plat_test_next_day(void);
void app_test_auto(void);
int  app_test_done(void);
void app_init(void);
void app_frame(void);
#endif
