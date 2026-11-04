# Licences and attribution

YomiGo is © knivesbatz, and free to use, share, and change:

- **Source code:** GNU General Public License, version 3 or any later version. See [`LICENSE`](LICENSE).
- **Course content** (everything in `content/` except `content/dict/`, which keeps its own licences below): Creative Commons Attribution-ShareAlike 4.0. See [`content/LICENSE`](content/LICENSE).

These were chosen so YomiGo can take in as much open knowledge as possible. Most open Japanese-learning tools are GPL-3.0, and the big open Japanese datasets (JMdict, KANJIDIC2, KanjiVG, Wiktionary, Tatoeba, open pitch-accent data) are CC BY-SA or compatible with it. Material that's copyrighted, such as textbooks and paid courses, still never goes in, whatever YomiGo's own licence.

| Part | Licence | Notes |
|---|---|---|
| DotGothic16 font, by Fontworks | SIL OFL 1.1 | The Classic look's type. See `mobile/app/src/main/assets/www/fonts/DotGothic16-OFL.txt` |
| Inter font, by Rasmus Andersson | SIL OFL 1.1 | The interface type. See `mobile/app/src/main/assets/www/fonts/Inter-OFL.txt` |
| Tauri and its Rust crates | MIT or Apache-2.0 | The Windows and Mac apps' window and web view. Each crate's licence is in its source on crates.io |
| Source code | GPL-3.0-or-later | `src/`, `desktop/`, `shared/`, `mobile/`, `extension/`, `tools/`, `tests/` |
| Course text, memory hooks, conversations | CC BY-SA 4.0 | Written for YomiGo. Facts such as readings and meanings aren't owned by anyone, but the wording here is |

YomiGo isn't affiliated with or endorsed by Apple, Nintendo, The Japan Times (Genki), the JLPT organisers, Duolingo, WaniKani, Anki, Quizlet, or any other product it mentions. Names are used only to describe compatibility or inspiration.

## Text recognition

- **Windows:** the text reader uses the OCR built into Windows 10 and 11. No model ships with YomiGo.
- **Android:** reading photos uses Google's [ML Kit text recognition](https://developers.google.com/ml-kit/vision/text-recognition/android), bundled in the app, under [ML Kit's terms](https://developers.google.com/ml-kit/terms). It runs on the phone.

## Transcription

- **Engine:** [whisper.cpp](https://github.com/ggml-org/whisper.cpp) by Georgi Gerganov and contributors, MIT licence, built into `YomiGoListen.exe` from the pinned copy in `third_party/whisper.cpp`.
- **Model:** OpenAI's [Whisper](https://github.com/openai/whisper) large-v3-turbo, MIT licence, in the [ggml version](https://huggingface.co/ggerganov/whisper.cpp) hosted on Hugging Face. It's downloaded on first use, not shipped with YomiGo.

## Conversation

- **Engine:** [llama.cpp](https://github.com/ggml-org/llama.cpp) by Georgi Gerganov and contributors, MIT licence, built into `YomiGoTalk.exe` from the pinned copy in `third_party/llama.cpp`.
- **Model:** Google's [Gemma 4 E4B](https://huggingface.co/google/gemma-4-E4B-it-qat-q4_0-gguf), Apache 2.0, downloaded on first use, not shipped with YomiGo. Sakura is a made-up character, not a real person.

## Dictionary and stroke data

`content/dict/` keeps its own licences, and they differ from the rest of the project:

- **`jmdict_common.tsv`** is cut from [JMdict](https://www.edrdg.org/wiki/index.php/JMdict-EDICT_Dictionary_Project). This publication has included material from the JMdict (EDICT, etc.) dictionary files in accordance with the licence provisions of the Electronic Dictionaries Research Group. See the [EDRDG licence](https://www.edrdg.org/edrdg/licence.html). CC BY-SA 4.0.
- **`strokes.tsv`** is simplified from [KanjiVG](https://kanjivg.tagaini.net/), by Ulrich Apel and contributors. CC BY-SA 3.0.

The Study Guide and course tips are written in YomiGo's own words after reading the guide sites listed in [the bibliography](docs/BIBLIOGRAPHY.md). They contain no text copied from those sites.

## Study material

Commercial courses and books never go in this repository, and the course contains no text, audio, or exercises copied from them. The apps open a family's own files from a local folder, and don't unlock, decrypt, or redistribute anything.

Genki practice groups YomiGo's own cards by the lesson that covers those topics. It contains no Genki vocabulary lists, dialogues, or exercises.
