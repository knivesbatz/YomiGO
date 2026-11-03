// YomiGoTalk: a conversation partner that runs on this PC, with llama.cpp.
// usage: YomiGoTalk --get model.gguf progress.txt     downloads the model
//        YomiGoTalk model.gguf folder yomigo-pid       answers folder\talk_req.txt in folder\talk_out.txt until YomiGo closes
// A request is one message per line: "S", "U" or "A", a tab, then the text, with newlines written as \n.
// An optional "X" line lists side jobs: fix (check the learner's last message), en (translate the reply).
// The reply is "JA: ..." then one line for each side job asked for: "FIX: ...", "EN: ...".
// exit 0 done, or YomiGo closed; 1 bad arguments; 2 the model didn't load; 4 the processor lacks AVX2; 5 download failed
#include <thread>
#include "llama.h"
#include "../helpers/hf_get.h"

// One revision of Google's own build, checked against the SHA-256 Hugging Face lists for it
static const wchar_t MODEL_URL_PATH[] = L"/google/gemma-4-E4B-it-qat-q4_0-gguf/resolve/4b4a2c1d584be7264f87aac328a1bc739ce81b6c/gemma-4-E4B_q4_0-it.gguf";
static const char MODEL_SHA256[] = "676c35070db6dbe52f93e9c864ee0fba4eddea94b9c875d9cb10daff453fbaee";
static const unsigned long long MODEL_SIZE = 5154941280ull;

// Asked after each reply, then forgotten, so they never become part of the conversation. Quoting the text keeps a
// small model from mixing up whose message it's checking.
static std::string ask_fix(const std::string &said) {
    return "(Not part of the chat. A Japanese learner wrote: 「" + said + "」 Check it carefully for mistakes in verb forms, particles, and word choice. Spaces between words, and kana where kanji could go, are fine. "
           "If it's correct and natural, answer only OK. If not, answer with the corrected Japanese, then \" - \", then a few words of English saying why.)";
}
static std::string ask_en(const std::string &ja) { return "(Not part of the chat. Translate this into natural English, and answer with the translation only: 「" + ja + "」)"; }

static void no_log(enum ggml_log_level, const char *, void *) {}

static std::string read_file(const std::wstring &path) {
    std::string s; FILE *f = _wfopen(path.c_str(), L"rb");
    if (!f) return s;
    char b[4096]; size_t n;
    while ((n = fread(b, 1, sizeof b, f)) > 0) s.append(b, n);
    fclose(f);
    return s;
}

// One line, without emoji, which YomiGo's font can't draw
static std::string one_line(std::string s) {
    std::string t;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = (unsigned char)s[i], c1 = i + 1 < s.size() ? (unsigned char)s[i + 1] : 0, c2 = i + 2 < s.size() ? (unsigned char)s[i + 2] : 0;
        if (c >= 0xF0) { i += 3; continue; }                            // four-byte characters: emoji and rare symbols
        if (c == 0xE2 && (c1 == 0x9C || c1 == 0x9D)) { i += 2; continue; } // dingbats, such as sparkles
        if (c == 0xEF && c1 == 0xB8 && c2 == 0x8F) { i += 2; continue; }   // the mark that asks for emoji style
        t += (char)c;
    }
    s = t;
    for (char &c : s) if (c == '\n' || c == '\r') c = ' ';
    while (!s.empty() && s.back() == ' ') s.pop_back();
    while (!s.empty() && s[0] == ' ') s.erase(0, 1);
    return s;
}

static llama_context *ctx;
static const llama_vocab *vocab;
static std::vector<llama_token> cached; // what the context holds, so a reply only reads the new messages
static llama_sampler *smp;

// Reads toks from position at, keeping what's already there before it
static bool feed(const std::vector<llama_token> &toks, size_t at) {
    for (size_t i = at; i < toks.size(); i += 512) {
        int len = (int)(toks.size() - i < 512 ? toks.size() - i : 512);
        if (llama_decode(ctx, llama_batch_get_one(const_cast<llama_token *>(&toks[i]), len))) return false;
        cached.insert(cached.end(), toks.begin() + i, toks.begin() + i + len);
    }
    return true;
}

static std::string generate(int max) {
    std::string out;
    for (int i = 0; i < max; i++) {
        llama_token t = llama_sampler_sample(smp, ctx, -1);
        if (llama_vocab_is_eog(vocab, t)) break;
        char piece[256]; int pn = llama_token_to_piece(vocab, t, piece, sizeof piece, 0, true);
        std::string ps(piece, pn > 0 ? pn : 0);
        if (ps.find("<turn|>") != std::string::npos || ps.find("<|turn>") != std::string::npos) break; // the turn ends
        out += ps;
        if (llama_decode(ctx, llama_batch_get_one(&t, 1))) break;
        cached.push_back(t);
    }
    return out;
}

static std::vector<llama_token> tokenize(const std::string &s, bool bos) {
    std::vector<llama_token> t(s.size() + 16);
    int n = llama_tokenize(vocab, s.data(), (int)s.size(), t.data(), (int)t.size(), bos, true);
    t.resize(n > 0 ? n : 0);
    return t;
}

