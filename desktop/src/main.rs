// YomiGo for Windows and Mac: the shared web UI in a Tauri window. Rust does what a web page can't: saves, the clipboard,
// the Windows helper programs (OCR, transcriber, conversation partner, updater), the Material folder and Wi-Fi sync.
#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]
use std::io::{Read, Write};
use std::net::{TcpListener, TcpStream, UdpSocket};
use std::path::{Path, PathBuf};
use std::process::{Child, Command};
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{mpsc, Arc, Mutex, OnceLock};
use std::time::Duration;
use std::{fs, thread};
use tauri::{Emitter, Manager};

#[cfg(windows)]
const NO_WINDOW: u32 = 0x0800_0000;
const SPEECH_MODEL: &str = "models\\ggml-large-v3-turbo-q5_0.bin";
const TALK_MODEL: &str = "models\\gemma-4-E4B_q4_0-it.gguf";
const SYNC_PORT: u16 = 47820;

struct Paths { dir: PathBuf, root: PathBuf, exe: String, tv: bool, test: bool }
static PATHS: OnceLock<Paths> = OnceLock::new();
fn paths() -> &'static Paths { PATHS.get().expect("paths are set at start") }

/// Learner names become file names, so only these characters are allowed (as the app itself enforces)
fn safe(name: &str) -> Result<&str, String> {
    let ok = !name.is_empty() && name.len() <= 31 && name.chars().all(|c| c.is_ascii_alphanumeric() || c == '-' || c == '_');
    if ok { Ok(name) } else { Err("That name can't be a file name.".into()) }
}

#[derive(serde::Serialize)]
struct Host { version: &'static str, os: &'static str, tv: bool, dir: String, speech_model: bool, talk_model: bool }

#[tauri::command]
fn host() -> Host {
    let p = paths();
    Host { version: env!("CARGO_PKG_VERSION"), os: if cfg!(target_os = "macos") { "macos" } else { "windows" }, tv: p.tv, dir: p.dir.display().to_string(),
        speech_model: p.dir.join(SPEECH_MODEL).exists(), talk_model: p.dir.join(TALK_MODEL).exists() }
}

/// Every learner's save, as (name, text)
#[tauri::command]
fn saves() -> Vec<(String, String)> {
    let mut out = vec![];
    if let Ok(rd) = fs::read_dir(&paths().root) {
        for e in rd.flatten() {
            let path = e.path();
            let (Some(stem), Some("sav")) = (path.file_stem().and_then(|s| s.to_str()), path.extension().and_then(|s| s.to_str())) else { continue };
            if safe(stem).is_ok() { if let Ok(b) = fs::read(&path) { out.push((stem.to_string(), String::from_utf8_lossy(&b).into_owned())); } }
        }
    }
    out.sort();
    out
}

/// Write beside the file, then swap, keeping one old copy: a crash mid-save can't lose progress
fn write_safe(p: &Path, text: &str) -> std::io::Result<()> {
    let tmp = p.with_extension("sav.tmp");
    let bak = p.with_extension("sav.bak");
    { let mut f = fs::File::create(&tmp)?; f.write_all(text.as_bytes())?; f.sync_all()?; }
    let _ = fs::remove_file(&bak);
    let had = fs::rename(p, &bak).is_ok();
    if let Err(e) = fs::rename(&tmp, p) { if had { let _ = fs::rename(&bak, p); } return Err(e); }
    Ok(())
}

#[tauri::command]
fn save(name: String, text: String) -> Result<(), String> {
    if !text.starts_with("YOMIGO-SAVE 1\n") { return Err("Not a YomiGo save.".into()); }
    write_safe(&paths().root.join(format!("{}.sav", safe(&name)?)), &text).map_err(|e| e.to_string())
}

/// A dated copy in the backup folder. The page passes today's date, as YYYYMMDD.
#[tauri::command]
fn backup(name: String, stamp: String) -> Result<String, String> {
    if stamp.len() != 8 || !stamp.bytes().all(|b| b.is_ascii_digit()) { return Err("bad date".into()); }
    let root = &paths().root;
    let dir = root.join("backup");
    fs::create_dir_all(&dir).map_err(|e| e.to_string())?;
    let to = dir.join(format!("{}-{}.sav", safe(&name)?, stamp));
    fs::copy(root.join(format!("{name}.sav")), &to).map_err(|e| e.to_string())?;
    Ok(to.display().to_string())
}

