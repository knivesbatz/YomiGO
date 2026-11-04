# Changelog

## 0.21.1

- Fixes updates from 0.21.0 onwards: the updater now closes the new Windows app properly before swapping in a newer version. 0.20 updates straight to this version

## 0.21.0

A new YomiGo for Windows, a new look everywhere, and a new icon.

- **YomiGo for Windows is rebuilt in Rust**, around the same screens as the phone app, so both get every new feature at once. Everything the PC app did still works: Talk with Sakura, the Text Reader with paste, clipboard watch, screenshots and transcription, keep on top, your Material folder, backups, Wi-Fi sync with the phone, and updates. Your saves are the same files in the same place, so there's nothing to move
- **The look of macOS Big Sur**, on the PC and the phone. A Finder-style sidebar on the PC, with the page title in the toolbar. System Preferences-style settings with switches and segmented controls. Messages-style bubbles in conversations. Notification-style banners. Small centred alerts with the app icon. Light and dark, or follow the system
- **Right-click menus** on dictionary results, your own words, and Text Reader lines: add, copy, hear, or remove
- **Keyboard shortcuts:** Ctrl+1 to Ctrl+8 for each place, Ctrl+F for the dictionary, Ctrl+, for Settings, Esc to go back, and 1 to 4 to pick an answer
- **More reward:** a combo counter for answers in a row, XP that floats up from right answers, XP that counts up on the results screen, and a short sound for every press, switch and page. On Android, a small buzz for right and wrong answers
- **Android:** the back gesture now steps back inside YomiGo instead of closing it, and the icon is a proper adaptive icon
- **The TV app works like a game console's menu:** a row of big tiles with today's class first, round buttons for Settings, Sync and switching learner, a cursor that glows and bounces, its own sounds for moving, choosing and going back, A and B prompts along the bottom, and L and R to flip between places. Typing uses an on-screen keyboard. Set up any controller under Settings, switch the prompts to ✕ and ○, change the stick deadzone, or turn on CRT margins. It scales to the screen, from 640×480 up
- **A new icon and logo** in the Big Sur style: a study card with 読 and a sakura blossom
- The PC window has no title bar buttons. Drag it by the toolbar, double-click the toolbar to zoom, and right-click it to minimise or close. Alt+F4 works too
- **Windows 95, 98, ME, 2000, XP, Vista, 7 and 8 are no longer supported.** YomiGo for Windows now needs Windows 10 or 11, and the Edge WebView2 runtime that comes with them

## 0.20.0

- No class teaches more than 25 new cards now. Classes that had grown to 40 to 66 are split into parts, such as At a restaurant, part 2, each with its own review, practice, typing, quiz, and homework. The course grows from 85 to 124 classes, and the same cards are taught in the same order
- Your place is kept: a save from before this update moves to the matching class, and any class you'd finished stays finished. Saves now name their class by where it sits in the course and which part, so future splits keep your place too
- Exams come after the last part of their class

## 0.19.0

- 117 new kanji, from 159 to 276 kanji cards: every N5 and N4 kanji now has a card. Each has its readings, its meaning, an original memory hook, and stroke order drawn one stroke at a time
- They're taught in the classes about the same things: 肉, 魚, and 茶 with the restaurant, 病 and 院 with health, 買 and 売 with shopping, 東 西 南 北 with directions, and so on
- The full fact-check is back: every word and kanji, 1,698 of them, agrees with JMdict and KanjiVG, with nothing left to review
- Existing saves pick up the new kanji in their reviews, ten a day

## 0.18.0

- Sync over home Wi-Fi: open Settings > Sync on the PC or TV, then Settings > Sync with your PC on the phone, and enter the address and code the PC shows. The PC merges the two copies and both keep the result: each card keeps the copy with more reviews, progress and counters take the higher, your own words are joined, and settings come from the device used last. No account and no internet; the PC listens only while its Sync screen is open, and three wrong codes close it. Windows asks once to allow YomiGo on private networks
- Phone: a tab bar with icons and a highlight on the current tab; Today shows your name, streak, XP, and rank; the blossom no longer sits over the counters at the top; in quizzes, leaving is a small × and I don't know is quieter than Check
- Phone: the Library's media is one list, without the before-and-after headings
- Fixed: on the phone, the petals from 0.17's effects were drawn huge, faded, and behind the page

## 0.17.0

- Right answers burst into sakura petals and sparkles, bigger the longer your run. From three in a row a cheer pops up: いいね！, then すごい！ at five, さいこう！ at ten, and てんさい！ at twenty, with the count. Acing a quiz or passing an exam rains petals with the fanfare. A miss starts the run again. On the PC, TV, and phone, where phones set to reduce motion skip the flying petals
- Exams on the PC and phone: switching to another program or app while a question is open counts that question as wrong, so answers can't be looked up mid-exam. The exam says so when it starts, and the exams list explains it. The TV doesn't do this

## 0.16.0

- 196 more words, from 1,224 to 1,420 word cards, each with an example sentence. A check against core N5 and N4 vocabulary found everyday words the course had skipped, and they're in now: 公園, 塩, 多い, 少ない, 嫌い, いい, 今年, 毎朝, 階段, 会話, and verbs such as 取る, 置く, 返す, 売る, 習う, 答える, 困る, 慣れる, and 続く. Plus words from real learning material: 先輩, 迷惑, 緊張, 海外, 書類, 獣医, and more
- The dictionary and the Text Reader show each word's romaji beside its reading, on the PC, TV, phone, and in YomiGo Reader. こんにちは reads konnichiwa, as it's said
- On the PC, Enter searches the dictionary, and Enter sends in the add-a-word and name boxes, even when another button has focus
- Talk: every message starts with who's speaking, Sakura: or You:, each in its own colour, and suggested corrections show in green
- The phone's Settings has a YomiGo on GitHub button

