// YomiGo Setup: installs YomiGo for this Windows user, with no admin rights, the way the macOS Installer looks.
// The same program installs the desktop app (YomiGo-Setup.exe) or the TV app (YomiGo-TV-Setup.exe), chosen by its
// file name, and a copy of it left beside the app uninstalls. Both apps share one folder, so they share saves.
#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]
use std::os::windows::process::CommandExt;
use std::path::{Path, PathBuf};
use std::process::Command;
use std::sync::OnceLock;
use std::fs;

const NO_WINDOW: u32 = 0x0800_0000;
// The app and its helpers ride inside the installer; build.bat builds them first
const APP: &[u8] = include_bytes!("../../dist/YomiGo.exe");
const HELPERS: &[(&str, &[u8])] = &[
    ("YomiGoUpdate.exe", include_bytes!("../../dist/YomiGoUpdate.exe")),
    ("YomiGoOCR.exe", include_bytes!("../../dist/YomiGoOCR.exe")),
    ("YomiGoListen.exe", include_bytes!("../../dist/YomiGoListen.exe")),
    ("YomiGoTalk.exe", include_bytes!("../../dist/YomiGoTalk.exe")),
];

struct Mode { tv: bool, uninstall: bool, me: PathBuf }
static MODE: OnceLock<Mode> = OnceLock::new();
fn mode() -> &'static Mode { MODE.get().expect("mode is set at start") }
fn exe_name(tv: bool) -> &'static str { if tv { "YomiGoHTPC.exe" } else { "YomiGo.exe" } }
fn product(tv: bool) -> &'static str { if tv { "YomiGo TV" } else { "YomiGo" } }
fn reg_key(tv: bool) -> String { format!("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{}", if tv { "YomiGoTV" } else { "YomiGo" }) }
fn env_dir(k: &str) -> PathBuf { std::env::var_os(k).map(PathBuf::from).unwrap_or_default() }
fn default_dir() -> PathBuf { env_dir("LOCALAPPDATA").join("Programs").join("YomiGo") }
fn start_menu() -> PathBuf { env_dir("APPDATA").join("Microsoft\\Windows\\Start Menu\\Programs") }
fn hidden(cmd: &str) -> Command { let mut c = Command::new(cmd); c.creation_flags(NO_WINDOW); c }

#[derive(serde::Serialize)]
struct Info { tv: bool, uninstall: bool, name: &'static str, version: &'static str, dir: String, size_mb: u32 }
#[tauri::command]
fn info() -> Info {
    let m = mode();
    let dir = if m.uninstall { m.me.parent().map(Path::to_path_buf).unwrap_or_default() } else { default_dir() };
    let size = APP.len() + HELPERS.iter().map(|h| h.1.len()).sum::<usize>();
    Info { tv: m.tv, uninstall: m.uninstall, name: product(m.tv), version: env!("CARGO_PKG_VERSION"), dir: dir.display().to_string(), size_mb: (size / 1_000_000) as u32 }
}

/// Writes beside the target then swaps, so a half-written program is never left behind
fn put(path: &Path, bytes: &[u8]) -> std::io::Result<()> {
    let tmp = path.with_extension("new");
    fs::write(&tmp, bytes)?;
    if path.exists() { fs::remove_file(path)?; }
    fs::rename(&tmp, path)
}
/// A .lnk through Windows' own shortcut maker. Paths go in single-quoted PowerShell strings, quotes doubled.
fn shortcut(lnk: &Path, target: &Path, args: &str) -> Result<(), String> {
    let q = |p: &Path| p.display().to_string().replace('\'', "''");
    let ps = format!("$s=(New-Object -ComObject WScript.Shell).CreateShortcut('{}');$s.TargetPath='{}';$s.Arguments='{}';$s.WorkingDirectory='{}';$s.IconLocation='{},0';$s.Save()",
        q(lnk), q(target), args.replace('\'', "''"), q(target.parent().unwrap()), q(target));
    let ok = hidden("powershell").args(["-NoProfile", "-NonInteractive", "-Command", &ps]).status().map(|s| s.success()).unwrap_or(false);
    if ok { Ok(()) } else { Err(format!("Couldn't make the shortcut {}", lnk.display())) }
}
fn desktop() -> PathBuf {
    // The desktop can live in OneDrive, so ask Windows where it is
    hidden("powershell").args(["-NoProfile", "-NonInteractive", "-Command", "[Environment]::GetFolderPath('Desktop')"]).output().ok()
        .map(|o| PathBuf::from(String::from_utf8_lossy(&o.stdout).trim())).filter(|p| p.is_dir()).unwrap_or_else(|| env_dir("USERPROFILE").join("Desktop"))
}
fn reg(args: &[&str]) -> bool { hidden("reg").args(args).status().map(|s| s.success()).unwrap_or(false) }