// ---- clipboard ----
#[cfg(windows)]
mod clip {
    use windows_sys::Win32::System::DataExchange::*;
    use windows_sys::Win32::System::Memory::{GlobalLock, GlobalSize, GlobalUnlock};
    const CF_UNICODETEXT: u32 = 13;
    const CF_DIB: u32 = 8;
    pub fn seq() -> u32 { unsafe { GetClipboardSequenceNumber() } }
    pub fn text() -> String {
        unsafe {
            if OpenClipboard(std::ptr::null_mut()) == 0 { return String::new(); }
            let mut out = String::new();
            let h = GetClipboardData(CF_UNICODETEXT);
            if !h.is_null() {
                let p = GlobalLock(h) as *const u16;
                if !p.is_null() {
                    let mut n = 0; while *p.add(n) != 0 && n < 1 << 20 { n += 1; }
                    out = String::from_utf16_lossy(std::slice::from_raw_parts(p, n));
                    GlobalUnlock(h);
                }
            }
            CloseClipboard();
            out
        }
    }
    /// A copied picture is a DIB: add the file header, and it's a BMP the reader can open
    pub fn image_bmp() -> Option<Vec<u8>> {
        unsafe {
            if OpenClipboard(std::ptr::null_mut()) == 0 { return None; }
            let mut out = None;
            let h = GetClipboardData(CF_DIB);
            if !h.is_null() {
                let p = GlobalLock(h) as *const u8;
                if !p.is_null() {
                    let size = GlobalSize(h);
                    let dib = std::slice::from_raw_parts(p, size);
                    if size >= 40 {
                        let rd = |o: usize| u32::from_le_bytes([dib[o], dib[o + 1], dib[o + 2], dib[o + 3]]);
                        let (hsize, bits, comp, mut colors) = (rd(0), u16::from_le_bytes([dib[14], dib[15]]), rd(16), rd(32));
                        if colors == 0 && bits <= 8 { colors = 1 << bits; }
                        let masks = if comp == 3 && hsize == 40 { 12 } else { 0 };
                        let off = 14 + hsize + colors * 4 + masks;
                        let mut f = Vec::with_capacity(14 + size);
                        f.extend_from_slice(b"BM");
                        f.extend_from_slice(&((14 + size) as u32).to_le_bytes());
                        f.extend_from_slice(&[0; 4]);
                        f.extend_from_slice(&off.to_le_bytes());
                        f.extend_from_slice(dib);
                        out = Some(f);
                    }
                    GlobalUnlock(h);
                }
            }
            CloseClipboard();
            out
        }
    }
}
/// On a Mac, the clipboard through pbpaste. There are no screenshots to read there: the OCR helper is Windows only.
#[cfg(not(windows))]
mod clip {
    pub fn text() -> String { std::process::Command::new("pbpaste").output().map(|o| String::from_utf8_lossy(&o.stdout).into_owned()).unwrap_or_default() }
    /// macOS has no change counter here, so a hash of the text stands in for one
    pub fn seq() -> u32 { text().bytes().fold(2166136261u32, |h, b| (h ^ b as u32).wrapping_mul(16777619)) }
    pub fn image_bmp() -> Option<Vec<u8>> { None }
}
#[tauri::command] fn clip_seq() -> u32 { clip::seq() }
#[tauri::command] fn clip_text() -> String { clip::text() }

// ---- helper programs ----
fn helper(exe: &str) -> Command {
    let p = paths();
    let mut c = Command::new(p.dir.join(exe));
    c.current_dir(&p.dir);
    #[cfg(windows)] { use std::os::windows::process::CommandExt; c.creation_flags(NO_WINDOW); }
    c
}
fn code_of(c: std::io::Result<std::process::ExitStatus>) -> i32 { c.ok().and_then(|s| s.code()).unwrap_or(-1) }
fn read_lossy(p: &Path) -> String { fs::read(p).map(|b| String::from_utf8_lossy(&b).into_owned()).unwrap_or_default() }

#[tauri::command] fn has(name: String) -> bool { paths().dir.join(name).exists() }
/// How far a download or transcription has got, from the helper's progress file
#[tauri::command] fn pct(name: String) -> u32 {
    if !matches!(name.as_str(), "listen" | "talk") { return 0; }
    read_lossy(&paths().root.join(format!("{name}_pct.txt"))).trim().parse().unwrap_or(0)
}
#[tauri::command] fn fetch_helpers() { let _ = helper("YomiGoUpdate.exe").arg("--helpers").spawn(); }

