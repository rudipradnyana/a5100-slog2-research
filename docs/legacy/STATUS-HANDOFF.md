# a5100 S-Log2, STATUS & HANDOFF

> Last updated: 2026-11-06 23:05 WIB
> Camera: Sony ILCE-5100 (a5100), firmware 3.10, kernel 3.0.27_nl-rt106
> Owner: Rudi. Goal: real S-Log2 gamma in VIDEO (not just ACK).

## TL;DR, where we are

We can **load** an S-Log2 gamma curve into the camera and flip its activation
switch ON, verified byte-for-byte. We have **NOT yet produced a single valid
S-Log2 video clip.** The recorder `prepare()`+`start()` now succeed in movie
mode, but `stop()` throws and no clip is finalized. The last test (GT18) hard-
crashed the camera (battery pull required) before writing any log.

**Decision agreed with Rudi (2026-11-06): stop app-run crash-looping. Next step
is OFFLINE native analysis (Hopper) to answer one question before touching the
camera again.**

## THE key open question

Does the gamma table we write via `setExtendedGammaTable()` actually reach the
**video encode pipeline**, or is it only ACKed by the service and ignored?

- If USED → fix recording properly (design on paper first, no crash-loop).
- If IGNORED → S-Log2 via `setExtendedGammaTable` will never show in video;
  final answer is "not possible via this path" and we stop torturing the camera.

## Proven facts (do NOT re-derive)

1. Curve load works: write 2048-byte S-Log2 table → `verify32` matches → `setExtendedGammaTable` OK → readback size=2048. Consistent across GT12→GT18.
2. Activation switch: `setPictureEffectGammaForceOff(boolean)` + `isPictureEffectGammaForceOff()` live on **`CameraEx$GammaTable`** (NOT `CameraEx`, NOT ParametersModifier). BEFORE=false → set(true) → AFTER=true. Confirmed GT15→GT18.
3. Open options (DEX-verified): `CameraEx$OpenOptions` has `setInheritSetting(Z)`, `setMovieMode(I)`, `setPreview(Z)`, `setRecordingMode(I)`, `setTargetMedia(String)`. Constants on `CameraEx`: `RECORDING_MODE_STILL/MOVIE/CURRENT`, `MOVIE_MODE_SINGLE/MULTI_STREAMING/CURRENT`.
4. Opening in MOVIE mode fixes the `InhibitedOperationException` on recording: GT17 got `prepare OK` + `start OK` (first time ever).
5. `CameraEx.open()` MUST be on a background thread + Looper.prepare(), 20s timeout. Main-thread = freeze → battery pull.
6. MediaRecorder (Sony `com.sony.scalar.media.MediaRecorder`): full API dumped. `setCamera(CameraEx)`, `setVideoSource(int)` VideoSource.CAMERA=1, `setAudioSource(int)` AudioSource.CAMCORDER=1, `setParameters(Parameters)`, `prepare()`, `start()`, `stop()`, `release()`. `setOutputMedia(String)` wants an **int** (NumberFormatException on "MPEG4"), output format goes via `Parameters.setOutputFormat(String)`.
7. Valid PAL profile from GT17 `getSupportedParameters()` dump:
   - output-format: AVCHD, MPEG4, XAVCS
   - video-size: FHD_1080, HD_1080, VGA (NO HD_720!)
   - frame-rate: 50i, 50p, 25p (NO 30p!)
   - bitrate: FX/FH/PS (AVCHD), HQ/SP (MPEG4), 50M (XAVCS)
   - MPEG4 valid combo ≈ HD_1080 + 25p + HQ/SP

## What failed (do NOT repeat blindly)

- GT12: called `setPictureEffectGammaForceOff` on CameraEx → NoSuchMethodException (wrong class).
- GT12 hold-600s strategy: holding CameraEx session blocks UI recording; camera reboots; curve lost.
- GT13/14: MediaRecorder on thread without Looper.prepare() → RuntimeException.
- GT15: setOutputMedia("MPEG4") → NumberFormatException; setAudioSource CAMCORDER → InhibitedOperationException; setParameters failed (bad order/profile).
- GT17: profile HD_720+30p (INVALID for PAL) → start OK but stop() → IllegalStateException, no clip.
- GT18: hard crash (battery pull) before any log line, reason unknown, likely proxy/listener setup or setCamera ordering triggered native hang.

## Artifacts (backup-20261106-230552)

- `~/a5100-re/backup-20261106-230552/`, all GT13..GT18 sources + APKs, DEX parsers + dumps, telnet helpers.
- DEX parsers (work): `dexparse.py`, `dexfields.py`, `dexmethods.py`, `dexstrings.py`
- DEX dumps: `dexdump.txt` (all methods), `dexstrings.txt` (all strings)
- Stripped odex: `cex_stripped.dex`
- Original files: `~/a5100-re/native/` (sony.cameraex.odex, libObj.so, libScalarDaemon.so, libcamera.so, libObj.rea)
- Curve: `~/a5100-re/slog2_curve.bin` (2048 bytes)
- Full tarball backup: `/Users/rudipradnyana/a5100-backup-20261106-1321.tar.gz` (82MB)

## Next step (tomorrow, OFFLINE, no camera)

Run Hopper (or objdump/readelf) on `~/a5100-re/native/libObj.so` / `libcamera.so`
to trace what `native_setExtendedGammaTable` actually does with the table:
does it store into a live-processing register/state consumed by the video encode
path, or just stash it? Look for the gamma table pointer being read during
encoder setup. This answers THE question before any more camera runs.

