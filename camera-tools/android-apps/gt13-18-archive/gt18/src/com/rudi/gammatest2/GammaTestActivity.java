package com.rudi.gammatest2;

import android.app.Activity;
import android.os.Bundle;
import android.os.Looper;
import java.io.FileWriter;
import java.io.PrintWriter;
import java.lang.reflect.Method;
import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.Modifier;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Proxy;
import java.io.File;

public class GammaTestActivity extends Activity {
    static final String CURVE_B64 = "AAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEPAR0BKwE5AUYBUwFgAW0BeQGGAZIBngGqAbUBwAHMAdcB4gHsAfcBAQIMAhYCIAIqAjMCPQJGAlACWQJiAmsCdAJ9AoUCjgKXAp8CpwKvArgCwALIAs8C1wLfAucC7gL2Av0CBAMLAxMDGgMhAygDLwM2AzwDQwNKA1ADVwNdA2QDagNwA3cDfQODA4kDjwOVA5sDoQOnA60DswO4A74DxAPJA88D1APaA98D5QPqA+8D9QP6A/8DBAQJBA4EEwQYBB0EIgQnBCwEMQQ2BDsEPwREBEkETgRSBFcEWwRgBGUEaQRuBHIEdgR7BH8EgwSIBIwEkASVBJkEnQShBKUEqgSuBLIEtgS6BL4EwgTGBMoEzgTSBNUE2QTdBOEE5QTpBOwE8AT0BPgE+wT/BAMFBgUKBQ0FEQUVBRgFHAUfBSMFJgUqBS0FMQU0BTcFOwU+BUIFRQVIBUsFTwVSBVUFWQVcBV8FYgVlBWkFbAVvBXIFdQV4BXsFfwWCBYUFiAWLBY4FkQWUBZcFmgWdBaAFowWmBakFqwWuBbEFtAW3BboFvQW/BcIFxQXIBcsFzQXQBdMF1gXYBdsF3gXhBeMF5gXpBesF7gXxBfMF9gX5BfsF/gUABgMGBgYIBgsGDQYQBhIGFQYXBhoGHAYfBiEGJAYmBikGKwYuBjAGMgY1BjcGOgY8Bj4GQQZDBkUGSAZKBk0GTwZRBlMGVgZYBloGXQZfBmEGYwZmBmgGagZsBm8GcQZzBnUGeAZ6BnwGfgaABoMGhQaHBokGiwaNBo8GkgaUBpYGmAaaBpwGngagBqIGpAanBqkGqwatBq8GsQazBrUGtwa5BrsGvQa/BsEGwwbFBscGyQbLBs0GzwbRBtMG1QbXBtgG2gbcBt4G4AbiBuQG5gboBuoG7AbtBu8G8QbzBvUG9wb5BvoG/Ab+BgAHAgcEBwUHBwcJBwsHDQcOBxAHEgcUBxYHFwcZBxsHHQceByAHIgckByUHJwcpBysHLAcuBzAHMQczBzUHNwc4BzoHPAc9Bz8HQQdCB0QHRgdHB0kHSwdMB04HUAdRB1MHVQdWB1gHWQdbB10HXgdgB2IHYwdlB2YHaAdqB2sHbQduB3AHcQdzB3UHdgd4B3kHewd8B34HfweBB4IHhAeGB4cHiQeKB4wHjQePB5AHkgeTB5UHlgeYB5kHmwecB54HnwehB6IHowelB6YHqAepB6sHrAeuB68HsQeyB7MHtQe2B7gHuQe7B7wHvQe/B8AHwgfDB8QHxgfHB8kHygfLB80HzgfQB9EH0gfUB9UH1wfYB9kH2wfcB90H3wfgB+EH4wfkB+UH5wfoB+kH6wfsB+0H7wfwB/EI8wf0B/UH9wf4B/kH+wf8B/0H/gcACAEIAggECAUIBggICAkICggLCA0IDggPCBAIEggTCBQIFQgXCBgIGQgaCBwIHQgeCB8IIQgiCCMIJAgmCCcIKAgpCCsILAgtCC4ILwgxCDIIMwg0CDUINwg4CDkIOgg7CD0IPgg/CEAIQQhDCEQIRQhGCEcISAhKCEsITAhNCE4ITwhRCFIIUwhUCFUIVghYCFkIWghbCFwIXQheCGAIYQhiCGMIZAhlCGYIZwhpCGoIawhsCG0IbghvCHAIcghzCHQIdQh2CHcIeAh5CHoIewh9CH4IfwiACIEIggiDCIQIhQiGCIcIiQiKCIsIjAiNCI4IjwiQCJEIkgiTCJQIlQiWCJcImAiaCJsInAidCJ4InwigCKEIogijCKQIpQimCKcIqAipCKoIqwisCK0IrgivCLAIsQiyCLMItAi1CLYItwi4CLkIugi7CLwIvQi+CL8IwAjBCMIIwwjECMUIxgjHCMgIyQjKCMsIzAjNCI4IzwjQCNEI0gjTCNQI1QjWCNcI2AjZCNoI2wjcCN0I3gjfCOAI4QjiCOMI5AjkCOUI5gjnCOgI6QjqCOsI7AjtCO4I7wjwCPEI8gjzCPQI9Aj1CPYI9wj4CPkI+gj7CPwI/Qj+CP8IAAkACQEJAgkDCQQJBQkGCQcJCAkJCQoJCgkLCQwJDQkOCQ8JEAkRCRIJEgkTCRQJFQkWCRcJGAkZCRoJGgkbCRwJHQkeCR8JIAkhCSIJIgkjCSQJJQkmCScJKAkoCSkJKgkrCSwJLQkuCS8JLwkwCTEJMgkzCTQJNQk1CTYJNwk4CTkJOgk6CTsJPAk9CT4JPwlACUAJQQlCCUMJRAlFCUUJRglHCUgJSQlKCUoJSwlMCU0JTglOCU8JUAlRCVIJUwlTCVQJVQlWCVcJVwlYCVkJWglbCVwJXAldCV4JXwlgCWAJYQliCWMJZAlkCWUJZglnCWgJaAlpCWoJawlsCWwJbQluCW8JbwlwCXEJcglzCXMJdAl1CXYJdwl3CXgJeQl6CXoJewl8CX0Jfgl+CX8JgAmBCYEJggmDCYQJhAmFCYYJhwmHCYgJiQmKCYsJiwmMCY0JjgmOCY8JkAmRCZEJkgmTCZQJlAmVCZYJlwmXCZgJmQmaCZoJmwmcCZ0JnQmeCZ8JnwmgCaEJogmiCaMJpAmlCaUJpgmnCagJqAmpCaoJqgmrCawJrQmtCa4JrwmwCbAJsQmyCbIJswm0CbUJtQm2CbcJtwm4CbkJugm6CbsJvAm8Cb0Jvgm/Cb8JwAnBCcEJwgnDCcMJxAnFCcYJxgnHCcgJyAnJCcoJygk=";

