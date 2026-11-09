import re
p = '/tmp/gt18/src/com/rudi/gammatest2/GammaTestActivity.java'
src = open(p).read()

new_method = '''    void recordWithSession(Class<?> cx, Object inst) {
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
'''

start = src.index('    void recordWithSession(')
end = src.index('    private class ProbeRunnable')
src = src[:start] + new_method + '\n' + src[end:]

# add imports: File, Proxy, InvocationHandler, InvocationTargetException
src = src.replace('import java.lang.reflect.Modifier;',
 'import java.lang.reflect.Modifier;\nimport java.lang.reflect.InvocationHandler;\nimport java.lang.reflect.InvocationTargetException;\nimport java.lang.reflect.Proxy;\nimport java.io.File;')

# shortCause helper after cause()
helper = '''    static String shortCause(Throwable t) {
        Throwable c = t;
        while (c instanceof InvocationTargetException && c.getCause() != null) c = c.getCause();
        String s = c.toString();
        int p = s.lastIndexOf('.');
        return p >= 0 ? s.substring(p + 1) : s;
    }
'''
anchor = '    static byte[] b64decode(String s) {'
src = src.replace(anchor, helper + anchor)

# banner v18
src = src.replace('=== GammaTest v17 start (movieModeOpen + inAppRec) ===', '=== GammaTest v18 start (validProfile + listeners + clipWatch) ===')
src = src.replace('=== GammaTest v17 end ===', '=== GammaTest v18 end ===')

open(p, 'w').write(src)
print('patched v18', len(src))
