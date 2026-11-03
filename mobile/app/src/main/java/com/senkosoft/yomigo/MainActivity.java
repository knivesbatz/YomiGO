package com.senkosoft.yomigo;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageInstaller;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.provider.Settings;
import android.speech.RecognitionListener;
import android.speech.RecognizerIntent;
import android.speech.SpeechRecognizer;
import android.speech.tts.TextToSpeech;
import android.webkit.JavascriptInterface;
import android.webkit.WebResourceRequest;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.Toast;

import com.google.mlkit.vision.common.InputImage;
import com.google.mlkit.vision.text.TextRecognition;
import com.google.mlkit.vision.text.japanese.JapaneseTextRecognizerOptions;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.ArrayList;
import java.util.Locale;

/** A thin shell around the shared web UI. It adds speech, opening links, and updates from GitHub. */
public class MainActivity extends Activity {
    private static final String LATEST = "https://api.github.com/repos/knivesbatz/YomiGO/releases/latest";
    private static final String ACTION_INSTALL = "com.senkosoft.yomigo.INSTALL_STATUS";
    private WebView web;
    private TextToSpeech tts;
    private static final int PICK_IMAGE = 7;
    private volatile boolean ttsJapanese;
    private SpeechRecognizer recognizer;
    private final Handler main = new Handler(Looper.getMainLooper());

    @Override protected void onCreate(Bundle state) {
        super.onCreate(state);
        web = new WebView(this);
        WebSettings s = web.getSettings();
        s.setJavaScriptEnabled(true);
        s.setDomStorageEnabled(true);
        s.setAllowFileAccess(true);
        web.setWebViewClient(new WebViewClient() {
            // Only the bundled pages load inside the app. Everything else goes to the browser.
            @Override public boolean shouldOverrideUrlLoading(WebView v, WebResourceRequest r) {
                if ("file".equals(r.getUrl().getScheme())) return false;
                startActivity(new Intent(Intent.ACTION_VIEW, r.getUrl()));
                return true;
            }
        });
        web.addJavascriptInterface(new Bridge(), "YomiBridge");
        web.loadUrl("file:///android_asset/www/index.html");
        setContentView(web);
        tts = new TextToSpeech(this, st -> { if (st == TextToSpeech.SUCCESS) ttsJapanese = tts.setLanguage(Locale.JAPANESE) >= TextToSpeech.LANG_AVAILABLE; });
    }

    @Override protected void onDestroy() {
        if (tts != null) tts.shutdown();
        if (recognizer != null) recognizer.destroy();
        super.onDestroy();
    }

    @Override public void onBackPressed() {
        if (web.canGoBack()) web.goBack(); else super.onBackPressed();
    }

    private void heard(String text) {
        web.evaluateJavascript("ygHeard(" + JSONObject.quote(text) + ")", null);
    }

    private void listenNow() {
        if (checkSelfPermission(Manifest.permission.RECORD_AUDIO) != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(new String[]{Manifest.permission.RECORD_AUDIO}, 1);
            return;
        }
        if (!SpeechRecognizer.isRecognitionAvailable(this)) {
            Toast.makeText(this, "Speech recognition isn't available on this device.", Toast.LENGTH_LONG).show();
            heard("");
            return;
        }
        if (recognizer == null) {
            recognizer = SpeechRecognizer.createSpeechRecognizer(this);
            recognizer.setRecognitionListener(new RecognitionListener() {
                @Override public void onResults(Bundle b) {
                    ArrayList<String> r = b.getStringArrayList(SpeechRecognizer.RESULTS_RECOGNITION);
                    heard(r != null && !r.isEmpty() ? r.get(0) : "");
                }
                @Override public void onError(int e) { heard(""); }
                @Override public void onReadyForSpeech(Bundle b) { Toast.makeText(MainActivity.this, "Listening...", Toast.LENGTH_SHORT).show(); }
                @Override public void onBeginningOfSpeech() {}
                @Override public void onRmsChanged(float v) {}
                @Override public void onBufferReceived(byte[] b) {}
                @Override public void onEndOfSpeech() {}
                @Override public void onPartialResults(Bundle b) {}
                @Override public void onEvent(int t, Bundle b) {}
            });
        }
        Intent i = new Intent(RecognizerIntent.ACTION_RECOGNIZE_SPEECH);
        i.putExtra(RecognizerIntent.EXTRA_LANGUAGE_MODEL, RecognizerIntent.LANGUAGE_MODEL_FREE_FORM);
        i.putExtra(RecognizerIntent.EXTRA_LANGUAGE, "ja-JP");
        recognizer.startListening(i);
    }

    /** Text recognition runs on the phone; the picture isn't uploaded anywhere. */
    @Override protected void onActivityResult(int code, int result, Intent data) {
        super.onActivityResult(code, result, data);
        if (code != PICK_IMAGE) return;
        if (result != RESULT_OK || data == null || data.getData() == null) { ocrDone(""); return; }
        try {
            InputImage image = InputImage.fromFilePath(this, data.getData());
            TextRecognition.getClient(new JapaneseTextRecognizerOptions.Builder().build()).process(image)
                .addOnSuccessListener(t -> ocrDone(t.getText()))
                .addOnFailureListener(e -> ocrDone(""));
        } catch (Exception e) { ocrDone(""); }
    }

    private void ocrDone(String text) { main.post(() -> web.evaluateJavascript("ygOcr(" + JSONObject.quote(text) + ")", null)); }

    @Override public void onRequestPermissionsResult(int code, String[] p, int[] r) {
        if (r.length > 0 && r[0] == PackageManager.PERMISSION_GRANTED) listenNow(); else heard("");
    }

