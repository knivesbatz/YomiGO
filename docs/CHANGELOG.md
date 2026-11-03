# Changelog

## 0.9.0

- Talk: chat freely in Japanese with Sakura, a conversation partner that runs on your PC. She follows your level and today's lesson, and each reply comes with an English translation and a check of what you wrote. On the PC and the TV, from the Grammar Gate on
- Her model, Google's Gemma 4 E4B (5.2 GB), downloads once, the first time, and is checked against a fixed SHA-256. Nothing you write leaves the PC
- On the PC and the TV, speaking aloud (shadowing) is no longer a required step. It's an optional Speaking button on the class screen. The listening quiz stays in the class when Windows has a Japanese voice. Neither has ever used the microphone
- Model downloads now pick up where they stopped if the connection drops

## 0.8.0

- N3 Bridge, a new stage of 15 classes: 47 grammar points, 98 words, and 26 kanji with stroke order, three graded readings, three conversations, and the N3 Mastery Exam
- Covers deciding and becoming, timing with ところ and うちに, はず and わけ, hearsay, について and にとって, purpose and cause, さえ and ばかり, べき and わけにはいきません, ば〜ほど, feelings, ずに and たびに, ておく and ちゃう, and work Japanese
- A new N3 Champion rank and achievement
- Every new word and kanji checked against JMdict, with the few it flagged reviewed by hand
- TV: long questions and answers no longer run into the feedback line, and kanji cards keep their memory hook on screen
- Memory hooks that showed boxes on Windows, such as 亻 and 氵, now use characters the font has, and the content build refuses any it can't draw

## 0.7.1

- Who's learning: YomiGo asks once whether you're a woman, a man, or would rather not say. Men get ぼく and おれ in My words, and women あたし, each with an example. わたし stays in the course for everyone. Change it in Settings
- A new Study Guide page explains the words for I, you, he and she, and why Japanese usually leaves them out

## 0.7.0

- Transcribe: in the Text Reader, pick an audio or video file (MP3, M4A, WAV, MP4, and others Windows can play), and YomiGo writes down the Japanese in it, on your PC. Click any word to look it up and add it, like any other text
- Japanese subtitles are saved beside the file as name.ja.srt, which players such as VLC and mpv load by themselves
- The speech model (Whisper large-v3-turbo, 574 MB) downloads once, the first time, and is checked against a fixed SHA-256 before use
- The Text Reader keeps up to 400 lines, with Page (or PgUp and PgDn) to move through them. Before, only the newest few could be picked
- Needs 64-bit Windows 10 or 11 and a processor with AVX2, from about 2013 on
- Fixed: answers typed on a Japanese keyboard could never be right if it added a full-width space, letters, or 、。！. They now count the same as plain ones
- Questions that ask for romaji say they take kana too, and typing practice shows both when you miss

## 0.6.1

- Updating from 0.5.0 inside the app now also fetches the screenshot reader, signed like everything else
- The Android app builds again, with photo reading
- The phone's Text Reader keeps only Japanese lines when you paste

## 0.6.0

- Text Reader: paste Japanese, watch the clipboard for game text, or read a screenshot. Then pick any character to look up the word starting there, and add it to My words with its sentence
- Screenshots are read with the OCR built into Windows 10 and 11, by the new YomiGoOCR.exe
- Android: read text from a photo, on the phone, with ML Kit
- Lookups now undo common endings on Windows too, as Reader always has
- Katakana words are now found from their own spelling in the browser and on Android

## 0.5.0

- New class step: hear a word or sentence and pick its meaning, type what you hear, then repeat lines out loud
- Settings shows whether a Japanese voice is installed, and how to add one. Without one, classes skip listening
- Windows now detects more kinds of Japanese voice
- On Android, a Try it button reports what the phone heard. It checks words, not your accent

## 0.4.0

- My words: your own flashcards, up to 2,000, reviewed alongside the course
- Add them from the dictionary, by typing them in, or by importing from YomiGo Reader or an Anki-style list
- Copy your list out again for Anki, and delete words with a confirmation
- Words saved with their sentence show it when you review

## 0.3.0

- New N4 stage: 19 classes covering plain forms, permission and rules, must, experience, opinions, ability, plans, comparing, reasons, conditionals, giving and receiving, passive and causative, guessing, and polite Japanese, with the N4 Mastery Exam
- 125 new words and 64 new kanji, checked against JMdict, plus 45 grammar points
- 8 new conversations
- Graded reading: 11 original stories, on the classes that use them, with questions and an English switch
- Genki practice now reaches lessons 8 to 12 of book 1
- Roadmap: the long-term goal is now written down, along with what it would take to reach it

## 0.2.0

- Study Guide in the Library: a setup checklist, a daily routine, immersion, mining, and where guides disagree
- Study tips through the course, in YomiGo's own words, after reading the guide sites
- Four new classes: colours, family, weather small talk, and how often, plus ね, よ, and 〜ています
- Dictionary of about 22,000 common words from JMdict, in every app
- Reader finds words from the forms you meet on a page, such as 食べました or 高くない
- Stroke order, drawn one stroke at a time, on kana and kanji cards
- N64 controller support and prompts on the TV app, following the N64 launcher's layouts
- The entrance exam now starts with romaji, as the course does, and has an I don't know button
- Firefox: clearer install steps, and the manifest is ready for Mozilla signing
- Every course word and kanji checked against JMdict. This found and fixed one wrong card: see [the report](FACTCHECK.md)

## 0.1.2

- New class step: type each new word several times
- I don't know button on quizzes and conversations
- Answer boxes are easier to spot, with a bright frame, "Type here", and a blinking cursor
- Copy and paste with Ctrl+C and Ctrl+V, plus buttons on the PC. On Android, use the phone's own copy and paste
- The Windows Japanese keyboard: text you're composing now shows in the answer box, underlined, with the candidate list beside it. Finished text goes straight in
- The name box says why it can't use a Japanese name, instead of ignoring OK
- TV app: hints no longer cover the on-screen keyboard, and the keyboard's Space and F3 keys now work

## 0.1.1

- The picture now fits small windows, so Desktop no longer loses its title and footer on a 640×480 screen
- A real keyboard can type on the TV app's name and answer screens
- TV lessons no longer run past their panel
- Phone home screen: fixed spacing in the class header, and the blossom is pink again
- Updater no longer asks Windows for admin rights

## 0.1.0

First base build.

- Desktop, HTPC, Mobile, and Reader foundations
- 47 classes through N5: 466 taught cards, 16 conversations, and 7 exams
- Entrance exam, spaced repetition, XP, ranks, streaks, achievements, and boss exams
- Optional Genki practice, asked on first run and in Settings
- Signed updates from GitHub on Windows 7 or later, and user-confirmed APK updates on Android
- Windows builds with an automated Windows 95 import check
