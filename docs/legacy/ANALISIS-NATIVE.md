# ANALISIS NATIVE — jalur Extended Gamma Table (a5100)

> Generated: 2026-11-06 23:2x WIB, sesi offline (no camera).
> Tools: objdump + capstone (Thumb-2), nm, strings, ELF parsing.
> Target: libObj.so (16MB), libScalarDaemon.so (1.8MB), libcamera.so (198KB),
>         sony.cameraex.odex / cameraex.dex.

## TL;DR — jawaban untuk "apakah tabel gamma dipakai atau diabaikan?"

**DIPAKAI — ada jalur nyata, bukan ACK kosong.** Tapi ada temuan yang
mengubah arah: `setExtendedGammaTable` kemungkinan besar adalah jalur
**Picture Effect (foto still)**, BUKAN jalur **Picture Profile / S-Log (movie)**.
S-Log2 (movie) hidup di jalur `PP_GAMMA` yang terpisah dan TIDAK di-expose
via CameraEx Java API di a5100.

## Rantai panggilan yang terbukti (statis, byte-level)

```
Java: CameraEx.native_setExtendedGammaTable(GammaTable, boolean)
        (DEX: method #447, signature (LGammaTable;Z)V)
  |
  v  (JNI dynamic registration, library = libcamera.so — see below)
libcamera.so: CameraHardwareDiadem::setExtendedGammaTable(int,int)
        @ 0x221e8, demangled export, ARM mode
        -> builds 3 SDComMsgParam: [id=7,4] [id=38,arg1] [id=127,arg2]
        -> SDComMsg_Basic(MSGID 0x103e, params)
        => Sends to ScalarDaemon (IPC / SDCom)
  |
  v
libScalarDaemon.so: CommandProcessorCameraSetExtendedGammaTable
        SyncProcessor::execute  @ 0x140452  (validates msg)
        SyncProcessor::extractSDComMessage @ 0x14037a
             param 0x26 -> stores pointer at [obj+0x34]
             param 0x7f -> stores bool at [obj+0x38]
             param 0x04 -> stores bool at [obj+0x39]
        AsyncProcessor::execute  @ 0x140570
             @0x1405ea: calls func(pointer, bool38, bool39)
             -> forwarded to Liro imaging engine
  |
  v
libObj.so: CamUser::SequenceSetExGammaTableControl
        (mangled _ZN7CamUser30SequenceSetExGammaTableControlE)
        param ids: PARAMID_EX_GAMMA_TABLE_SW
                   PARAMID_EX_GAMMA_TABLE_POINTER
                   PARAMID_PICTURE_EFFECT_GAMMA_MODE
        error strings at 0xcf29b8..0xcf2b63
        log: "extendedGammaTableModeSet=%d", "extendedGammaTablePointerSet=0x%08x"
```

## Fakta kunci (dengan bukti)

1. **Ada 3 jalur gamma BERBEDA di firmware:**
   - `PP_GAMMA`, `PP_BLACKGAMMA_RANGE`, `PP_BLACKGAMMA_LVL` (libObj.so @ 0xbf8a9f..0xbf8abc)
     → **Picture Profile** = gamma movie (S-Log/Cine). Inilah jalur S-Log asli Sony.
   - `setExtendedGammaTable` / `native_setExtendedGammaTable` → extended gamma table (2048 byte).
   - `setPictureEffectGammaForceOff` / `isPictureEffectGammaForceOff` (GammaTable class)
     → saklar "matikan gamma Picture Effect".

2. **`PARAMID_PICTURE_EFFECT_GAMMA_MODE`** (libObj.so @ 0xcf2ab3/0xcf2b0f)
   → Nama parameternya secara eksplisit bilang "PICTURE_EFFECT". Artinya tabel extended
   gamma yang kita tulis terikat ke mode **Picture Effect**, bukan Picture Profile.

3. **`setPictureEffectGammaForceOff`** — nama method ini juga "PictureEffect". `ForceOff=true`
   berarti: matikan gamma Picture Effect bawaan supaya kurva custom (extended gamma table)
   yang aktif. Konsisten dengan pengamatan runtime kita (ForceOff BEFORE=false → AFTER=true).

4. **CameraEx Java API TIDAK mengekspos Picture Profile / S-Log.**
   `dexdump.txt` CameraEx: ada `setPictureEffect*`, `setExtendedGammaTable`, tapi
   TIDAK ada `setPictureProfile`, `setPPGamma`, `setBlackGamma`, `setSLog`.
   → Di a5100, jalur PP (S-Log movie) tidak dibuka ke apps via CameraEx.

5. **`Liro` = imaging engine yang melayani still MAUPUN movie.**
   Bukti: `SetMovieAfDriveSpeedForLiro`, `SetStillEEFrameRateModeForLiro`,
   `SetEEFrameRateByHDMIForLiro` — Liro dipakai keduanya. Jadi "diteruskan ke Liro"
   TIDAK otomatis berarti "diteruskan ke encoder video".

