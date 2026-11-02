# YomiGo 読みGo

A Japanese-learning system for one family, developed by **SENKOSOFT**. Study from your first greeting to N5 through daily classes, flashcards, conversations, and boss exams, on four kinds of device.

| App | For | Look |
|---|---|---|
| **YomiGo Desktop** | Windows | Late-90s Japanese CD-ROM software with bitmap Japanese type |
| **YomiGo HTPC** | TV and controller | Original console-style menus at 320×240, with CRT mode |
| **YomiGo Mobile** | Android | Modern, sakura-themed, with speech input |
| **YomiGo Reader** | Chrome and Firefox | Select Japanese on any page to look it up and save it |

## Getting it

Download from [Releases](https://github.com/knivesbatz/YomiGO/releases/latest), then follow the [install guide](docs/INSTALL.md).

## What works today

- **The whole beginner path:** 47 classes across 7 stages: sounds, romaji, hiragana, katakana, grammar, 80 kanji, and everyday situations.
- **A daily class:** warm-up, lesson, drill, conversation, quiz, homework with a check, and an immersion task.
- **Exams that gate progress:** an entrance exam, then 7 trials ending in the N5 Mastery Exam. Each needs 80%, with unlimited retakes.
- **Spaced repetition:** every taught item goes into a flashcard deck, and classes keep bringing old material back.
- **Scripted conversations:** 16 scenarios you must complete to move on.
- **RPG progress:** XP, ranks, streaks, achievements, and exams fought as boss duels.
- **Your pace:** 1, 2, or 3 classes a day.
- **Slow immersion:** labels shift into Japanese as you learn to read them.
- **Genki practice (optional):** review sorted by Genki lesson, using YomiGo's own cards.
- **Library:** media picks released before 1999 or after 2020, curated links, and your own Material folder.

Everything N4 and above, AI conversation, OCR, and audio transcription is still to come. See the [roadmap](docs/ROADMAP.md), [checklist](docs/CHECKLIST.md), and [known issues](docs/KNOWN_ISSUES.md).

YomiGo's exams are its own, and are not official JLPT tests.

## Building

Needs Visual Studio with C++ tools, Python 3 with Pillow, and Node 18 or later.

```bat
build.bat
python tests\ui_walk.py
node --test tests\test_core.mjs
```

Android builds in CI, or locally from `mobile/` with Gradle and the Android SDK.

## How it's put together

`content/` holds the course as plain text. `tools/build_content.py` fact-checks it and generates the data every app uses. Windows apps share C code with no runtime dependencies, and Mobile and Reader share JavaScript. Both cores pass the same test vectors. More in the [decision log](docs/DECISIONS.md).

## Private material

Study materials you own stay on your own PC, and are never committed here. YomiGo only opens them from your local folder. See [NOTICE](NOTICE.md).