    static void log(String line) {
        System.out.println("GAMMA: " + line);
        try {
            PrintWriter w = new PrintWriter(new FileWriter("/sdcard/GAMMALOG.TXT", true));
            w.println(line);
            w.flush();
            w.close();
        } catch (Throwable t) { }
        try {
            Runtime.getRuntime().exec(new String[]{"log", "-t", "GAMMA", line});
        } catch (Throwable t) { }
    }
    static String cause(Throwable t) {
        StringBuilder sb = new StringBuilder(t.toString());
        Throwable c = t.getCause();
        int depth = 0;
        while (c != null && depth < 4) { sb.append(" <- ").append(c.toString()); c = c.getCause(); depth++; }
        return sb.toString();
    }
    static String shortCause(Throwable t) {
        Throwable c = t;
        while (c instanceof InvocationTargetException && c.getCause() != null) c = c.getCause();
        String s = c.toString();
        int p = s.lastIndexOf('.');
        return p >= 0 ? s.substring(p + 1) : s;
    }
    static byte[] b64decode(String s) {
        return android.util.Base64.decode(s, android.util.Base64.DEFAULT);
    }
    volatile boolean probeDone = false;
    volatile boolean recordDone = false;
    void probeInBackground() {
        try { Looper.prepare(); log("bg: Looper prepared"); } catch (Throwable t) { log("bg: looper prep: " + t); }
        Object inst = null;
        try {
            log("bg: decoding curve...");
            byte[] curve = b64decode(CURVE_B64);
            log("bg: curve len=" + curve.length);
            Class<?> cx = Class.forName("com.sony.scalar.hardware.CameraEx");
            Class<?> oopt = Class.forName("com.sony.scalar.hardware.CameraEx$OpenOptions");
            Constructor<?> oct = oopt.getDeclaredConstructor();
            oct.setAccessible(true);
            Object opts = oct.newInstance();
            // v17: open in MOVIE mode (per TikTok finding + DEX: setMovieMode/setRecordingMode exist on OpenOptions)
            try {
                // constants live on CameraEx: RECORDING_MODE_MOVIE, MOVIE_MODE_SINGLE
                int recMovie = 1, movieSingle = 0;
                for (Field f : cx.getDeclaredFields()) {
                    try {
                        f.setAccessible(true);
                        if (f.getName().equals("RECORDING_MODE_MOVIE")) recMovie = f.getInt(null);
                        else if (f.getName().equals("MOVIE_MODE_SINGLE")) movieSingle = f.getInt(null);
                    } catch (Throwable t) { }
                }
                log("open: RECORDING_MODE_MOVIE=" + recMovie + " MOVIE_MODE_SINGLE=" + movieSingle);
                Method srm = oopt.getMethod("setRecordingMode", int.class);
                srm.invoke(opts, recMovie);
                log("open: setRecordingMode(MOVIE) OK");
                Method smm = oopt.getMethod("setMovieMode", int.class);
                smm.invoke(opts, movieSingle);
                log("open: setMovieMode(SINGLE) OK");
                try {
                    Method spv = oopt.getMethod("setPreview", boolean.class);
                    spv.invoke(opts, Boolean.TRUE);
                    log("open: setPreview(true) OK");
                } catch (Throwable t) { log("open: setPreview THREW: " + cause(t)); }
            } catch (Throwable t) {
                log("open: movie-mode config THREW: " + cause(t));
            }
            Method open = null;
            for (Method m : cx.getDeclaredMethods())
                if (m.getName().equals("open") && Modifier.isStatic(m.getModifiers())) open = m;
            Class<?>[] ps = open.getParameterTypes();
            Object[] args = new Object[ps.length];
            for (int i = 0; i < ps.length; i++) {
                Class<?> p = ps[i];
                if (p == int.class) args[i] = 0;
                else if (p == boolean.class) args[i] = Boolean.FALSE;
                else if (oopt.isAssignableFrom(p)) args[i] = opts;
                else args[i] = null;
            }
            log("bg: calling open()...");
            inst = open.invoke(null, args);
            log("bg: open=" + (inst == null ? "null" : "OK"));
            if (inst == null) { probeDone = true; return; }

            Method cg = cx.getMethod("createGammaTable");
            Object table = cg.invoke(inst);
            log("bg: table=" + (table == null ? "null" : "OK"));
            if (table == null) { probeDone = true; return; }

            // v14 FIX: setPictureEffectGammaForceOff lives on GAMMA TABLE, not CameraEx
            try {
                Class<?> tcls = table.getClass();
                Method isf = tcls.getMethod("isPictureEffectGammaForceOff");
                log("gt: peGammaForceOff BEFORE=" + isf.invoke(table));
            } catch (Throwable t) {
                log("gt: isPictureEffectGammaForceOff missing: " + cause(t));
            }
            try {
                Class<?> tcls = table.getClass();
                Method sf = tcls.getMethod("setPictureEffectGammaForceOff", boolean.class);
                sf.invoke(table, Boolean.TRUE);
                log("gt: setPictureEffectGammaForceOff(true) OK");
            } catch (Throwable t) {
                log("gt: setPictureEffectGammaForceOff THREW: " + cause(t));
            }
            try {
                Class<?> tcls = table.getClass();
                Method isf2 = tcls.getMethod("isPictureEffectGammaForceOff");
                log("gt: peGammaForceOff AFTER=" + isf2.invoke(table));
            } catch (Throwable t) { }

            Method gs = table.getClass().getMethod("getSize");
            log("bg: size=" + gs.invoke(table));

            // write S-Log2 curve
            Method wr = table.getClass().getMethod("write", byte[].class, int.class, int.class, int.class);
            Object rw = wr.invoke(table, curve, 0, curve.length, 0);
            log("bg: write -> " + (rw == null ? "null" : rw.toString()));

            // verify: read back first 32 bytes
            byte[] back = new byte[32];
            Method rd = table.getClass().getMethod("read", byte[].class, int.class, int.class, int.class);
            rd.invoke(table, back, 0, 32, 0);
            StringBuilder hex = new StringBuilder();
            for (int i = 0; i < 32; i++) hex.append(String.format("%02x", back[i]));
            log("bg: verify32=" + hex);

            // set it
            Method st = cx.getMethod("setExtendedGammaTable", table.getClass());
            log("bg: calling setExtendedGammaTable(SLog2)");
            try {
                st.invoke(inst, table);
                log("bg: setExtendedGammaTable OK");
            } catch (Throwable t) {
                log("bg: setExtendedGammaTable THREW: " + cause(t));
            }

            // readback
            Method gt = cx.getMethod("getExtendedGammaTable");
            Object cur = gt.invoke(inst);
            log("bg: readback=" + (cur == null ? "null" : "non-null"));
            if (cur != null) {
                log("bg: readback size=" + cur.getClass().getMethod("getSize").invoke(cur));
            }

            // v14: RECORD VIDEO IN THIS SESSION via MediaRecorder
            recordWithSession(cx, inst);
        } catch (Throwable t) {
            log("bg: FATAL: " + cause(t));
        }
        probeDone = true;
        recordDone = true;
        log("bg: probe finished");
    }