    class Bridge {
        @JavascriptInterface public String version() { return BuildConfig.VERSION_NAME; }
        @JavascriptInterface public void speak(String t) { if (tts != null) tts.speak(t, TextToSpeech.QUEUE_FLUSH, null, "yg"); }
        @JavascriptInterface public boolean hasVoice() { return ttsJapanese; }
        @JavascriptInterface public void ocrPick() {
            main.post(() -> startActivityForResult(new Intent(Intent.ACTION_GET_CONTENT).setType("image/*").addCategory(Intent.CATEGORY_OPENABLE), PICK_IMAGE));
        }
        @JavascriptInterface public void installVoice() {
            main.post(() -> {
                try { startActivity(new Intent(TextToSpeech.Engine.ACTION_INSTALL_TTS_DATA)); }
                catch (Exception e) { toast("Add a Japanese voice in your phone's text-to-speech settings."); }
            });
        }
        @JavascriptInterface public void listen() { main.post(MainActivity.this::listenNow); }
        @JavascriptInterface public void openUrl(String u) {
            // Web links only; a page can't use this to launch other kinds of intent
            if (u != null && u.startsWith("https://")) main.post(() -> startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(u))));
        }
        @JavascriptInterface public void checkUpdate() { new Thread(MainActivity.this::checkForUpdate).start(); }
    }

    private void toast(String m) { main.post(() -> Toast.makeText(this, m, Toast.LENGTH_LONG).show()); }

    private static byte[] get(String url) throws Exception {
        HttpURLConnection c = (HttpURLConnection) new URL(url).openConnection();
        c.setRequestProperty("User-Agent", "YomiGoMobile");
        c.setConnectTimeout(15000);
        c.setReadTimeout(60000);
        if (c.getResponseCode() != 200 || !c.getURL().getProtocol().equals("https")) throw new Exception("HTTP " + c.getResponseCode());
        try (InputStream in = c.getInputStream()) {
            ByteArrayOutputStream out = new ByteArrayOutputStream();
            byte[] b = new byte[65536];
            for (int n; (n = in.read(b)) > 0; ) out.write(b, 0, n);
            return out.toByteArray();
        }
    }

    static boolean newer(String a, String b) {
        String[] x = a.split("\\."), y = b.split("\\.");
        for (int i = 0; i < 3; i++) {
            int p = i < x.length ? Integer.parseInt(x[i]) : 0, q = i < y.length ? Integer.parseInt(y[i]) : 0;
            if (p != q) return p > q;
        }
        return false;
    }

    private void checkForUpdate() {
        try {
            JSONObject rel = new JSONObject(new String(get(LATEST), "UTF-8"));
            String tag = rel.getString("tag_name").replaceFirst("^v", "");
            if (!newer(tag, BuildConfig.VERSION_NAME)) { toast("You already have the latest version."); return; }
            String url = null;
            JSONArray assets = rel.getJSONArray("assets");
            for (int i = 0; i < assets.length(); i++) {
                JSONObject a = assets.getJSONObject(i);
                if ("YomiGo.apk".equals(a.getString("name"))) url = a.getString("browser_download_url");
            }
            if (url == null) { toast("This release has no Android build yet."); return; }
            final String dl = url;
            main.post(() -> new AlertDialog.Builder(this).setTitle("YomiGo " + tag)
                .setMessage("A new version is available. Download and install it? Android will ask you to confirm.")
                .setPositiveButton("Install", (d, w) -> new Thread(() -> install(dl)).start())
                .setNegativeButton("Later", null).show());
        } catch (Exception e) {
            toast("Couldn't check for updates: " + e.getMessage());
        }
    }

    /** Android checks the APK is signed with the same key as this app, and always asks the user to confirm. */
    private void install(String url) {
        try {
            if (Build.VERSION.SDK_INT >= 26 && !getPackageManager().canRequestPackageInstalls()) {
                toast("Allow YomiGo to install updates, then try again.");
                startActivity(new Intent(Settings.ACTION_MANAGE_UNKNOWN_APP_SOURCES, Uri.parse("package:" + getPackageName())));
                return;
            }
            toast("Downloading...");
            byte[] apk = get(url);
            PackageInstaller pi = getPackageManager().getPackageInstaller();
            int id = pi.createSession(new PackageInstaller.SessionParams(PackageInstaller.SessionParams.MODE_FULL_INSTALL));
            try (PackageInstaller.Session s = pi.openSession(id)) {
                try (OutputStream o = s.openWrite("yomigo.apk", 0, apk.length)) { o.write(apk); s.fsync(o); }
                registerReceiver(new BroadcastReceiver() {
                    @Override public void onReceive(Context c, Intent i) {
                        int st = i.getIntExtra(PackageInstaller.EXTRA_STATUS, PackageInstaller.STATUS_FAILURE);
                        if (st == PackageInstaller.STATUS_PENDING_USER_ACTION) {
                            Intent confirm = i.getParcelableExtra(Intent.EXTRA_INTENT);
                            if (confirm != null) startActivity(confirm);
                        } else if (st != PackageInstaller.STATUS_SUCCESS) {
                            toast("Update failed. Your current version is unchanged.");
                        }
                    }
                }, new IntentFilter(ACTION_INSTALL), Build.VERSION.SDK_INT >= 33 ? Context.RECEIVER_NOT_EXPORTED : 0);
                Intent cb = new Intent(ACTION_INSTALL).setPackage(getPackageName());
                s.commit(PendingIntent.getBroadcast(this, id, cb, PendingIntent.FLAG_MUTABLE | PendingIntent.FLAG_UPDATE_CURRENT).getIntentSender());
            }
        } catch (Exception e) {
            toast("Update failed: " + e.getMessage());
        }
    }
}