/// Reads a screenshot on the clipboard with the OCR in Windows. Ok(text), or Err(reader's exit code; 100 = no picture, -1 = didn't start)
#[tauri::command]
async fn ocr() -> Result<String, i32> {
    tauri::async_runtime::spawn_blocking(|| {
        let root = &paths().root;
        let (inp, out) = (root.join("ocr_in.bmp"), root.join("ocr_out.txt"));
        let bmp = clip::image_bmp().ok_or(100)?;
        fs::write(&inp, bmp).map_err(|_| -1)?;
        let _ = fs::remove_file(&out);
        let code = code_of(helper("YomiGoOCR.exe").arg(&inp).arg(&out).status());
        if code != 0 { return Err(code); }
        Ok(read_lossy(&out))
    }).await.unwrap_or(Err(-1))
}

/// Transcribes a file the user picks, or with get, downloads the speech model. Ok((exit code, lines)), Err if it didn't start.
#[tauri::command]
async fn listen(get: bool) -> Result<(i32, String), String> {
    tauri::async_runtime::spawn_blocking(move || {
        let p = paths();
        let _ = fs::create_dir_all(p.dir.join("models"));
        let (out, pct) = (p.root.join("listen_out.txt"), p.root.join("listen_pct.txt"));
        let _ = fs::remove_file(&out);
        let mut c = helper("YomiGoListen.exe");
        if get { c.arg("--get"); }
        c.arg(p.dir.join(SPEECH_MODEL));
        if !get { c.arg(&out); }
        c.arg(&pct);
        let code = c.status().map_err(|e| e.to_string())?.code().unwrap_or(-1);
        Ok((code, read_lossy(&out)))
    }).await.map_err(|e| e.to_string())?
}

#[tauri::command]
async fn talk_get() -> Result<i32, String> {
    tauri::async_runtime::spawn_blocking(|| {
        let p = paths();
        let _ = fs::create_dir_all(p.dir.join("models"));
        Ok(code_of(helper("YomiGoTalk.exe").arg("--get").arg(p.dir.join(TALK_MODEL)).arg(p.root.join("talk_pct.txt")).status()))
    }).await.map_err(|e: tauri::Error| e.to_string())?
}

/// The partner stays running, holding the model and the conversation, and closes itself when YomiGo does
static PARTNER: Mutex<Option<Child>> = Mutex::new(None);
/// Asks the partner; Ok(reply), or Err(its exit code; -1 if it couldn't start)
#[tauri::command]
async fn talk(req: String) -> Result<String, i32> {
    tauri::async_runtime::spawn_blocking(move || {
        let p = paths();
        {
            let mut g = PARTNER.lock().unwrap();
            if g.as_mut().map_or(true, |c| !matches!(c.try_wait(), Ok(None))) {
                *g = Some(helper("YomiGoTalk.exe").arg(p.dir.join(TALK_MODEL)).arg(&p.root).arg(std::process::id().to_string()).spawn().map_err(|_| -1)?);
            }
        }
        let (req_p, out_p) = (p.root.join("talk_req.txt"), p.root.join("talk_out.txt"));
        let tmp = p.root.join("talk_req.tmp");
        fs::write(&tmp, req).and_then(|_| fs::rename(&tmp, &req_p)).map_err(|_| -1)?;
        loop {
            if let Ok(b) = fs::read(&out_p) { let _ = fs::remove_file(&out_p); return Ok(String::from_utf8_lossy(&b).into_owned()); }
            if let Some(c) = PARTNER.lock().unwrap().as_mut() {
                if let Ok(Some(st)) = c.try_wait() { return Err(st.code().unwrap_or(-1)); }
            }
            thread::sleep(Duration::from_millis(150));
        }
    }).await.unwrap_or(Err(-1))
}

