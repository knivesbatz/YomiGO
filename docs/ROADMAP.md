# Roadmap

## The goal

Make YomiGo the perfect all-in-one tool for becoming fluent in Japanese, so a learner can go from zero to fluent without needing to leave it, except to use real Japanese and talk with real people.

That's a direction, not something any single update can claim. YomiGo gets there **one complete update at a time**: each one is built, tested, and released before the next begins.

To call it done, YomiGo would need:

- A full course: N3, N2, N1, and Beyond N1
- Listening at every level, with real audio
- Speaking practice, and open conversation
- Reading from real Japanese, including manga and games
- Your own flashcards, mined from what you read and watch
- Handwriting practice
- Courses that adapt to each learner's weak spots
- Real testing with real learners, native-speaker review of the course, and the hardware it promises to support

No app alone can make someone fluent. Fluency also takes thousands of hours of real Japanese and real conversation. YomiGo's job is to get you there, and to keep you going.

## Next up

1. **Next:** more words again, toward about 3,700 for N3. The course has 2,607 word cards, up from 443 in 0.10
2. **After that:** faster transcription on the graphics card, transcription on Android, vertical manga text, direct game-text hooking, N2, N1, Beyond N1, and handwriting

## Done

- **0.40:** 50 more N3 words
- **0.39:** 75 more N3 words
- **0.38:** 78 more N3 words
- **0.37:** 62 more N3 words
- **0.36:** 66 more N3 words
- **0.35:** 58 more N3 words
- **0.34:** 100 N3 words
- **0.33:** every N4 word
- **0.32:** 103 more N4 words, past 2,000 word cards
- **0.31:** 77 N4 words the course had missed
- **0.30:** every N5 word, including この, その, あの, and どの
- **0.29:** dates of the month, counting to ten, and 82 more words
- **0.28:** the entrance exam can start you halfway through N4 or N3, and 52 more words
- **0.27:** 61 more words
- **0.26:** listening in the entrance exam
- **0.25:** 86 more words, and Find new words knows the verbs you've learned in every form
- **0.24:** the twelve months, and 75 more words
- **0.23:** Windows installers, Find new words in any text, entrance exam up to N3
- **0.22:** YomiGo for Mac
- **0.21:** YomiGo for Windows rebuilt in Rust, with the macOS Big Sur look on the PC and phone, and a console-style TV app
- **0.20:** long classes split into parts of at most 25 new cards
- **0.19:** every N5 and N4 kanji, with stroke order and memory hooks
- **0.18:** sync over home Wi-Fi, and a tidier phone app
- **0.17:** effects for right answers, and exams you can't look things up in
- **0.16:** 196 more words, including skipped N5 basics; romaji in the dictionary
- **0.15:** 215 more words, the Study Guide in place of links, open licences
- **0.14:** 165 more words
- **0.13:** 119 more words, chimes, failed-exam practice, automatic updates
- **0.12:** 117 more words with example sentences
- **0.11:** 166 high-frequency words with example sentences, set phrases, and casual reactions
- **0.10:** Genki II practice, lessons 13 to 23
- **0.9:** a conversation partner that runs on your PC
- **0.8:** N3 stage, with its exam
- **0.7:** transcription of your own audio and video, with Japanese subtitles
- **0.6:** Text Reader, with screenshots, photos, and game text from the clipboard
- **0.5:** listening, dictation, and shadowing
- **0.4:** your own flashcards
- **0.3:** N4 stage, graded reading
- **0.2:** dictionary, stroke order, N64 pads, Study Guide
- **0.1:** base build through N5, all four apps

## Details

## 0.1 Base build (now)

Shared course through N5, all four apps, updates, tests, and CI.

## 0.2 Real dictionary and writing (done, except these)

- Handwriting practice on screen, and furigana on web pages
- Add listening, typing, and speaking to the entrance exam

## 0.3 OCR and mining

Owner's request. Flashcards made from what's on screen:

- OCR for flashcard creation, studying [GameSentenceMiner](https://github.com/bpwhelan/GameSentenceMiner) and [anki_miner](https://github.com/0xzerolight/anki_miner). Both are GPL-3.0, so YomiGo takes ideas only unless the owner chooses a compatible licence
- Manga reading based on the [Mokuro](https://github.com/kha-white/mokuro) workflow
- Game text capture, based on ideas from [LunaTranslator](https://github.com/HIllya51/LunaTranslator)
- User-created cards, and direct Anki export

## 0.4 Audio

- Local transcription with Whisper or a similar model: no uploads, and correctable transcripts
- Dictation and shadowing exercises
- Vocabulary pulled from transcripts of your own files

## 0.5 N3

N3 vocabulary, grammar, kanji, and mastery exam: done in 0.8. Genki II practice: done in 0.10.

## 0.6 Conversation AI

Open conversation that remembers the lesson's goals, corrects mistakes, and tracks weak spots. Needs a local model, or a paid service the owner approves first. Speech input and scripted conversations remain as the offline path.

## Later

- N2, N1, and YomiGo's own Beyond N1 stage
- Keigo, regional speech, and more on gestures and expressions
- FSRS scheduling, weekly tournaments, skill trees
- Automatic sync between devices
- Android-TV-friendly layouts, and more CRT profiles
- Firefox signing, and browser store listings