// A side question after the reply, then the context goes back to just after the reply
static std::string side(const std::string &ask) {
    size_t base = cached.size();
    std::string out;
    if (feed(tokenize(std::string("<turn|>\n<|turn>user\n") + ask + "<turn|>\n<|turn>model\n", false), 0)) out = generate(160);
    llama_memory_seq_rm(llama_get_memory(ctx), 0, (llama_pos)base, -1);
    cached.resize(base);
    return one_line(out);
}

int wmain(int argc, wchar_t **argv) {
    if (argc < 4) return 1;
    if (!wcscmp(argv[1], L"--get")) { pct_path = argv[3]; progress(0); return get_model(argv[2], MODEL_URL_PATH, MODEL_SIZE, MODEL_SHA256); }
    if (!has_avx2()) return 4;
    std::wstring req = std::wstring(argv[2]) + L"\\talk_req.txt", out = std::wstring(argv[2]) + L"\\talk_out.txt";
    HANDLE yomigo = OpenProcess(SYNCHRONIZE, FALSE, (DWORD)_wtoi(argv[3]));
    if (!yomigo) return 1;

    llama_log_set(no_log, 0);
    llama_backend_init();
    llama_model *model = llama_model_load_from_file(utf8(argv[1]).c_str(), llama_model_default_params());
    if (!model) return 2;
    llama_context_params cp = llama_context_default_params();
    cp.n_ctx = 4096; cp.n_batch = 512;
    cp.n_threads = cp.n_threads_batch = (int)(std::thread::hardware_concurrency() / 2 ? std::thread::hardware_concurrency() / 2 : 1);
    if (!(ctx = llama_init_from_model(model, cp))) return 2;
    vocab = llama_model_get_vocab(model);
    smp = llama_sampler_chain_init(llama_sampler_chain_default_params());
    llama_sampler_chain_add(smp, llama_sampler_init_penalties(llama_vocab_n_tokens(vocab), 64, 1.1f, 0.0f, 0.0f));
    llama_sampler_chain_add(smp, llama_sampler_init_top_p(0.9f, 1));
    llama_sampler_chain_add(smp, llama_sampler_init_temp(0.7f));
    llama_sampler_chain_add(smp, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));

    while (WaitForSingleObject(yomigo, 150) == WAIT_TIMEOUT) {
        std::string text = read_file(req), jobs, prompt, said, reply = "JA: (?)";
        if (text.empty()) continue;
        DeleteFileW(req.c_str());
        // Written in Gemma 4's own turn format, as the chat template in the model file writes it.
        // shortcut: this llama.cpp's built-in formatter doesn't know Gemma 4 yet; a different model needs its own format here.
        for (size_t at = 0; at < text.size();) {
            size_t nl = text.find('\n', at); if (nl == std::string::npos) nl = text.size();
            std::string line = text.substr(at, nl - at); at = nl + 1;
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.size() < 3 || line[1] != '\t') continue;
            std::string b = line.substr(2);
            for (size_t k = 0; (k = b.find("\\n", k)) != std::string::npos;) b.replace(k, 2, "\n");
            if (line[0] == 'X') { jobs = b; continue; }
            if (line[0] == 'U') said = b;
            prompt += std::string("<|turn>") + (line[0] == 'S' ? "system" : line[0] == 'A' ? "model" : "user") + "\n" + b + "<turn|>\n";
        }
        prompt += "<|turn>model\n";
        std::vector<llama_token> toks = tokenize(prompt, true);
        if (!toks.empty() && toks.size() < llama_n_ctx(ctx) - 700) { // YomiGo keeps the history short, so this holds
            size_t keep = 0;
            while (keep < cached.size() && keep < toks.size() && cached[keep] == toks[keep]) keep++;
            if (keep == toks.size()) keep--; // the last token must be read again, to get its prediction
            llama_memory_seq_rm(llama_get_memory(ctx), 0, (llama_pos)keep, -1);
            cached.resize(keep);
            if (feed(toks, keep)) {
                std::string ja = one_line(generate(200));
                reply = "JA: " + ja + "\n";
                if (jobs.find("fix") != std::string::npos) reply += "FIX: " + side(ask_fix(one_line(said))) + "\n";
                if (jobs.find("en") != std::string::npos) reply += "EN: " + side(ask_en(ja)) + "\n";
            } else { cached.clear(); llama_memory_clear(llama_get_memory(ctx), true); }
        }
        // Written beside, then renamed, so YomiGo never reads half a reply
        std::wstring tmp = out + L".tmp";
        FILE *f = _wfopen(tmp.c_str(), L"wb");
        if (f) { fwrite(reply.data(), 1, reply.size(), f); fclose(f); MoveFileExW(tmp.c_str(), out.c_str(), MOVEFILE_REPLACE_EXISTING); }
    }
    llama_sampler_free(smp);
    llama_free(ctx);
    llama_model_free(model);
    return 0;
}