// ---- Material folder: the owner's own books and audio, opened with their usual programs ----
fn scan(base: &Path, rel: &Path, depth: u32, out: &mut Vec<(String, String)>) {
    if depth > 5 || out.len() >= 800 { return; }
    let Ok(rd) = fs::read_dir(base.join(rel)) else { return };
    for e in rd.flatten() {
        let name = e.file_name().to_string_lossy().into_owned();
        if name.starts_with('.') { continue; }
        let r2 = rel.join(&name);
        if e.path().is_dir() { scan(base, &r2, depth + 1, out); }
        else if [".mp3", ".mp4", ".pdf", ".epub", ".doc"].iter().any(|x| name.to_lowercase().ends_with(x)) && out.len() < 800 {
            out.push((r2.display().to_string(), base.join(&r2).display().to_string()));
        }
    }
}
fn material_list() -> Vec<(String, String)> {
    let root = &paths().root;
    let mut out = vec![];
    let home = std::env::var_os("HOME").map(PathBuf::from).unwrap_or_default();
    for b in [root.join("Material"), root.join("..").join("Material"), PathBuf::from("C:\\YomiGO\\Material"), home.join("YomiGO").join("Material")] {
        scan(&b, Path::new(""), 0, &mut out);
        if !out.is_empty() { break; }
    }
    out
}
#[tauri::command] fn material() -> Vec<(String, String)> { material_list() }

/// Opens a web link, or a file from the Material folder, with its usual program. Nothing else.
#[tauri::command]
fn open(target: String) -> Result<(), String> {
    let ok = target.starts_with("https://") || material_list().iter().any(|(_, full)| *full == target);
    if !ok { return Err("Not something YomiGo opens.".into()); }
    Command::new(if cfg!(windows) { "explorer.exe" } else { "open" }).arg(&target).spawn().map(|_| ()).map_err(|e| e.to_string())
}

#[tauri::command]
fn topmost(app: tauri::AppHandle, on: bool) { if let Some(w) = app.get_webview_window("main") { let _ = w.set_always_on_top(on); } }

/// The signed updater swaps in a newer version. quiet installs without asking, then restarts YomiGo.
fn run_updater(quiet: bool) -> bool {
    let p = paths();
    if p.test || !p.dir.join("YomiGoUpdate.exe").exists() { return false; }
    Command::new(p.dir.join("YomiGoUpdate.exe")).current_dir(&p.dir).args(updater_args(&p.exe, std::process::id(), quiet)).spawn().is_ok()
}
/// What the updater is told: which program to swap, this version, and this process to close. It only accepts YomiGo.exe and YomiGoHTPC.exe.
fn updater_args(exe: &str, pid: u32, quiet: bool) -> Vec<String> {
    let mut a = vec![exe.to_string(), env!("CARGO_PKG_VERSION").to_string(), pid.to_string()];
    if quiet { a.push("--auto".into()); }
    a
}
#[tauri::command] fn update() -> bool { run_updater(false) }

// ---- home Wi-Fi sync: the phone posts its save, the page merges it with this PC's copy, and the merged save goes back ----
static SYNC_STOP: Mutex<Option<Arc<AtomicBool>>> = Mutex::new(None);
static SYNC_REPLY: Mutex<Option<mpsc::Sender<(u16, String)>>> = Mutex::new(None);

