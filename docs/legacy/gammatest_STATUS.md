# GammaTest build - 2026-11-06
- Toolchain: ~/a5100-re/tools/android-14 (build-tools r34) + froyo/android-2.2_r03-linux/android.jar
- Keystore: debug.keystore (pass: android, alias: androiddebugkey)
- Build: javac -> d8 -> aapt package/add -> apksigner (v1 only)
- GT6.apk = GT5 logic (CameraEx.open(0, OpenOptions) works; next: dump GammaTable internals + native_createGammaTable + setExtendedGammaTable custom table)
- v4 findings: Permission Denial fixed by android.permission.CAMERA in manifest
- createGammaTable() returns GammaTable{mPictureEffectGammaForceOff=false}
- getExtendedGammaTable()/getExtendedGammmaTable() = null (no custom table set yet)
- pmca install: cd ~/a5100-re/pmca-re && ./.venv/bin/python pmca-console.py install -f <apk> -d libusb

## 2026-11-06 09:49 both-apps test
- gammatest2 (v6) crashed when opened together with old app (cameraex session race)
- OLD com.rudi.gammatest (v5) still installed & WORKS (open(0) OK again)
- createGammaTable GammaTable has only mPictureEffectGammaForceOff field visible (no curve array fields - maybe parent class or native fields)
- getExtendedGammaTable/getExtendedGammmaTable = null
- NEXT: open ONLY gammatest2 after fresh boot; if still crashes, its probe dies before first log -> check logcat via telnet for stack trace

## GT8 build note (2026-11-06)
- JDK25 javac output breaks d8 (NPE on inner classes). ALWAYS compile with JDK17 (JBR at /Users/rudipradnyana/Library/Java/JavaVirtualMachines/jbr-17.0.14).
- GT8.apk = full sequence: open -> enumerate gamma signatures -> createGammaTable + dump incl. superclass fields -> try GammaTable(int,int) ctors (0,0),(1,1),(256,12) -> getExtendedGammaTable -> setExtendedGammaTable(factory table) -> readback
- Install pending camera/telnet back online.

## GT9 (2026-11-06 12:46)
- DEX analysis: GammaTable extends DeviceBuffer; DeviceBuffer.write([B,I,I,I), read([B,I,I,I), getSize() available
- Factory GammaTable buffer = 2048 bytes -> assuming 1024 u16 LE entries
- GT9: creates factory table, writes LINEAR curve (0..4095 u16 LE), reads back 32 bytes to verify, then setExtendedGammaTable + readback getSize
- Install pending USB reconnect

## GT9 hang analysis (2026-11-06 13:00)
- v9 froze between "v9 start" and "calling open()" = inside Class.forName / method scan / OpenOptions ctor
- Hypothesis: static init of CameraEx (or OpenOptions) binds to cameraex service which is busy/stuck from previous run
- GT9 v3 built: every tiny step now logged BEFORE executing (forName, ctor, args, open)
- Install pending USB; Rudi must battery-pull to recover from current hang
