package com.rudi.gammatest2;

import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;
import java.io.FileWriter;
import java.io.PrintWriter;
import java.lang.reflect.Method;

public class GammaTestActivity extends Activity {
    static final String TAGV = "GT21r";
    TextView statusView;

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

    static String cause(Throwable t) {
        StringBuilder sb = new StringBuilder(t.toString());
        Throwable c = t.getCause();
        int depth = 0;
        while (c != null && depth < 4) { sb.append(" <- ").append(c.toString()); c = c.getCause(); depth++; }
        return sb.toString();
    }

    void probe() {
        log("=== " + TAGV + " probe start ===");
        log(TAGV + ": step1 bootclasspath probe");
        String[] direct = {
            "SCALARDAEMON.SystemEventIssuerCamera",
            "SCALARDAEMON.SystemEventIssuerCamera$PictureProfileParameter",
            "com.sony.scalar.hardware.CameraEx",
            "com.sony.scalar.hardware.SystemEventIssuerCamera",
            "com.sony.scalar.hardware.CameraEx$ParametersModifier"
        };
        for (String cn : direct) {
            try {
                Class<?> c = Class.forName(cn, false, GammaTestActivity.class.getClassLoader());
                log(TAGV + ": DIRECT-FOUND " + cn);
                Method[] ms = c.getDeclaredMethods();
                for (int i = 0; i < ms.length; i++) {
                    String n = ms[i].getName();
                    if (n.toLowerCase().contains("pictureprofile") || n.toLowerCase().contains("gamma") || n.toLowerCase().contains("cinematone"))
                        log(TAGV + ":   m " + ms[i].toString());
                }
                log(TAGV + ":   total methods=" + ms.length);
            } catch (Throwable t) {
                log(TAGV + ": direct-miss " + cn + ": " + t.getClass().getName());
            }
        }

        log(TAGV + ": step2 CameraEx ClassLoader probe");
        try {
            Class<?> cx = Class.forName("com.sony.scalar.hardware.CameraEx");
            ClassLoader pcl = cx.getClassLoader();
            log(TAGV + ": CameraEx CL=" + pcl.getClass().getName());
            String[] probes = {
                "SCALARDAEMON.SystemEventIssuerCamera",
                "SCALARDAEMON.SystemEventIssuerCamera$PictureProfileParameter",
                "com.sony.scalar.hardware.SystemEventIssuerCamera"
            };
            for (String cn : probes) {
                try {
                    Class<?> c = Class.forName(cn, false, pcl);
                    log(TAGV + ": CL-FOUND " + cn);
                    Method[] ms = c.getDeclaredMethods();
                    for (Method m : ms) {
                        String n = m.getName();
                        if (n.toLowerCase().contains("pictureprofile") || n.toLowerCase().contains("gamma"))
                            log(TAGV + ":   m " + m.toString());
                    }
                    log(TAGV + ":   total methods=" + ms.length);
                } catch (Throwable t) {
                    log(TAGV + ": cl-miss " + cn + ": " + t.getClass().getName());
                }
            }
        } catch (Throwable t) {
            log(TAGV + ": cameraex load err: " + cause(t));
        }

        log("=== " + TAGV + " probe finished ===");

        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (statusView != null) {
                    statusView.setText("GT21r PROBE DONE!\nCheck /sdcard/GAMMALOG.TXT\nPress shutter/power to exit.");
                }
            }
        });
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        statusView = new TextView(this);
        statusView.setTextSize(20);
        statusView.setText("GT21r Probing...\nPlease wait...");
        setContentView(statusView);

        log("=== " + TAGV + " onCreate enter ===");

        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    Thread.sleep(500);
                    probe();
                } catch (Throwable t) {
                    log(TAGV + ": PROBE-FATAL: " + cause(t));
                }
            }
        }).start();
    }
}