fn local_ip() -> String {
    // A UDP "connection" sends nothing; it only makes Windows pick the address the home network sees
    UdpSocket::bind("0.0.0.0:0").and_then(|s| { s.connect("192.168.255.255:9")?; s.local_addr() })
        .map(|a| a.ip().to_string()).unwrap_or_else(|_| "127.0.0.1".into())
}
fn respond(s: &mut TcpStream, status: u16, body: &str) {
    let line = match status { 200 => "200 OK", 204 => "204 No Content", 403 => "403 Forbidden", _ => "400 Bad Request" };
    let head = format!("HTTP/1.0 {line}\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Methods: POST\r\nAccess-Control-Allow-Headers: Content-Type\r\nContent-Type: text/plain; charset=utf-8\r\nConnection: close\r\nContent-Length: {}\r\n\r\n", body.len());
    let _ = s.write_all(head.as_bytes()).and_then(|_| s.write_all(body.as_bytes()));
}
/// The whole request's body, or None. A few quiet seconds and it gives up.
fn read_request(s: &mut TcpStream) -> Option<(bool, Vec<u8>)> {
    s.set_read_timeout(Some(Duration::from_secs(3))).ok()?;
    let (mut buf, mut chunk) = (Vec::new(), [0u8; 8192]);
    loop {
        let n = s.read(&mut chunk).ok()?;
        if n == 0 { return None; }
        buf.extend_from_slice(&chunk[..n]);
        if buf.len() > 4 << 20 { return None; }
        if let Some(h) = buf.windows(4).position(|w| w == b"\r\n\r\n") {
            let head = String::from_utf8_lossy(&buf[..h]).to_lowercase();
            if head.starts_with("options") { return Some((true, vec![])); }
            let need: usize = head.lines().find_map(|l| l.strip_prefix("content-length:")).and_then(|v| v.trim().parse().ok()).unwrap_or(0);
            if buf.len() - h - 4 >= need { return Some((false, buf[h + 4..].to_vec())); }
        }
    }
}
fn serve(app: &tauri::AppHandle, mut s: TcpStream) {
    let _ = s.set_nonblocking(false);
    match read_request(&mut s) {
        None => respond(&mut s, 400, ""),
        Some((true, _)) => respond(&mut s, 204, ""),
        Some((false, body)) => {
            let (tx, rx) = mpsc::channel();
            *SYNC_REPLY.lock().unwrap() = Some(tx);
            let _ = app.emit("sync", String::from_utf8_lossy(&body).into_owned());
            let (st, b) = rx.recv_timeout(Duration::from_secs(15)).unwrap_or((400, "The PC didn't answer".into()));
            respond(&mut s, st, &b);
        }
    }
}
#[tauri::command]
fn sync_open(app: tauri::AppHandle) -> Result<String, String> {
    sync_close();
    let test = paths().test;
    let l = TcpListener::bind((if test { "127.0.0.1" } else { "0.0.0.0" }, SYNC_PORT)).map_err(|e| e.to_string())?;
    l.set_nonblocking(true).map_err(|e| e.to_string())?;
    let stop = Arc::new(AtomicBool::new(false));
    *SYNC_STOP.lock().unwrap() = Some(stop.clone());
    thread::spawn(move || while !stop.load(Ordering::Relaxed) {
        match l.accept() {
            Ok((s, _)) => serve(&app, s),
            Err(e) if e.kind() == std::io::ErrorKind::WouldBlock => thread::sleep(Duration::from_millis(100)),
            Err(_) => break,
        }
    });
    Ok(if test { "127.0.0.1".into() } else { local_ip() })
}
#[tauri::command]
fn sync_reply(status: u16, body: String) { if let Some(tx) = SYNC_REPLY.lock().unwrap().take() { let _ = tx.send((status, body)); } }
#[tauri::command]
fn sync_close() { if let Some(s) = SYNC_STOP.lock().unwrap().take() { s.store(true, Ordering::Relaxed); } }

