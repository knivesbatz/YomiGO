# Building and contributing

YomiGo is a family project, but bug reports are welcome.

## Build it

You need Visual Studio with C++ tools, CMake, Python 3 with Pillow, and Node 18 or later. The transcriber builds whisper.cpp from a submodule, once.

```bat
git submodule update --init
build.bat
python tests\ui_walk.py
node --test tests\test_core.mjs
```

The Windows programs land in `dist`. Android builds in CI, or locally from `mobile/` with Gradle and the Android SDK. `python tools/make_brand.py` remakes the logo, banner, and screenshots.

## How it's put together

`content/` holds the course as plain text. `tools/build_content.py` checks it and generates the data every app uses. The Windows apps share C code with no runtime dependencies, and Mobile and Reader share JavaScript. Both cores pass the same test vectors. The reasons are in the [decision log](docs/DECISIONS.md), and [the checklist](docs/CHECKLIST.md) shows the state of each feature.

## Rules

- **Course corrections:** edit `content/`, then run `python tools/build_content.py`. It fails on inconsistent data. Please cite a dictionary or reference.
- **Never commit** study material, saves, keys, or anything from a paid course. Write original text.
- **Windows code** must keep passing `tools/check_pe.py`. Look up anything newer than Windows 95 at run time.
