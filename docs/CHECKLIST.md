# Feature checklist

Complete means implemented and tested as described. **Partial** says what's missing.

| Feature | Desktop | HTPC | Mobile | Reader |
|---|---|---|---|---|
| Local profiles | Complete | Complete | Complete | n/a |
| Entrance exam, adaptive by section | Partial: reading only | Partial: reading only | Partial: reading only | n/a |
| Stages 0-7, up to N4 | Complete | Complete | Complete | n/a |
| Graded reading with questions | Complete | Complete | Complete | n/a |
| N3, N2, N1, Beyond N1 | Not started | Not started | Not started | n/a |
| Daily class with up to 9 steps | Complete | Complete | Complete | n/a |
| Romaji, hiragana, katakana drills | Complete | Complete | Complete | n/a |
| Stroke order | Complete | Complete | Complete | n/a |
| Handwriting practice | Not started | Not started | Not started | n/a |
| Vocabulary, grammar, 144 kanji | Complete | Complete | Complete | n/a |
| Spaced repetition | Complete | Complete | Complete | n/a |
| Your own cards | Complete | Complete | Complete | Saved words, copied into YomiGo |
| Exams gating progress | Complete | Complete | Complete | n/a |
| Conversations | Partial: scripted | Partial: scripted | Partial: scripted | n/a |
| Open AI conversation | Not started | Not started | Not started | n/a |
| XP, ranks, streaks, boss exams | Complete | Complete | Partial: no achievements list | n/a |
| Weekly tournaments, skill trees | Not started | Not started | Not started | n/a |
| Homework with a check | Complete | Complete | Complete | n/a |
| Progressive Japanese labels | Partial: navigation and buttons | Partial | Partial | n/a |
| Media picks and links | Complete | Complete | Complete | n/a |
| Own Material folder | Complete | Complete | Not started | n/a |
| Genki practice setting | Complete | Complete | Complete | n/a |
| Text-to-speech | Partial: needs a Japanese voice; not yet heard | Partial: as Desktop | Partial: not run on a device | n/a |
| Speech input | n/a | n/a | Partial: not run on a device | n/a |
| Controller, prompts, rebinding | n/a | Partial: no real pad tested | n/a | n/a |
| CRT mode | n/a | Partial: no real CRT tested | n/a | n/a |
| Updates | Complete on Windows 7+ | Complete on Windows 7+ | Partial: not run on a device | Manual |
| Backup and restore | Partial: backup copies; restore by hand | Partial: as Desktop | Complete: text export and import | n/a |
| Dictionary | Complete | Complete | Complete | Complete, not yet in a real browser install |
| Sentence capture, Anki export | n/a | n/a | n/a | Complete, not yet in a real browser install |
| Furigana on pages | n/a | n/a | n/a | Not started |
| OCR, manga reading, game text | Not started | Not started | Not started | n/a |
| Audio transcription | Not started | Not started | Not started | n/a |
| Automatic sync | Not started | Not started | Not started | Not started |

## How things are tested

- `tests/test_core.c` and `tests/test_core.mjs`: scheduling, kana conversion, damaged saves, every question, and every conversation reply.
- `tests/ui_walk.py`: plays all 47 classes, all exams, and all conversations through the real screens of both Windows builds.
- Mobile's UI was walked through the whole course in a browser at phone width.
- `tools/check_pe.py`: fails the build if a Windows program imports something Windows 95 lacks.