#[tauri::command]
async fn install(dir: String, desktop_icon: bool) -> Result<(), String> {
    tauri::async_runtime::spawn_blocking(move || {
        let tv = mode().tv;
        let dir = PathBuf::from(dir.trim());
        if !dir.is_absolute() { return Err("Choose a full folder path, such as C:\\Users\\you\\YomiGo.".into()); }
        fs::create_dir_all(dir.join("saves")).map_err(|e| format!("Couldn't make the folder: {e}"))?;
        let exe = dir.join(exe_name(tv));
        put(&exe, APP).map_err(|_| format!("Couldn't write {}. If {} is open, close it, then try again.", exe_name(tv), product(tv)))?;
        for (name, bytes) in HELPERS { put(&dir.join(name), bytes).map_err(|e| format!("Couldn't write {name}: {e}"))?; }
        // the uninstaller is this program, beside the app
        let un = dir.join(format!("Uninstall {}.exe", product(tv)));
        if fs::read(&mode().me).ok().and_then(|me| put(&un, &me).ok()).is_none() { return Err("Couldn't write the uninstaller.".into()); }
        shortcut(&start_menu().join(format!("{}.lnk", product(tv))), &exe, "")?;
        if desktop_icon { shortcut(&desktop().join(format!("{}.lnk", product(tv))), &exe, "")?; }
        let key = reg_key(tv);
        let size_kb = ((APP.len() + HELPERS.iter().map(|h| h.1.len()).sum::<usize>()) / 1024).to_string();
        let uninstall = format!("\"{}\" --uninstall", un.display());
        for (name, kind, value) in [("DisplayName", "REG_SZ", product(tv).to_string()), ("DisplayVersion", "REG_SZ", env!("CARGO_PKG_VERSION").to_string()),
            ("Publisher", "REG_SZ", "knivesbatz".into()), ("DisplayIcon", "REG_SZ", exe.display().to_string()), ("InstallLocation", "REG_SZ", dir.display().to_string()),
            ("UninstallString", "REG_SZ", uninstall), ("URLInfoAbout", "REG_SZ", "https://github.com/knivesbatz/YomiGO".into()),
            ("NoModify", "REG_DWORD", "1".into()), ("NoRepair", "REG_DWORD", "1".into()), ("EstimatedSize", "REG_DWORD", size_kb)] {
            if !reg(&["add", &key, "/v", name, "/t", kind, "/d", &value, "/f"]) { return Err("Couldn't add YomiGo to Settings > Apps.".into()); }
        }
        Ok(())
    }).await.map_err(|e| e.to_string())?
}

#[tauri::command]
fn launch(dir: String) -> Result<(), String> {
    let exe = PathBuf::from(dir).join(exe_name(mode().tv));
    Command::new(&exe).current_dir(exe.parent().unwrap()).spawn().map(|_| ()).map_err(|e| e.to_string())
}

