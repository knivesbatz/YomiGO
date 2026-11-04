# Compatibility

| Platform | Status | What was actually checked |
|---|---|---|
| Windows 11 | Tested | The app opened off screen at 640×480 and 125% scaling, in light and dark, PC and TV layouts, and its screens were captured and checked. Rust tests cover saves, names and sync requests |
| Windows 10 | Expected to work | Same program, which needs the Edge WebView2 runtime that Windows 10 has. Not run on 10 |
| Windows 8.1 and older | Not supported since 0.21 | Use 0.20 from the releases page |
| Android 8 and later | Unproven until a device test | UI tested in a browser at phone width. CI builds the APK. Speech and updates need a real phone |
| Chrome, Edge | Unproven until installed | Reader's logic tested on a page with stand-in storage. Not yet loaded as an extension |
| Firefox 115 and later | Temporary install only | As Chrome, and needs Mozilla signing to stay installed |
| HDTV | Expected to work | The stretched output is simple, but hasn't been viewed on a TV |
| CRT | Unproven | Margins and white limit are in place. Not seen on a tube. RF needs external hardware |
