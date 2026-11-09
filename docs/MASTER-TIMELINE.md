# Master Timeline — Sony A5100 S-Log2 Reverse Engineering

> Seluruh kronologi proyek: dari pertanyaan awal sampai status terakhir.
> Semua tanggal 2026, waktu WIB (UTC+7). Sumber: log sesi harian + dokumen legacy.

---

## Fase 0 — Latar & Motivasi (sebelum 2026-11-02)

**Masalah:** Sony A5100 (ILCE-5100) tidak punya Picture Profile / S-Log — fitur
dihapus Sony dari lini entry-level. A6100 (lebih baru!) juga tidak punya; hanya
A6400 ke atas yang dapat PP. Rudi (filmmaker) ingin merekam flat/log dari kamera
yang dia punya, tanpa beli kamera baru.

**Pertanyaan pemicu (2026-10-27):** kenapa Canon Magic Lantern bisa menambah
RAW recording, sedangkan jalur app Sony tidak bisa sejauh itu? Target dipersempit:
**bukan RAW — cukup S-Log2 di A5100.**

**Riset awal (Tony Northrup video, 2026-11-01):** firmware Sony signed jauh
lebih sulit dari Canon; OpenPocketCine (Pocket3) menunjukkan AI-assisted firmware
hack mungkin. Tidak ada yang pernah membongkar A5100 sebelumnya.

---

## Fase 1 — Bedah Firmware Resmi (2026-11-02)

**Aksi:** unduh firmware resmi A5100 v3.10 (217 MB, rilis 2016-03-14) dari
`support.d-imaging.sony.co.jp`, unpack dengan `fwtool.py`. Dapat partisi
`nflasha3/5/7/15/16` (FAT/ext2/cramfs): Linux ARM32 + Android 2.3.7 (board
`dslr-diadem`).

**Temuan kunci:**
- **String S-Log2 ADA di resource:** `string_english.uxc` memuat menu Picture
  Profile lengkap + "Gamma curve equivalent to S-Log2". `CautionConfig.so` punya
  1847 node `PictureProfile*` termasuk `...GammaSLOG2`. `InhInfo_InhMgr.db`
  punya `INH_FACTOR_CAM_SET_GAMMA_TYPE_PP_GAMMA_SLOG2_TYPE_P`.
  → **Kapabilitas ikut tertanam, hanya jalur menampilkannya yang hilang.**
  (Catatan koreksi: ini artefak basis kode bersama, BUKAN bukti fitur aktif.)
- **Rantai gamma lengkap ditemukan:** Android `CameraEx$GammaTable` API →
  JNI `libcamera.so` (`setExtendedGammaTable(int,int)` @0x321e8, SDComMsg
  0x103e, params id 4/0x26/0x7f) → `libScalarDaemon.so`
  (`CommandProcessorCameraSetExtendedGammaTable`, Sync @0x140452, Async
  @0x140570) → `libObj.so` (`CamUser::SequenceSetExGammaTableControl`,
  `PARAMID_EX_GAMMA_TABLE_SW/_POINTER`) → ISP firmware `av-cam.bin`
  (`EX_GAMMA_TABLE_CONTROL[ON=...][ForceOff=...][Pointer=...]`,
  `Gamma/EC_Gamma.cpp`).
- **Pintu masuk legal:** PMCA (PlayMemories Camera Apps) masih bisa install
  APK via USB (`pmca-console`, `devices.yml`: ILCE-5100 arch CXD90014, apps 2.7,
  updatershell yes). Toko online Sony sudah mati, tidak dibutuhkan.
- **Koreksi penting #1:** string `*ForLiro NON SUPPORTED` ada 221 buah untuk
  semua fungsi (SetColorMode, SetNRLevel, dst) — "Liro" BUKAN nama platform,
  itu pola error generik. Jangan jadikan bukti blokir model.

---

## Fase 2 — Static RE dengan Ghidra (2026-11-02, lanjutan)

**Setup:** Ghidra 12.1.4 (rea menolak 12.2), decompiler mac_arm_64 disalin dari
12.2, JDK 25+. Target: `libObj.so` (16 MB, stripped), `libScalarDaemon.so`
(1.8 MB), `libcamera.so` (198 KB).