6. **`CameraHardwareDiadem::setExtendedGammaTable(int,int)` hanya 2 int** — bukan 2048 byte.
   Tabel aktual lewat jalur lain (GammaTable object native di libcamera / shared memory),
   yang diekstrak di lapis JNI. Pointer tabel (field +0x34 di daemon) adalah bukti tabel
   asli ikut dikirim, bukan cuma ACK.

## Kesimpulan (verdict)

- **Tabel TIDAK diabaikan** — ada rantai IPC nyata sampai ke Liro.
- **TAPI** jalur ini adalah **Extended Gamma (Picture Effect / still)**, dan a5100
  **tidak mengekspos Picture Profile / S-Log (movie)** lewat CameraEx Java API.
- Konsekuensi: `setExtendedGammaTable` bisa mengubah kurva gamma untuk **foto (still)**,
  tetapi **belum terbukti** menyentuh pipeline **encode video**. Bahkan lebih mungkin
  tabel ini TIDAK aktif di jalur video, karena video-gamma di Sony = Picture Profile (PP),
  jalur yang terpisah dan terkunci di a5100.

## Rekomendasi langkah berikutnya (urut prioritas)

1. **Verifikasi empiris still vs movie** (paling cepat, off-camera, aman):
   - Uji `setExtendedGammaTable` → ambil **foto JPEG** (bukan video) → analisis histogram.
   - Kalau foto berubah jadi flat/S-Log, berarti jalur = still, dan kesimpulan "video
     tidak tersentuh" makin kuat.
2. **Analisis native lanjut** untuk 100% certainty:
   - Trace apakah pointer tabel (field +0x34) pernah dibaca oleh kode **encoder video**
     (cari xref ke struktur encode / M4ROOT / AVC / XAVC).
   - Bandingkan handler `MSGID_CAMERA_SET_EXTENDED_GAMMATABLE` (daemon @ 0x169e3b)
     vs `MSGID_SET_PICTURE_PROFILE_CMD` (libObj @ 0xbf56c8) — apakah keduanya
     berujung di register Liro yang sama atau berbeda.
3. **Cek apakah PP bisa dibuka paksa**: `pvt_ActionSetPictureProfile` ada di libObj,
   tapi a5100 gak punya menu PP. Kalau jalur PP bisa dipanggil langsung (bypass menu),
   itu satu-satunya rute S-Log **movie** yang valid — tapi kemungkinan besar di-gate
   oleh model/firmware.

## Catatan metodologi

- Apple objdump tidak support `force-thumb`; pakai capstone 5.0.7 (CS_MODE_THUMB).
- libObj.so dan libScalarDaemon.so **stripped**; identifikasi fungsi via:
  demangled dynsym + string xref + literal pool scan (LE u32).
- VMA == file offset untuk segmen LOAD pertama semua lib; konversi manual via
  program header PT_LOAD (p_vaddr/p_offset/p_filesz).

---

## UPDATE 2026-11-07 08:05 — HASIL EMPIRIS (definitif)

Tes foto (GT20): set kurva S-Log2 + setPictureEffectGammaForceOff(true) + buka mode
STILL + burstableTakePicture() → JPEG tersimpan (DSC04692.JPG, 6000x3376, 7.96MB).

**Perbandingan histogram DSC04692 (gamma) vs DSC04691 (normal):**
```
            normal      gamma
mean luma   212.0       212.0
std dev     68.1        68.1
pixel<16    0.5%        0.5%
pixel>240   60.3%       60.4%
bins        2086 3639 5561 2280 3803 4583 7285 60763
            2116 3582 5574 2261 3826 4580 7230 60831
```

**VERDICT EMPIRIS: kurva TIDAK mengubah output foto.** Perbedaan <1% = noise JPEG,
bukan efek gamma. S-Log2 (yang bikin kontras flat + shadow naik drastis) akan
menghasilkan histogram yang JAUH berbeda kalau beneran diterapkan.

**Kesimpulan final:** `setExtendedGammaTable` adalah dead-end — tidak menyentuh foto
maupun video. S-Log2 hanya mungkin lewat Picture Profile (`PP_GAMMA_SLOG2`), yang
di-gate firmware a5100 (`SetPictureProfileForLiro NON SUPPORTED`).

**GT20 crash-after-save:** app SELESAI normal (log "probe finished" + "v20 end" +
foto tersimpan). Crash terjadi di teardown kamera SETELAH foto aman — bukan kegagalan
tes. Foto tetap valid dan dianalisis.

**Foto hasil:** ~/a5100-re/ftp-root/gamma_DSC04692.JPG + normal_DSC04691.JPG

## 2026-11-07 — S-Log2 value FOUND + gate solved (static, dual-confirmed)