/// Takes this app away. Files both apps use stay while the other one is still installed. Saves stay unless asked.
#[tauri::command]
async fn uninstall(keep_saves: bool) -> Result<(), String> {
    tauri::async_runtime::spawn_blocking(move || {
        let m = mode();
        let tv = m.tv;
        let dir = m.me.parent().ok_or("No folder")?.to_path_buf();
        let exe = dir.join(exe_name(tv));
        if exe.exists() && fs::remove_file(&exe).is_err() { return Err(format!("{} is still open. Close it, then try again.", product(tv))); }
        let _ = fs::remove_file(start_menu().join(format!("{}.lnk", product(tv))));
        let _ = fs::remove_file(desktop().join(format!("{}.lnk", product(tv))));
        reg(&["delete", &reg_key(tv), "/f"]);
        if !dir.join(exe_name(!tv)).exists() {
            for (name, _) in HELPERS { let _ = fs::remove_file(dir.join(name)); }
            for name in ["YomiGo.exe.old", "YomiGoHTPC.exe.old", "YomiGoUpdate.exe.old"] { let _ = fs::remove_file(dir.join(name)); }
            let _ = fs::remove_dir_all(dir.join("models"));
            if !keep_saves {
                // into the Recycle Bin, not gone for good
                let q = dir.join("saves").display().to_string().replace('\'', "''");
                let ps = format!("Add-Type -AssemblyName Microsoft.VisualBasic;[Microsoft.VisualBasic.FileIO.FileSystem]::DeleteDirectory('{q}','OnlyErrorDialogs','SendToRecycleBin')");
                let _ = hidden("powershell").args(["-NoProfile", "-NonInteractive", "-Command", &ps]).status();
            }
        }
        // This uninstaller is running, so it removes itself, and the folder if that's all that's left, once it has closed
        let me = m.me.display().to_string();
        let _ = hidden("cmd").raw_arg(format!("/c ping 127.0.0.1 -n 3 >nul & del /f /q \"{me}\" & rmdir \"{}\"", dir.display())).spawn();
        Ok(())
    }).await.map_err(|e| e.to_string())?
}

#[tauri::command]
fn launch_url() { let _ = Command::new("explorer.exe").arg("https://github.com/knivesbatz/YomiGO/blob/main/LICENSE").spawn(); }

fn main() {
    let me = std::env::current_exe().expect("Setup can't find itself");
    let name = me.file_name().unwrap().to_string_lossy().to_lowercase();
    let args: Vec<String> = std::env::args().collect();
    let _ = MODE.set(Mode { tv: name.contains("tv"), uninstall: args.iter().any(|a| a == "--uninstall"), me });
    // --silent [folder] installs with no window (no desktop shortcut); with --uninstall it removes, keeping saves
    if let Some(i) = args.iter().position(|a| a == "--silent") {
        let r = if mode().uninstall { tauri::async_runtime::block_on(uninstall(true)) }
            else { tauri::async_runtime::block_on(install(args.get(i + 1).cloned().unwrap_or_else(|| default_dir().display().to_string()), false)) };
        std::process::exit(if r.is_ok() { 0 } else { 1 });
    }
    tauri::Builder::default()
        .setup(|app| {
            // YG_SETUP_EVAL (a script file) is for tests: the window opens off screen, out of the taskbar, and runs it
            use tauri::Manager;
            let w = app.get_webview_window("setup").expect("setup window");
            if let Some(js) = std::env::var_os("YG_SETUP_EVAL").and_then(|f| fs::read_to_string(f).ok()) {
                let _ = w.set_position(tauri::PhysicalPosition::new(-6000, -6000)); let _ = w.set_skip_taskbar(true); let _ = w.show();
                std::thread::spawn(move || { std::thread::sleep(std::time::Duration::from_millis(1500)); let _ = w.eval(&js); });
            } else { let _ = w.show(); let _ = w.set_focus(); }
            Ok(())
        })
        .invoke_handler(tauri::generate_handler![info, install, launch, launch_url, uninstall])
        .run(tauri::generate_context!())
        .expect("YomiGo Setup couldn't start");
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn put_replaces_a_file_whole() {
        let d = std::env::temp_dir().join(format!("yg-setup-{}", std::process::id()));
        fs::create_dir_all(&d).unwrap();
        let p = d.join("a.exe");
        put(&p, b"one").unwrap(); put(&p, b"two").unwrap();
        assert_eq!(fs::read(&p).unwrap(), b"two");
        assert!(!p.with_extension("new").exists());
        fs::remove_dir_all(&d).unwrap();
    }
    #[test]
    fn names_follow_the_app() {
        assert_eq!((exe_name(false), product(false)), ("YomiGo.exe", "YomiGo"));
        assert_eq!((exe_name(true), product(true)), ("YomiGoHTPC.exe", "YomiGo TV"));
        assert!(reg_key(true).ends_with("\\YomiGoTV"));
    }
}