**Temuan:**
- `SystemEventIssuerCamera::setExtendedGammaTable(int jobId, int physAddr,
  bool, bool)` @0x128ada → SysParamList {0x101=jobId, 0x251=physAddr tabel,
  0x252/0x253=flag} via issueEvent(0x13, 0x14).
- AsyncProcessor::execute @0x150570: pointer tabel → `getTicketPhysicalAddr`
  → vector::at(0) → tunggu event balasan 0x1e16, cek 0x1c51 (job) + 0x1c52
  (status==1 sukses). **Mesin upload tabel gamma utuh, menunggu konfirmasi.**
- **Insight ticket-system:** tabel gamma TIDAK dikirim inline — hidup di
  SHARED-MEMORY TICKET. Param "pointer" = alamat fisik ticket.
- `CameraEx$GammaTable extends DeviceBuffer` punya `getSize/read/write`
  (stub OpenMemories-Framework, "added in V3").

---

## Fase 3 — GT-Series APK (2026-11-03 s.d. 11-06)

Serial aplikasi Android uji (GT1..GT22), build recipe: javac 1.8 + d8 + aapt +
apksigner v1, keystore debug. Install via PMCA USB (butuh baterai ≥50%).

**Yang terbukti runtime:**
- `CameraEx.open()` works (butuh background thread + Looper.prepare(), 20s
  timeout; main-thread = freeze → cabut baterai).
- Curve load works: tulis 2048-byte S-Log2 table → verify32 match →
  `setExtendedGammaTable` OK → readback size=2048. Konsisten GT12→GT18.
- Activation switch: `setPictureEffectGammaForceOff` ada di **CameraEx$GammaTable**
  (bukan CameraEx). BEFORE=false → set(true) → AFTER=true (GT15–GT18).
- OpenOptions: `setInheritSetting/setMovieMode/setPreview/setRecordingMode/
  setTargetMedia`; RECORDING_MODE_MOVIE membuka jalur rekam (GT17 pertama kali
  `prepare OK` + `start OK`).
- MediaRecorder Sony: `setOutputMedia` minta **int**; profil PAL valid:
  MPEG4 HD_1080+25p+HQ/SP (HD_720 & 30p INVALID → GT17 gagal stop()).

**Yang gagal (empiris, jangan ulang):**
- GT10/GT20 — **DEAD END FINAL:** setExtendedGammaTable → foto JPEG burst →
  histogram `DSC04691` (normal) vs `DSC04692` (gamma) **identik** (mean luma
  212.0, std 68.1, bins sama <1% = noise JPEG). Tabel TIDAK menyentuh output
  still, apalagi video. Analisis native (6 Nov) menjelaskan: jalur ini milik
  **Picture Effect (still)**, bukan Picture Profile (movie). S-Log movie = jalur
  `PP_GAMMA` terpisah, tidak diexpose CameraEx Java.
- GT13/14: MediaRecorder tanpa Looper → RuntimeException.
- GT17: profil PAL invalid → stop() IllegalStateException, tidak ada clip.
- GT18: hard crash (battery pull) sebelum log — penyebab tak diketahui.
- **Keputusan 2026-11-06:** STOP crash-looping app. Desain dulu di kertas.

---

## Fase 4 — Analisis Native Offline: Nilai S-Log2 & Gate (2026-11-07 pagi)

**Pertanyaan blanko:** berapa nilai enum S-Log2, dan apakah gate
"SetPictureProfileForLiro NON SUPPORTED" memblokir A5100?

**Jawaban (dual-confirmed, lihat ANALISIS-NATIVE.md):**
1. **Nilai S-Log2:** domain MWF = **0x480**; nilai di kabel SD = **indeks 10**.
   Konfirmasi A: switch 12-nilai `PARAMID_PP_GAMMA_TYPE` (MWF 0x9685) di
   libObj 0x951c4c: `0x480→10 (SLOG2)`, `0x481→11 (ITU709_800)`, dst.
   Konfirmasi B: tabel VALUEID ascending-dense dengan anchor
   `EX_GAMMA_TABLE_OFF=0x4da` → SLOG2 @0xed3a74 = 0x480. 
   **Koreksi #2:** README/TECH SPEC lama menulis 1165 (0x48d) — SALAH
   (0x48d = param ID 0x238×2+..., kebetulan bentrok MOVIE_SIZE_H).
   **Near-miss:** switch 8-nilai {0x478..0x47d,0x4cf,0x4d0} = PARAMID_MOVIE_SIZE_H,
   BUKAN gamma — hampir salah label.
