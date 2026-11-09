package com.rudi.gammatest2;

import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;
import java.io.File;
import java.io.FileWriter;
import java.io.PrintWriter;

public class GammaTestActivity extends Activity {
    static final String TAGV = "GT22";
    TextView statusView;
    static boolean libLoaded = false;
    static String loadReport = "";

    static synchronized void log(String line) {
        System.out.println("GAMMA: " + line);
        PrintWriter w = null;
        try {
            w = new PrintWriter(new FileWriter("/sdcard/GAMMALOG.TXT", true));
            w.println(line);
            w.flush();
        } catch (Throwable t) {
            System.err.println("Log error: " + t);
        } finally {
            if (w != null) {
                try { w.close(); } catch (Throwable t2) { }
            }
        }
    }

    static void tryLoadNative() {
        if (libLoaded) return;
        String[] paths = {
            "/data/lib/com.rudi.gammatest2/lib/libppinjector.so",
            "/data/data/com.rudi.gammatest2/lib/libppinjector.so",
            "/data/local/libppinjector.so",
            "/system/lib/libppinjector.so"
        };
        for (String p : paths) {
            File f = new File(p);
            log(TAGV + ": checking path " + p + " (exists=" + f.exists() + ", len=" + f.length() + ")");
            if (f.exists()) {
                try {
                    System.load(p);
                    libLoaded = true;
                    loadReport = "Loaded: " + p;
                    log(TAGV + ": System.load SUCCESS: " + p);
                    return;
                } catch (Throwable t) {
                    loadReport = "Failed " + p + ": " + t.getMessage();
                    log(TAGV + ": System.load FAILED on " + p + ": " + t);
                }
            }
        }
        try {
            System.loadLibrary("ppinjector");
            libLoaded = true;
            loadReport = "Loaded via loadLibrary";
            log(TAGV + ": System.loadLibrary SUCCESS");
        } catch (Throwable t) {
            loadReport = "All loads failed: " + t.getMessage();
            log(TAGV + ": System.loadLibrary FAILED: " + t);
        }
    }

    public native int injectSLog2(int gammaVal);

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        statusView = new TextView(this);
        statusView.setTextSize(20);
        statusView.setText("GT22 Native S-Log2 Injector\nInitializing...");
        setContentView(statusView);

        log("=== " + TAGV + " onCreate enter ===");

        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    Thread.sleep(600);
                    tryLoadNative();

                    int res = -999;
                    if (libLoaded) {
                        log(TAGV + ": Invoking injectSLog2(10)...");
                        try {
                            res = injectSLog2(10);
                            log(TAGV + ": injectSLog2(10) returned: " + res);
                        } catch (Throwable t) {
                            log(TAGV + ": native invoke error: " + t);
                        }
                    } else {
                        log(TAGV + ": Skipped invoke because libLoaded=false (" + loadReport + ")");
                    }

                    final int finalRes = res;
                    final boolean loaded = libLoaded;
                    final String rpt = loadReport;
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            if (statusView != null) {
                                String msg = "=== GT22 RESULT ===\n" +
                                             "Lib: " + (loaded ? "LOADED" : "FAILED") + "\n" +
                                             rpt + "\n\n" +
                                             (loaded ? "gamma=10 (S-Log2)\nNative result: " + finalRes + "\n" +
                                             (finalRes >= 0 ? "SUCCESS! Check LCD viewfinder!" : "Error code: " + finalRes)
                                             : "Cannot invoke native function") +
                                             "\n\nPress shutter/power to exit.";
                                statusView.setText(msg);
                            }
                        }
                    });

                    log("=== " + TAGV + " execution finished ===");
                } catch (Throwable t) {
                    log(TAGV + ": FATAL: " + t);
                }
            }
        }).start();
    }
}