    void recordWithSession(Class<?> cx, Object inst) {
        try {
            log("MR: starting recorder setup v18");
            Class<?> mrC = Class.forName("com.sony.scalar.media.MediaRecorder");
            Class<?> pc = Class.forName("com.sony.scalar.media.MediaRecorder$Parameters");
            Constructor<?> mrc = mrC.getDeclaredConstructor();
            mrc.setAccessible(true);
            Object rec = mrc.newInstance();
            log("MR: instance OK");

            // v18: attach ALL listeners via one dynamic proxy -> async events visible
            try {
                String[] ifs = {
                    "com.sony.scalar.media.MediaRecorder$OnRecordListener",
                    "com.sony.scalar.media.MediaRecorder$OnPreparedListener",
                    "com.sony.scalar.media.MediaRecorder$OnErrorListener",
                    "com.sony.scalar.media.MediaRecorder$OnRecTimeListener",
                    "com.sony.scalar.media.MediaRecorder$OnRecRemainListener",
                    "com.sony.scalar.media.MediaRecorder$OnStreamWriteListener"
                };
                Class<?>[] iClasses = new Class<?>[ifs.length];
                for (int i = 0; i < ifs.length; i++) iClasses[i] = Class.forName(ifs[i]);
                Object proxy = Proxy.newProxyInstance(GammaTestActivity.class.getClassLoader(), iClasses, new EventProxy());
                int attached = 0;
                for (Method m : mrC.getDeclaredMethods()) {
                    String n = m.getName();
                    if (!n.startsWith("setOn")) continue;
                    Class<?>[] pt = m.getParameterTypes();
                    if (pt.length == 1 && pt[0].isInterface()) {
                        try { m.invoke(rec, proxy); attached++; } catch (Throwable t) { log("MR: " + n + " THREW: " + shortCause(t)); }
                    }
                }
                log("MR: listeners attached=" + attached);
            } catch (Throwable t) {
                log("MR: proxy setup THREW: " + cause(t));
            }

            // v18 order: camera -> video source -> audio (optional) -> params -> prepare -> start
            try {
                Method sc = mrC.getMethod("setCamera", cx);
                sc.invoke(rec, inst);
                log("MR: setCamera OK");
            } catch (Throwable t) { log("MR: setCamera THREW: " + cause(t)); }

            int videoSrc = 1;
            try {
                Class<?> vc = Class.forName("com.sony.scalar.media.MediaRecorder$VideoSource");
                for (Field f : vc.getDeclaredFields()) { f.setAccessible(true); if (f.getName().equals("CAMERA")) videoSrc = f.getInt(null); }
            } catch (Throwable t) { }
            try {
                mrC.getMethod("setVideoSource", int.class).invoke(rec, videoSrc);
                log("MR: setVideoSource(" + videoSrc + ") OK");
            } catch (Throwable t) { log("MR: setVideoSource THREW: " + cause(t)); }

            boolean audioOK = false;
            for (int a : new int[]{1, 0}) {
                try {
                    mrC.getMethod("setAudioSource", int.class).invoke(rec, a);
                    log("MR: setAudioSource(" + a + ") OK");
                    audioOK = true;
                    break;
                } catch (Throwable t) { log("MR: setAudioSource(" + a + ") refused: " + shortCause(t)); }
            }
            if (!audioOK) log("MR: audio skipped, going video-only");

            // v18: EXACT supported profile (GT17 SUPPORTED dump): MPEG4 = HD_1080 + 25p + HQ/SP
            try {
                Object prm = mrC.getMethod("getParameters").invoke(rec);
                pc.getMethod("setOutputFormat", String.class).invoke(prm, "MPEG4");
                pc.getMethod("setVideoSize", String.class).invoke(prm, "HD_1080");
                pc.getMethod("setVideoFrameRate", String.class).invoke(prm, "25p");
                try { pc.getMethod("setVideoEncodingBitRate", String.class).invoke(prm, "HQ"); } catch (Throwable t) { log("MR: bitrate THREW: " + cause(t)); }
                try { pc.getMethod("setVideoAspectRatio", String.class).invoke(prm, "16_9"); } catch (Throwable t) { }
                mrC.getMethod("setParameters", pc).invoke(rec, prm);
                log("MR: setParameters OK (MPEG4 HD_1080 25p HQ)");
                Method fl = pc.getMethod("flatten");
                log("MR: params now: " + (String) fl.invoke(prm));
            } catch (Throwable t) {
                log("MR: params setup THREW: " + cause(t));
            }

            try {
                mrC.getMethod("prepare").invoke(rec);
                log("MR: prepare OK");
            } catch (Throwable t) {
                log("MR: prepare THREW: " + cause(t));
                return;
            }
            try {
                mrC.getMethod("start").invoke(rec);
                log("MR: start OK - RECORDING 20s");
            } catch (Throwable t) {
                log("MR: start THREW: " + cause(t));
                return;
            }

            // v18: live clip watch - prove bytes hit the card WHILE recording
            File d1 = new File("/sdcard/PRIVATE/M4ROOT/CLIP");
            File d2 = new File("/android/mnt/sdcard/PRIVATE/M4ROOT/CLIP");
            File clipDir = d1.isDirectory() ? d1 : d2;
            for (int i = 0; i < 10; i++) {
                Thread.sleep(2000);
                try {
                    if (clipDir != null && clipDir.isDirectory()) {
                        File[] files = clipDir.listFiles();
                        int n = 0; long sz = -1; String newest = "-";
                        if (files != null) {
                            for (File f : files) {
                                String nm = f.getName();
                                if (nm.endsWith(".MP4")) {
                                    n++;
                                    if (newest.equals("-") || nm.compareTo(newest) > 0) { newest = nm; sz = f.length(); }
                                }
                            }
                        }
                        log("clipWatch[" + (i + 1) + "]: mp4=" + n + " newest=" + newest + " size=" + sz);
                    } else {
                        log("clipWatch[" + (i + 1) + "]: no clip dir visible");
                    }
                } catch (Throwable t) { log("clipWatch err: " + shortCause(t)); }
            }

            try {
                mrC.getMethod("stop").invoke(rec);
                log("MR: stop OK");
            } catch (Throwable t) {
                log("MR: stop THREW: " + cause(t));
            }
            try { mrC.getMethod("release").invoke(rec); log("MR: released"); } catch (Throwable t) { }
        } catch (Throwable t) {
            log("MR: FATAL: " + cause(t));
        }
    }

    public static class EventProxy implements InvocationHandler {
        public Object invoke(Object proxy, Method m, Object[] args) {
            StringBuilder sb = new StringBuilder("EVENT " + m.getDeclaringClass().getSimpleName() + "." + m.getName() + "(");
            if (args != null) for (int i = 0; i < args.length; i++) sb.append(i > 0 ? "," : "").append(args[i] == null ? "null" : args[i].toString());
            sb.append(")");
            log(sb.toString());
            return null;
        }
    }

    private class ProbeRunnable implements Runnable {
        public void run() { probeInBackground(); }
    }
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        log("=== GammaTest v18 start (validProfile + listeners + clipWatch) ===");
        Thread t = new Thread(new ProbeRunnable(), "gamma-probe");
        t.setDaemon(true);
        t.start();
        long deadline = System.currentTimeMillis() + 120000;
        while (!recordDone && System.currentTimeMillis() < deadline) {
            try { Thread.sleep(500); } catch (InterruptedException e) { break; }
        }
        if (!recordDone) log("=== TIMEOUT ===");
        log("=== GammaTest v18 end ===");
        finish();
    }
}