## Camera logistics (verified)

- Telnet: `192.168.68.117:23` (home wifi). Must re-enable Telnet Tweak after each boot.
- FTP out from camera: `busybox ftpput -u cam -p cam -P 2121 <MAC_IP> <file>`; pyftpdlib server on Mac 192.168.68.115:2121 (LaunchAgent com.rudi.cameraftp).
- SD card: must be IN camera for on-body tests. Videos: `/android/mnt/sdcard/PRIVATE/M4ROOT/CLIP/C####.MP4` (91 clips, last C0091). Photos: DCIM/100MSDCF/.
- Log: `/android/mnt/sdcard/GAMMALOG.TXT`. BusyBox has no tail/wc/base64/timeout; use `dd bs=1 skip=N`.
- USB MTP install via pmca (`~/a5100-re/pmca-re/`). Install fails with "505 Low battery" unless battery ≥ ~50%.
- APK build recipe (GT15 verified): javac 1.8 (jbr-17) + d8 --min-api 8 + aapt + apksigner --v1 (keystore ~/a5100-re/gammatest/debug.keystore, pass android).
- Camera crash/freeze recovery = battery pull, wait, re-enable telnet.

---

## UPDATE 2026-11-06 23:34, Analisis native OFFLINE selesai

Hasil lengkap di `ANALISIS-NATIVE.md` (di repo ini). Ringkas:

**Verdict:** tabel extended-gamma TIDAK diabaikan (ada rantai IPC nyata ke Liro imaging
engine), TAPI jalur yang kita pakai (`setExtendedGammaTable` / `setPictureEffectGammaForceOff`)
adalah jalur **Picture Effect (foto still)**, BUKAN Picture Profile / S-Log (movie).

**Bukti kunci (byte-level, disasm capstone Thumb-2):**
- `libcamera.so: CameraHardwareDiadem::setExtendedGammaTable(int,int)` @0x221e8 → kirim
  3 param int (id 4/38/127) via SDComMsg ke daemon.
- `libScalarDaemon.so: CommandProcessorCameraSetExtendedGammaTable::AsyncProcessor::execute`
  @0x140570 → pointer tabel di [obj+0x34], 2 bool di [obj+0x38/0x39], diteruskan ke Liro.
- `libObj.so: CamUser::SequenceSetExGammaTableControl` → param `PARAMID_PICTURE_EFFECT_GAMMA_MODE`.
- Gamma **movie** asli = `PP_GAMMA` / `PP_BLACKGAMMA_RANGE` / `PP_BLACKGAMMA_LVL` (libObj.so),
  = **Picture Profile**. CameraEx di a5100 TIDAK mengekspos PP/S-Log (cek dexdump: tidak ada
  `setPictureProfile`/`setSLog`/`setBlackGamma`).

**Next step (besok):**
1. Bukti empiris: setExtendedGammaTable → foto JPEG → cek histogram (kalau flat = jalur still).
2. Bypass Picture Profile: trace `pvt_ActionSetPictureProfile` + `PP_GAMMA` di libObj,
   apakah bisa dipanggil langsung tanpa menu PP (a5100 gak punya menu PP).

**Dependency baru:** `capstone` 5.0.7 (pip) untuk disasm Thumb-2, Apple objdump tidak
support `force-thumb`.

**Backup snapshot baru:** `/Users/rudipradnyana/a5100-backup-20261106-233428.tar.gz`
(mencakup ANALISIS-NATIVE.md + native libs + backup 23:05).

## UPDATE 2026-11-07 09:15, S-Log2 value & gate SOLVED (statis), GT21 siap

Lengkap di `ANALISIS-NATIVE.md` + memory harian 2026-11-07. Ringkas:

1. **VALUEID_PP_GAMMA_SLOG2 = 0x480** (domain MWF); nilai di kabel SD = **indeks 10**.
   Dua konfirmasi: switch 12-nilai PARAMID_PP_GAMMA_TYPE (MWF 0x9685) @libObj 0x951c4c,
   dan tabel VALUEID ascending dengan anchor EX_GAMMA_TABLE_OFF=0x4da.
2. **Gate bukan blokir model**: `ForLiro NON SUPPORTED` = default-case MsgID converter PP.
   Jalur sah: ObjMsg 0x75AE (PP) / 0x75E6 (generic), param 0x9685, struct byte[0x1a]=indeks.
3. Dictionary decoder: base 0x9218 (anchor PARAMID_EX_GAMMA_TABLE_SW=0x9a18 @idx 2048).
   Near-miss: switch {0x478-0x47d,0x4cf,0x4d0} = PARAMID_MOVIE_SIZE_H, bukan gamma.
4. Native route: PictureProfileParameter 0x160B; gamma value@+0x88 flag@+0x8c → IPC 0x238;
   command (model=2, id=0x40). resetPictureProfile default gamma=0=STD.
5. **GT21 built** (`~/a5100-re/gt21/GT21.apk`, signed): probe-only, TIDAK memanggil
   setPictureProfile. Variant B (0x480 mentah) source di /tmp/gt21b.
6. Kamera OFFLINE sejak 08:52 (ping 192.168.68.117 100% loss). Menunggu Rudi.
7. Ghidra 12.2 butuh JDK 25+; pakai openjdk 27 brew. Project: ghidra-proj/a5100obj.
   Build recipe GT21 + field→param map lengkap: memory/2026-11-07.md (workspace) dan ANALISIS-NATIVE.md.