## 0.15.0

- 215 more words, from 1,009 to 1,224 word cards, each with an example sentence, in 27 classes. Common words that had been missing, such as 忙しい, 甘い, 明るい, 鼻, 大人, すぐ, ゆっくり, and 残念, plus the body and health, animals, food and cooking, the post office and getting around, shops, travel and hotels, the home, feelings, and work
- No more Links tab. YomiGo doesn't send you to other sites: the Study Guide now covers what they taught, in YomiGo's own words, with new pages on pitch accent, reading lots of easy Japanese, using YomiGo with real Japanese (pages, games, manga, videos), speaking with real people, and the JLPT levels. Its last page now starts after N3, where the course ends
- YomiGo is now free software: the code under the GPL, version 3 or later, and the course under Creative Commons BY-SA 4.0, so open Japanese data and open-source tools can be built in
- On the TV, switching to the Study Guide with R no longer leaves the highlight on Dict, where OK would jump back
- On the PC, the Media list shows four picks a page, so the last one no longer runs under the page buttons

## 0.14.0

- 165 more words, from 844 to 1,009 word cards, each with an example sentence, in 22 classes across the course. Chosen from the most frequent words in real learning material that the course didn't teach yet
- Counters for books, small animals, flat things, and months (冊, 匹, 枚, ヶ月), countries and Tokyo, restaurant and izakaya words, shopping and paying (カード, ユーロ, ドル), getting around town (側, 降りる, 中心), and the house
- Everyday verbs: 住む, 生きる, 降る, 違う, 間違える, 受ける, 込む, 気をつける, 出かける, and more
- Describing and reasons: 美しい, 素晴らしい, 優しい, 正しい, 大変, 暇, 楽, 不便, なぜなら, ため
- How people talk about people: 彼ら, 人々, 皆さん, 君, ちゃん, お客様, and why お前 and ばか are rude
- On the lesson card, a meaning that needs two lines no longer runs into the example sentence on the TV
- Existing saves pick up the new words in their reviews, ten a day

## 0.13.0

- 119 more words, from 725 to 844 word cards, each with an example sentence: cooking, sweets, travel, money, rules and signs, the home, the body, and more verbs such as 呼ぶ, 始まる, 見せる, and 頑張る
- Chimes: a right answer rings a bell that climbs a note for each one you get in a row, and acing a quiz or passing an exam plays a fanfare. A miss plays a soft low tone and starts the climb again. Settings > Sounds turns them off, on each device
- Right and wrong are obvious: a big ○ or × after every answer, in Japanese marking style, on the PC, TV, and phone
- Fail an exam and you can practise just the cards you missed, or take an unscored practice test. The missed cards are due in Flashcards straight away
- Multiple-choice answers now offer verbs against verbs and adjectives against adjectives, so a verb can't be guessed by its "to"
- On the PC, Enter in an answer box always checks your answer. Before, it pressed whichever button the mouse was resting on, which could be I don't know
- Pronouns: "I use ぼく" is a choice for who's learning, and the course's examples and Sakura follow it
- Back buttons on more screens: chats, reading, tasks, your words, Genki, practice quizzes, and Talk
- Talk: Sakura uses the words and grammar you've learned lately, her check no longer corrects your word for I or flags either reading of a number such as きゅう and く, Enter sends instead of starting a new topic, and you can scroll back through the chat
- Updates install on their own: the PC and TV check each time YomiGo opens, and a newer version downloads, is checked against its signature, and restarts YomiGo. The phone checks at start and opens Android's install screen, which always needs your tap
- The SENKOSOFT name is gone from the apps and the README
- Finishing the whole course now says Course complete, not N4 complete
- こんにちは and こんばんは: the first class says they're written with は but said wa, typing konnichiwa is accepted everywhere, including your own words, and conversation hints spell particles as they're said (konnichiwa, not konnichiha)
- The Library's Files tab is gone. Classes and Genki still open your Material files

## 0.12.0

- 117 more words, from 608 to 725 word cards, each with a natural example sentence, in 20 classes across the course
- Everyday gaps filled: だれ, たくさん, 欲しい, 前, 中, 後ろ, 隣, 外, 今朝, 先週, みんな, どうして, なんで, スマホ, ラーメン, and more
- Each class's examples use what that class teaches: plans with つもり, the doctor class with たら, gifts with あげる and くれる, and others
- New topics: jobs, the hotel desk, health and feeling ill, household chores, outings, and family beyond parents and siblings
- Existing saves pick up the new words in their reviews, ten a day

## 0.11.0

- 166 new words, from 443 to 608 word cards, each with a natural example sentence and its translation, shown on the lesson card. On the PC and TV, Y speaks the sentence
- Chosen from the most frequent words in real learning material that the course didn't teach, and placed in the classes they belong to: travel, the house and bathroom, the body, school, clothes, family, food and restaurants, directions, hobbies and sport, and weather
- Set phrases people say every day: ただいま, いってきます, おつかれさまです, and more. Natural reactions for casual talk: なるほど, そうなんだ, やっぱり, 実は, とりあえず, しょうがない, and more
- Existing saves keep up: words added to classes you've already taken join your reviews, ten a day
- Every new word checked against JMdict, with the few it flagged reviewed by hand

## 0.10.0

- Genki II practice: lessons 13 to 23, in the 3rd edition's order, practised with YomiGo's own N4 and N3 cards. Switch books with the Genki I / Genki II button, or L and R on the TV
- The TV's Genki list fits on screen, in two columns of six
- Shorter Genki topic lines, so none is cut off on the PC

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