### 1. Nilai numerik VALUEID_PP_GAMMA_SLOG2 = 0x480 (domain MWF/ObjMsg)

Dua konfirmasi independen:

**A. Switch converter `PARAMID_PP_GAMMA_TYPE` (MWF 0x9685), libObj.so @ 0x951c4c..0x951d00**
12 nilai wire → indeks kecil, dibaca langsung dari target branch:
```
0x1b7→0 (STD)      0x1b8→1 (CINEMA_TONE1)   0x1b9→2 (CINEMA_TONE2)  0x1ba→3 (ITU709)
0x39c→4 (STD_MOV)  0x39d→5 (STD_STL)        0x39e→6 (CINE1)         0x39f→7 (CINE2)
0x3a0→8 (CINE3)    0x3a1→9 (CINE4)          0x480→10 (SLOG2)        0x481→11 (ITU709_800)
```
Grup nilai 4+6+2 persis mengikuti urutan blok nama VALUEID (still/movie/extra).

**B. Tabel nama VALUEID di .data bersifat ascending-dense**, anchor dari RE lama:
`PICTURE_EFFECT_GAMMA_AUTO=0x4dc @0xed3be4`, `EX_GAMMA_TABLE_OFF=0x4da @0xed3bdc` ✓
→ SLOG2 @0xed3a74 = 0x4dc − 92 entri = **0x480** ✓, ITU709_800 = 0x481 ✓.
Juga terverifikasi: EX_GAMMA 0x4da/0x4db ✓, MOVIE_HSIZE_NA=0x478, 1920=0x479 ... 640=0x47d,
HSIZE_800=0x4cf, HSIZE_3840=0x4d0 (cocok dengan switch MOVIE_SIZE_H 8-nilai — param itu BUKAN gamma).

### 2. Gate "SetPictureProfileForLiro NON SUPPORTED" = salah MsgID, bukan gate model

String error (0xd02946) adalah default-case handler konverter PP di libObj
(fungsi SET PP @ **0x951900**). Pola gate semua converter CamMsgConv:
```
msgId == <khusus>  → proses
msgId == 0x75E6    → proses (generic set)
else → log "...ForLiro NON SUPPORTED MsgID[0x%04x]"
```
PP SET converter menerima **ObjMsg MsgID 0x75AE** (spesifik PP) atau 0x75E6.
Kesimpulan lama "PP ditolak" salah — yang ditolak adalah MsgID yang salah.
(Catatan: switch 8-nilai {0x478-0x47d,0x4cf,0x4d0} milik PARAMID_MOVIE_SIZE_H,
bukan gamma — hampir saja salah label.)

### 3. Wire format PP SET (CamMsgConv → SD), libObj @ 0x951900 worker 0x951940..0x952150

- Baca param MWF 0x9673..0x9691 (seluruh PP) + 0x9a47/0x9a48/0x9a53 (PP_MODE)
- Pack pesan byte: [0]=5, [2]=sub-id, [3]=0x40 (=MSGID_SET_PICTURE_PROFILE_CMD),
  **byte[0x1a] = indeks gamma (0-11; S-Log2 = 0x0A)**
- Nilai yang dibawa kabel SD = INDEKS KECIL (0-11), bukan 0x4xx.

### 4. Native route final (libScalarDaemon)

`SystemEventIssuerCamera::setPictureProfile` @0x118248 (thumb):
- struct 0x160B, 43 pasang {value@+8n, flag@+8n+4}
- gamma: value @+0x88, flag @+0x8c → IPC param **0x238**
- akhir: sendEvent(model=2, id=0x40) — cocok dgn byte[3]=0x40 jalur libObj
- resetPictureProfile default gamma=0 (STD) ✓ konsisten dgn indeks 0=STD
- tidak ada caller internal & tidak diimpor libObj → caller asli = lib daemon lain
  (tidak ikut ter-dump). Route praktis: tiru jalur libObj (ObjMsg 0x75AE) atau
  panggil setter native dgn struct sendiri.

### 5. Jawaban blanko

1. **VALUEID_PP_GAMMA_SLOG2 = 0x480** (MWF); **nilai wire SD = 10** (0x0A).
2. **Gate = MsgID routing**; jalur sah: ObjMsg 0x75AE/0x75E6 param 0x9685=0x480,
   atau native struct[0x88]=10 + flag[0x8c]=1 + cmd(2,0x40).
   Risiko kecil: SD bisa minta 0x480 alih-alih 10 di param 0x238 — uji dua-duanya.

### Next: GT21 (minimal, read-only dulu)
- Tes A: native setPictureProfile dgn hanya gamma (flag+0x8c=1, val+0x88=10),
  tangkap log/response, TANPA rekam.
- Tes B: variant val=0x480.
- Baru kalau ACK/notify PP masuk: rekam 3 detik + histogram.