2. **Gate terpecah:** string error itu = default-case handler konverter PP
   (fungsi SET PP @0x951900). Pola semua converter CamMsgConv:
   `msgId == <khusus> → proses; msgId == 0x75E6 → proses; else → log error`.
   PP menerima **ObjMsg 0x75AE** (khusus) / **0x75E6** (generic). Yang salah
   selama ini adalah **MsgID**, bukan blokir model.
3. **Wire format PP SET:** pesan byte `[0]=5, [2]=sub-id, [3]=0x40
   (MSGID_SET_PICTURE_PROFILE_CMD), byte[0x1a]=indeks gamma (S-Log2=0x0A)`.
4. **Native route final:** struct 0x160B, gamma value@+0x88 flag@+0x8c →
   IPC param 0x238; command (model=2, id=0x40). resetPictureProfile default
   gamma=0 (STD).

**GT21 dibangun** (probe-only, tidak memanggil setPictureProfile; variant A
wire=10, variant B wire=0x480). **GT22 dibangun** (injectSLog2(10) native).
**Keduanya BELUM pernah dijalankan di kamera** (tidak ada catatan run).

---

## Fase 5 — patch_avcam: RAM Patch Capability Gates (2026-11-07)

**Akar masalah di ISP co-processor:** `av-cam.bin` (11,264,692 B, ITRON RTOS,
ARM7 sub-core) di-load ke RAM fisik **0x83541000** oleh kernel driver `liro.ko`
(translasi: Linux Phys = ARM7 Phys + 0x40000000; param kernel
`/sys/module/liro/parameters/adr` = 0x43541000).

**Tiga capability gate ditemukan & dipatch live di RAM** (`patch_avcam` via
`/dev/mem`, byte asli diverifikasi sebelum tulis):

