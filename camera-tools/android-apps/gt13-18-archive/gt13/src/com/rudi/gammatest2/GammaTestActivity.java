package com.rudi.gammatest2;

import android.app.Activity;
import android.os.Bundle;
import java.io.FileWriter;
import java.io.PrintWriter;
import java.lang.reflect.Method;
import java.lang.reflect.Constructor;
import java.lang.reflect.Modifier;

public class GammaTestActivity extends Activity {
    static final String CURVE_B64 = "AAEAAQABAAEAAQABAAEAAQABAAEAAQABAAEPAR0BKwE5AUYBUwFgAW0BeQGGAZIBngGqAbUBwAHMAdcB4gHsAfcBAQIMAhYCIAIqAjMCPQJGAlACWQJiAmsCdAJ9AoUCjgKXAp8CpwKvArgCwALIAs8C1wLfAucC7gL2Av0CBAMLAxMDGgMhAygDLwM2AzwDQwNKA1ADVwNdA2QDagNwA3cDfQODA4kDjwOVA5sDoQOnA60DswO4A74DxAPJA88D1APaA98D5QPqA+8D9QP6A/8DBAQJBA4EEwQYBB0EIgQnBCwEMQQ2BDsEPwREBEkETgRSBFcEWwRgBGUEaQRuBHIEdgR7BH8EgwSIBIwEkASVBJkEnQShBKUEqgSuBLIEtgS6BL4EwgTGBMoEzgTSBNUE2QTdBOEE5QTpBOwE8AT0BPgE+wT/BAMFBgUKBQ0FEQUVBRgFHAUfBSMFJgUqBS0FMQU0BTcFOwU+BUIFRQVIBUsFTwVSBVUFWQVcBV8FYgVlBWkFbAVvBXIFdQV4BXsFfwWCBYUFiAWLBY4FkQWUBZcFmgWdBaAFowWmBakFqwWuBbEFtAW3BboFvQW/BcIFxQXIBcsFzQXQBdMF1gXYBdsF3gXhBeMF5gXpBesF7gXxBfMF9gX5BfsF/gUABgMGBgYIBgsGDQYQBhIGFQYXBhoGHAYfBiEGJAYmBikGKwYuBjAGMgY1BjcGOgY8Bj4GQQZDBkUGSAZKBk0GTwZRBlMGVgZYBloGXQZfBmEGYwZmBmgGagZsBm8GcQZzBnUGeAZ6BnwGfgaABoMGhQaHBokGiwaNBo8GkgaUBpYGmAaaBpwGngagBqIGpAanBqkGqwatBq8GsQazBrUGtwa5BrsGvQa/BsEGwwbFBscGyQbLBs0GzwbRBtMG1QbXBtgG2gbcBt4G4AbiBuQG5gboBuoG7AbtBu8G8QbzBvUG9wb5BvoG/Ab+BgAHAgcEBwUHBwcJBwsHDQcOBxAHEgcUBxYHFwcZBxsHHQceByAHIgckByUHJwcpBysHLAcuBzAHMQczBzUHNwc4BzoHPAc9Bz8HQQdCB0QHRgdHB0kHSwdMB04HUAdRB1MHVQdWB1gHWQdbB10HXgdgB2IHYwdlB2YHaAdqB2sHbQduB3AHcQdzB3UHdgd4B3kHewd8B34HfweBB4IHhAeGB4cHiQeKB4wHjQePB5AHkgeTB5UHlgeYB5kHmwecB54HnwehB6IHowelB6YHqAepB6sHrAeuB68HsQeyB7MHtQe2B7gHuQe7B7wHvQe/B8AHwgfDB8QHxgfHB8kHygfLB80HzgfQB9EH0gfUB9UH1wfYB9kH2wfcB90H3wfgB+EH4wfkB+UH5wfoB+kH6wfsB+0H7wfwB/EH8wf0B/UH9wf4B/kH+wf8B/0H/gcACAEIAggECAUIBggICAkICggLCA0IDggPCBAIEggTCBQIFQgXCBgIGQgaCBwIHQgeCB8IIQgiCCMIJAgmCCcIKAgpCCsILAgtCC4ILwgxCDIIMwg0CDUINwg4CDkIOgg7CD0IPgg/CEAIQQhDCEQIRQhGCEcISAhKCEsITAhNCE4ITwhRCFIIUwhUCFUIVghYCFkIWghbCFwIXQheCGAIYQhiCGMIZAhlCGYIZwhpCGoIawhsCG0IbghvCHAIcghzCHQIdQh2CHcIeAh5CHoIewh9CH4IfwiACIEIggiDCIQIhQiGCIcIiQiKCIsIjAiNCI4IjwiQCJEIkgiTCJQIlQiWCJcImAiaCJsInAidCJ4InwigCKEIogijCKQIpQimCKcIqAipCKoIqwisCK0IrgivCLAIsQiyCLMItAi1CLYItwi4CLkIugi7CLwIvQi+CL8IwAjBCMIIwwjECMUIxgjHCMgIyQjKCMsIzAjNCM4IzwjQCNEI0gjTCNQI1QjWCNcI2AjZCNoI2wjcCN0I3gjfCOAI4QjiCOMI5AjkCOUI5gjnCOgI6QjqCOsI7AjtCO4I7wjwCPEI8gjzCPQI9Aj1CPYI9wj4CPkI+gj7CPwI/Qj+CP8IAAkACQEJAgkDCQQJBQkGCQcJCAkJCQoJCgkLCQwJDQkOCQ8JEAkRCRIJEgkTCRQJFQkWCRcJGAkZCRoJGgkbCRwJHQkeCR8JIAkhCSIJIgkjCSQJJQkmCScJKAkoCSkJKgkrCSwJLQkuCS8JLwkwCTEJMgkzCTQJNQk1CTYJNwk4CTkJOgk6CTsJPAk9CT4JPwlACUAJQQlCCUMJRAlFCUUJRglHCUgJSQlKCUoJSwlMCU0JTglOCU8JUAlRCVIJUwlTCVQJVQlWCVcJVwlYCVkJWglbCVwJXAldCV4JXwlgCWAJYQliCWMJZAlkCWUJZglnCWgJaAlpCWoJawlsCWwJbQluCW8JbwlwCXEJcglzCXMJdAl1CXYJdwl3CXgJeQl6CXoJewl8CX0Jfgl+CX8JgAmBCYEJggmDCYQJhAmFCYYJhwmHCYgJiQmKCYsJiwmMCY0JjgmOCY8JkAmRCZEJkgmTCZQJlAmVCZYJlwmXCZgJmQmaCZoJmwmcCZ0JnQmeCZ8JnwmgCaEJogmiCaMJpAmlCaUJpgmnCagJqAmpCaoJqgmrCawJrQmtCa4JrwmwCbAJsQmyCbIJswm0CbUJtQm2CbcJtwm4CbkJugm6CbsJvAm8Cb0Jvgm/Cb8JwAnBCcEJwgnDCcMJxAnFCcYJxgnHCcgJyAnJCcoJygk=";

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
    volatile boolean holdDone = false;
    void probeInBackground() {
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
            Object inst = open.invoke(null, args);
            log("bg: open=" + (inst == null ? "null" : "OK"));
            if (inst == null) { probeDone = true; return; }

            Method cg = cx.getMethod("createGammaTable");
            Object table = cg.invoke(inst);
            log("bg: table=" + (table == null ? "null" : "OK"));
            if (table == null) { probeDone = true; return; }

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

            // NEW v11: force OFF picture-effect gamma so extended table is used by the pipeline
            try {
                Method isf = cx.getMethod("isPictureEffectGammaForceOff");
                log("bg: peGammaForceOff BEFORE=" + isf.invoke(inst));
            } catch (Throwable t) {
                log("bg: isPictureEffectGammaForceOff missing: " + cause(t));
            }
            try {
                Method sf = cx.getMethod("setPictureEffectGammaForceOff", boolean.class);
                sf.invoke(inst, Boolean.TRUE);
                log("bg: setPictureEffectGammaForceOff(true) OK");
            } catch (Throwable t) {
                log("bg: setPictureEffectGammaForceOff THREW: " + cause(t));
            }
            try {
                Method isf2 = cx.getMethod("isPictureEffectGammaForceOff");
                log("bg: peGammaForceOff AFTER=" + isf2.invoke(inst));
            } catch (Throwable t) { }
        } catch (Throwable t) {
            log("bg: FATAL: " + cause(t));
        }
        probeDone = true;
        log("bg: probe finished");
        // v13: probe MediaRecorder API + short hold
        try {
            Class<?> mr = Class.forName("com.sony.scalar.media.MediaRecorder");
            log("MR: class OK");
            // enums
            Class<?>[] inner = mr.getDeclaredClasses();
            for (Class<?> c : inner) {
                if (c.isEnum()) {
                    Object[] consts = c.getEnumConstants();
                    StringBuilder sb = new StringBuilder("MR enum " + c.getSimpleName() + ": ");
                    if (consts != null) for (Object o : consts) sb.append(o.toString()).append(" ");
                    log(sb.toString());
                } else {
                    log("MR class: " + c.getName());
                }
            }
            // methods
            StringBuilder msb = new StringBuilder("MR methods: ");
            for (Method m : mr.getDeclaredMethods()) msb.append(m.getName()).append(" ");
            log(msb.toString());
            // ctors
            StringBuilder csb = new StringBuilder("MR ctors: ");
            for (Constructor<?> c : mr.getDeclaredConstructors()) csb.append(c.toString()).append(" | ");
            log(csb.toString());
            // CamcorderProfile
            Class<?> cp = Class.forName("com.sony.scalar.media.MediaRecorder$CamcorderProfile");
            StringBuilder csb2 = new StringBuilder("CP fields: ");
            java.lang.reflect.Field[] fs = cp.getDeclaredFields();
            for (java.lang.reflect.Field f : fs) csb2.append(f.getName()).append(" ");
            log(csb2.toString());
            StringBuilder msb2 = new StringBuilder("CP methods: ");
            for (Method m : cp.getDeclaredMethods()) msb2.append(m.getName()).append(" ");
            log(msb2.toString());
        } catch (Throwable t) {
            log("MR probe THREW: " + cause(t));
        }
        try {
            log("bg: HOLD 300s");
            long holdEnd = System.currentTimeMillis() + 300000;
            while (System.currentTimeMillis() < holdEnd) {
                try { Thread.sleep(1000); } catch (InterruptedException e) { break; }
            }
            log("bg: hold finished");
        } catch (Throwable t) {
            log("bg: hold THREW: " + cause(t));
        } finally {
            holdDone = true;
        }
    }
    private class ProbeRunnable implements Runnable {
        public void run() { probeInBackground(); }
    }
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        log("=== GammaTest v13 start (MR probe) ===");
        Thread t = new Thread(new ProbeRunnable(), "gamma-probe");
        t.setDaemon(true);
        t.start();
        long deadline = System.currentTimeMillis() + 20000;
        while (!probeDone && System.currentTimeMillis() < deadline) {
            try { Thread.sleep(300); } catch (InterruptedException e) { break; }
        }
        if (!probeDone) log("=== TIMEOUT ===");
        log("=== GammaTest v13 waiting bg hold... ===");
        // do NOT finish immediately; wait for the bg thread hold (max 620s) so the process stays alive
        long holdDeadline = System.currentTimeMillis() + 620000;
        while (!holdDone && System.currentTimeMillis() < holdDeadline) {
            try { Thread.sleep(500); } catch (InterruptedException e) { break; }
        }
        log("=== GammaTest v13 end ===");
        finish();
    }
}
