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
            log("MR: starting recorder setup");
            Class<?> mrC = Class.forName("com.sony.scalar.media.MediaRecorder");
            Class<?> pc = Class.forName("com.sony.scalar.media.MediaRecorder$Parameters");
            Constructor<?> mrc = mrC.getDeclaredConstructor();
            mrc.setAccessible(true);
            Object rec = mrc.newInstance();
            log("MR: instance OK");

            // v16: dump supported parameters first (learn what's valid in this state)
            try {
                Method gsp = mrC.getMethod("getSupportedParameters");
                Object sup = gsp.invoke(rec);
                Method fl = pc.getMethod("flatten");
                log("MR: SUPPORTED: " + (String) fl.invoke(sup));
            } catch (Throwable t) { log("MR: getSupportedParameters THREW: " + cause(t)); }

            // v16: configure via Parameters BEFORE touching recorder setters
            try {
                Method gp = mrC.getMethod("getParameters");
                Object prm = gp.invoke(rec);
                log("MR: getParameters OK");
                // output format first (setOutputMedia wanted an int -> format goes via Parameters)
                try {
                    Method sof = pc.getMethod("setOutputFormat", String.class);
                    Field fof = pc.getDeclaredField("OUTPUT_FORMAT_MPEG_4");
                    fof.setAccessible(true);
                    sof.invoke(prm, fof.get(null));
                    log("MR: param OUTPUT_FORMAT=MPEG4 set");
                } catch (Throwable t) { log("MR: setOutputFormat THREW: " + cause(t)); }
                try {
                    Method ss = pc.getMethod("setVideoSize", String.class);
                    Field vsf = pc.getDeclaredField("VIDEO_SIZE_HD_720");
                    vsf.setAccessible(true);
                    ss.invoke(prm, vsf.get(null));
                    log("MR: param VIDEO_SIZE=HD_720 set");
                } catch (Throwable t) { log("MR: setVideoSize THREW: " + cause(t)); }
                try {
                    Method sf = pc.getMethod("setVideoFrameRate", String.class);
                    Field fff = pc.getDeclaredField("VIDEO_FRAME_RATE_30P");
                    fff.setAccessible(true);
                    sf.invoke(prm, fff.get(null));
                    log("MR: param FRAME_RATE=30p set");
                } catch (Throwable t) { log("MR: setFrameRate THREW: " + cause(t)); }
                try {
                    Method sp = mrC.getMethod("setParameters", pc);
                    sp.invoke(rec, prm);
                    log("MR: setParameters OK");
                } catch (Throwable t) { log("MR: setParameters THREW: " + cause(t)); }
            } catch (Throwable t) {
                log("MR: params THREW: " + cause(t));
            }

            // bind camera + sources AFTER format config
            try {
                Method sc = mrC.getMethod("setCamera", cx);
                sc.invoke(rec, inst);
                log("MR: setCamera OK");
            } catch (Throwable t) {
                log("MR: setCamera THREW: " + cause(t));
            }
            int videoSrc = 1;
            try {
                Class<?> vc = Class.forName("com.sony.scalar.media.MediaRecorder$VideoSource");
                for (Field f : vc.getDeclaredFields()) {
                    f.setAccessible(true);
                    if (f.getName().equals("CAMERA")) videoSrc = f.getInt(null);
                }
            } catch (Throwable t) { }
            try {
                Method sv = mrC.getMethod("setVideoSource", int.class);
                sv.invoke(rec, videoSrc);
                log("MR: setVideoSource(" + videoSrc + ") OK");
            } catch (Throwable t) {
                log("MR: setVideoSource THREW: " + cause(t));
            }
            // v16: audio is OPTIONAL - InhibitedOperationException on GT15; try both known sources, keep going on failure
            int[] audioCandidates = {1, 0}; // CAMCORDER, DEFAULT
            for (int a : audioCandidates) {
                try {
                    Method sa = mrC.getMethod("setAudioSource", int.class);
                    sa.invoke(rec, a);
                    log("MR: setAudioSource(" + a + ") OK");
                    break;
                } catch (Throwable t) {
                    log("MR: setAudioSource(" + a + ") refused: " + cause(t));
                }
            }

            // prepare + start + record 15s + stop
            try {
                Method prep = mrC.getMethod("prepare");
                prep.invoke(rec);
                log("MR: prepare OK");
            } catch (Throwable t) {
                log("MR: prepare THREW: " + cause(t));
                return;
            }
            try {
                Method start = mrC.getMethod("start");
                start.invoke(rec);
                log("MR: start OK - RECORDING 15s");
                Thread.sleep(15000);
                Method stop = mrC.getMethod("stop");
                stop.invoke(rec);
                log("MR: stop OK - recorded 15s");
            } catch (Throwable t) {
                log("MR: start/stop THREW: " + cause(t));
            }
            try {
                Method rel = mrC.getMethod("release");
                rel.invoke(rec);
                log("MR: released");
            } catch (Throwable t) { }
        } catch (Throwable t) {
            log("MR: FATAL: " + cause(t));
        }
    }

    private class ProbeRunnable implements Runnable {
        public void run() { probeInBackground(); }
    }
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        log("=== GammaTest v16 start (paramFirst + audioOptional + inAppRec) ===");
        Thread t = new Thread(new ProbeRunnable(), "gamma-probe");
        t.setDaemon(true);
        t.start();
        long deadline = System.currentTimeMillis() + 120000;
        while (!recordDone && System.currentTimeMillis() < deadline) {
            try { Thread.sleep(500); } catch (InterruptedException e) { break; }
        }
        if (!recordDone) log("=== TIMEOUT ===");
        log("=== GammaTest v16 end ===");
        finish();
    }
}