| Target | Alamat fisik | Byte asli | Patch |
|---|---|---|---|
| ExGammaTableOn gate | `0x83813dc4` | `70 b5 0d 46` | `00 20 70 47` (movs r0,#0; bx lr) |
| ul_ExGammaTablePointer gate | `0x83813dfc` | `70 b5 0e 46` | idem |
| PictureProfile gate | `0x838119d0` | `70 b5 0e 46` | idem |

Context struct DSP @`0x83e67020`: `context[0x140]`=PP slot, `context[0x256]`
=ExGammaTableOn flag, `context[0x260]`=pointer.

**Eksekusi SUKSES (2026-11-09, kamera online di 192.168.68.104):** ketiga gate
ter-patch di RAM. **Zero brick risk** — RAM only, stock kembali tiap reboot.
**TAPI:** patch gate ≠ S-Log2 aktif. Belum ada bukti fisik (LCD/histogram).

**slog2_direct** (ptrace RPC ke ScalarDaemon PID 149, panggil
setPictureProfile dengan gamma 1165): return 0 = ACK IPC saja. Dengan koreksi
#2, nilai 1165 salah — jadi "sukses" ini bukan bukti apa pun.

---

## Fase 6 — LUT Hijack Hunt (2026-11-09) — HARI INI

**Strategi Kimi (7 Nov):** jangan kirim pointer baru (memicu `LiroSeq
ParamCheckErr` validasi Model ID 0x42030036) — **timpa in-place buffer kurva
Creative Style aktif di RAM** dengan `slog2_curve.bin` (2048 B, 1024×u16 LE).
ISP membaca DMA per-frame, tidak ada handshake.

**Percobaan 7 Nov GAGAL di langkah 0:** dump_changed/dump_contrast/
dump_check_idle ternyata identik byte-per-byte (MD5 sama
`a28c08408ef39492deec7f445f1da3fb`) — diff 974 region vs baseline = churn
liveview antar waktu, bukan efek Contrast. Alamat dump tidak dicatat konsisten.

**Hari ini (9 Nov), protokol diperbaiki:**

1. **Tool baru dibangun** (libc kamera rusak buat lld → freestanding):
   - `memdump` — dump RAM fisik via mmap2 (dd gagal >2GB: off_t 32-bit
     kernel 3.0.27). Bug fix: syscall nr di r7 (EABI), argc/argv dari stack.
   - `memdiff` — diff dua file DI KAMERA (~1 detik utk 508 KB, tanpa FTP).
   - Toolchain yang works: `xcrun clang -target armv7-none-linux-eabi
     -mcpu=cortex-a9 -mthumb -ffreestanding -fno-stack-protector -O2 -c` +
     `ld.lld --fix-cortex-a8 -static --entry _start`.
2. **Peta memori ScalarDaemon (PID 149, im.elf)** — semua /dev/mem mappings
   terenumerasi. Image plane liveview = 0x92ec0000 (1280×960×2 buffer ganda);
   region parameter = 0x82df9000 (508 KB); work memory ISP = 0xC0000000.
3. **Baseline A/A stabil:** dump 0x92ec0000 dua kali berjarak menit →
   identik byte-per-byte (MD5 sama). Protokol dump sehat.
4. **Diff Contrast -3 vs +3 (region 0x82df9000): hanya 2 diff:**
   - `0x5340C` (fisik `0x82E4C40C`): u16 `0x3CE5`(-3) → `0x3D05`(+3),
     delta **+0x20**, stabil lintas reboot (3 dump +3 independen, semua 0x3D05).
   - `0x574F5`: 32 byte log text daemon (noise).
   → Kandidat **scalar/anchor parameter** (≈95.2%↔95.3% domain 14-bit),
   BUKAN tabel kurva penuh. Verifikasi A==C belum dijalankan.
5. **Region kecil lain identik** antara -3/+3: 0x82d90000 ("IMCMN VER:109"+
   magic 55aa55aa), 0x83438000 (descriptor/log), 0x8e6d1000, 0x82eae000
   (filler 0x5A), 0x965de000.
6. **KAMERA WEDGE 2×** — kedua kali persis saat dump jendela register
   `0xf1000000` (4 KB): telnet connect→reset, ADB `error: closed` (adbd stub),
   lalu ping hilang. Recovery: power cycle. **Kesimpulan: 0xf1000000 DILARANG
   permanen** (register hardware gated → hang).
7. **Pelajaran setting:** crash-reboot me-reset Contrast ke 0; power-off
   bersih menyimpannya. IP kamera berubah-ubah (DHCP): .117 → .104.
8. **File `/tmp` kamera selamat** melewati wedge+reboot (tmpfs tidak wipe).
9. **Percakapan Lumix (Kimi):** analogi DMW-SFU1 (jual kunci fitur yang sudah
   ada) tidak berlaku — kita tidak butuh LUT Sony (kurva S-Log2 publik,
   `slog2_curve.bin` siap). Yang dibutuhkan: buffer kurva aktif yang bisa
   ditimpa. Kalau LUT hijack sukses, pertanyaan lisensi jadi tidak relevan.

---

## Fase 7 — Status Akhir & Roadmap (lihat NEXT-STEPS.md)

**Yang pasti:**
- Arsitektur lengkap terpetakan (dual-core: Linux Cortex-A9 + ITRON ARM7 ISP).
- 3 capability gate ter-patch live RAM (reproducible).
- Nilai S-Log2 = 0x480 MWF / indeks 10 wire (dual-confirmed static).
- Dead ends terbukti empiris: setExtendedGammaTable (GT10/GT20), ptrace ACK
  tanpa efek, dump churn tanpa protokol.
- Kandidat parameter Contrast: 0x82E4C40C (delta 0x20 per step) — perlu A==C.

**Yang belum:**
- SATU clip video dengan kurva flat S-Log2 (target `ffmpeg signalstats`
  YMIN ≥ 90) — BELUM pernah tercapai via jalur software mana pun.
- Protokol A==C untuk konfirmasi kandidat.
- Lokasi buffer LUT aktif yang bisa ditimpa in-place.
- GT21/GT22 belum pernah dijalankan di kamera.

**Risiko/edge yang harus dijaga kontributor baru:** lihat NEXT-STEPS.md §5.