fn main() {
    let exe_path = std::env::current_exe().expect("YomiGo can't find itself");
    let dir = exe_path.parent().unwrap().to_path_buf();
    let exe = exe_path.file_name().unwrap().to_string_lossy().into_owned();
    let args: Vec<String> = std::env::args().collect();
    // --root keeps a test's saves apart from the learner's; --tv starts the TV layout under any name
    let root = args.iter().position(|a| a == "--root").and_then(|i| args.get(i + 1)).map(PathBuf::from);
    let tv = exe.to_lowercase().contains("htpc") || args.iter().any(|a| a == "--tv");
    // Saves live in a saves folder beside the program on Windows, as they always have. A Mac app can't write inside itself,
    // so there they go in Application Support.
    let default_saves = if cfg!(target_os = "macos") {
        std::env::var_os("HOME").map(PathBuf::from).unwrap_or_default().join("Library/Application Support/YomiGo/saves")
    } else { dir.join("saves") };
    let save_dir = root.clone().unwrap_or(default_saves);
    let _ = fs::create_dir_all(&save_dir);
    let _ = PATHS.set(Paths { root: save_dir, dir, exe, tv, test: root.is_some() });
    tauri::Builder::default()
        .setup(move |app| {
            let w = app.get_webview_window("main").expect("main window");
            // Big Sur windows have round corners and no coloured outline, so ask Windows 11 for the same
            #[cfg(windows)]
            if let Ok(h) = w.hwnd() { unsafe {
                use windows_sys::Win32::Graphics::Dwm::DwmSetWindowAttribute;
                let h = h.0 as windows_sys::Win32::Foundation::HWND;
                let (round, none): (u32, u32) = (2, 0xFFFF_FFFE); // DWMWCP_ROUND, DWMWA_COLOR_NONE
                DwmSetWindowAttribute(h, 33, &round as *const u32 as _, 4);
                DwmSetWindowAttribute(h, 34, &none as *const u32 as _, 4);
            } }
            if paths().test {
                // Tests run off screen and never take the focus; --eval drives the page, --size sets the window
                let arg = |k: &str| args.iter().position(|a| a == k).and_then(|i| args.get(i + 1)).cloned();
                let _ = w.set_position(tauri::PhysicalPosition::new(-6000, -6000));
                let _ = w.set_skip_taskbar(true);
                let _ = w.show();
                // Practically invisible and never activated, so tests run in the background; a screen capture still reads it
                #[cfg(windows)]
                if let Ok(h) = w.hwnd() { unsafe {
                    use windows_sys::Win32::UI::WindowsAndMessaging::*;
                    let h = h.0 as windows_sys::Win32::Foundation::HWND;
                    SetWindowLongW(h, GWL_EXSTYLE, GetWindowLongW(h, GWL_EXSTYLE) | (WS_EX_LAYERED | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT) as i32);
                    SetLayeredWindowAttributes(h, 0, 1, LWA_ALPHA);
                } }
                if let Some((x, y)) = arg("--size").and_then(|s| s.split_once('x').map(|(a, b)| (a.parse().unwrap_or(1180), b.parse().unwrap_or(760)))) {
                    let _ = w.set_min_size(None::<tauri::Size>);
                    let _ = w.set_size(tauri::PhysicalSize::new(x, y));
                }
                let _ = w.set_position(tauri::PhysicalPosition::new(-6000, -6000));
                if let Some(js) = arg("--eval").and_then(|f| fs::read_to_string(f).ok()) {
                    let w2 = w.clone();
                    thread::spawn(move || { thread::sleep(Duration::from_millis(1500)); let _ = w2.eval(&js); });
                }
            } else {
                if paths().tv { let _ = w.set_fullscreen(true); }
                let _ = w.set_focus();
                run_updater(true); // every start, so YomiGo is always the newest version
            }
            let _ = w.show();
            Ok(())
        })
        .invoke_handler(tauri::generate_handler![host, saves, save, backup, clip_seq, clip_text, has, pct, fetch_helpers, ocr, listen,
            talk_get, talk, material, open, topmost, update, sync_open, sync_reply, sync_close])
        .run(tauri::generate_context!())
        .expect("YomiGo couldn't start");
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn names_never_escape_the_folder() {
        assert!(safe("Maya_2").is_ok());
        for bad in ["", "..", "a/b", "a\\b", "C:x", "名前", "a.sav"] { assert!(safe(bad).is_err(), "{bad}"); }
    }
    #[test]
    fn saves_swap_in_keeping_a_backup() {
        let d = std::env::temp_dir().join(format!("yg-test-{}", std::process::id()));
        fs::create_dir_all(&d).unwrap();
        let p = d.join("x.sav");
        write_safe(&p, "one").unwrap();
        write_safe(&p, "two").unwrap();
        assert_eq!(fs::read_to_string(&p).unwrap(), "two");
        assert_eq!(fs::read_to_string(d.join("x.sav.bak")).unwrap(), "one");
        fs::remove_dir_all(&d).unwrap();
    }
    #[test]
    fn start_up_asks_the_updater_to_install_without_asking() {
        assert_eq!(updater_args("YomiGoHTPC.exe", 42, true), ["YomiGoHTPC.exe", env!("CARGO_PKG_VERSION"), "42", "--auto"]);
        assert_eq!(updater_args("YomiGo.exe", 7, false), ["YomiGo.exe", env!("CARGO_PKG_VERSION"), "7"]);
    }
    #[test]
    fn sync_reads_a_whole_post() {
        let l = TcpListener::bind("127.0.0.1:0").unwrap();
        let a = l.local_addr().unwrap();
        let t = thread::spawn(move || { let mut c = TcpStream::connect(a).unwrap(); c.write_all(b"POST /sync HTTP/1.1\r\nContent-Length: 5\r\n\r\nhel").unwrap(); thread::sleep(Duration::from_millis(50)); c.write_all(b"lo").unwrap(); });
        let (mut s, _) = l.accept().unwrap();
        assert_eq!(read_request(&mut s).unwrap(), (false, b"hello".to_vec()));
        t.join().unwrap();
    }
}
