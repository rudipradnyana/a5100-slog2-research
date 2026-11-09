# Hasil Eksperimen — Tabel Lengkap

> Kolom status: ✅ terbukti / ❌ dead end (empiris) / ⏸ belum dijalankan / ⚠ butuh verifikasi
> Semua "tanpa efek" = diverifikasi fisik (histogram/LCD), bukan cuma return code.

---

## A. Jalur Software (API layer)

| # | Jalur | Yang dilakukan | Hasil | Status |
|---|---|---|---|---|
| A1 | `CameraEx.setExtendedGammaTable` (GT10) | Tulis 2048-byte S-Log2 table → verify32 match → setExtendedGammaTable OK → readback size=2048 | Histogram foto normal vs gamma **identik** (mean luma 212.0, std 68.1). Tabel tidak menyentuh output. | ❌ dead end |
| A2 | idem + `setPictureEffectGammaForceOff(true)` (GT15–GT20) | Saklar force-off flip BEFORE=false→AFTER=true | Foto burst: DSC04691 vs DSC04692, bins <1% beda = noise JPEG. | ❌ dead end |
| A3 | Analisis native (6 Nov) — kenapa A1/A2 gagal | Disasm full chain | `setExtendedGammaTable` = jalur **Picture Effect (still)**, bukan PP/movie. S-Log movie = jalur `PP_GAMMA` terpisah, tidak diexpose CameraEx Java. | ✅ penjelasan |
| A4 | `setPictureProfile` via ptrace RPC (slog2_direct) | Instantiate `PictureProfileParameter`, gamma=1165, `setPictureProfile()` | Return 0 = **ACK IPC saja**. LCD nol perubahan. PLUS: nilai 1165 (0x48d) ternyata SALAH (koreksi 7 Nov: yang benar indeks 10 / 0x480 MWF). | ❌ invalid test |
| A5 | GT21 (probe-only) | Baca PP capability dari daemon, log ke GAMMALOG.TXT, tanpa panggil setter | **Belum pernah dijalankan di kamera.** | ⏸ |
| A6 | GT22 (injectSLog2(10)) | Panggil native setPictureProfile dengan struct 0x160B (gamma@+0x88=10, flag@+0x8c=1) | **Belum pernah dijalankan.** Fallback: variant gamma=0x480 mentah (slog2_v2.c). | ⏸ |
| A7 | ObjMsg 0x75AE/0x75E6 param 0x9685 (=0x480) | Kirim wire message PP SET langsung (teridentifikasi 7 Nov sebagai jalur sah) | Belum diimplementasikan/diuji. Wire format: `[0]=5,[2]=sub,[3]=0x40,byte[0x1a]=10`. | ⏸ |
| A8 | ADB over TCP | adb connect 192.168.68.104:5555 → device terdaftar tapi shell selalu `error: closed` | adbd stub pabrik. Bukan jalur masuk. | ❌ dead end |

## B. Firmware / RAM layer

| # | Jalur | Yang dilakukan | Hasil | Status |
|---|---|---|---|---|
| B1 | patch_avcam (3 gates) | Patch `0x83813dc4`, `0x83813dfc`, `0x838119d0` → `00 20 70 47` (movs r0,#0; bx lr) di RAM av-cam.bin | Patch sukses & reproducible (byte asli verify dulu). Efek hilang tiap reboot. **Gate terbuka ≠ fitur aktif** — belum diikuti tes fisik. | ✅ patch works / ⚠ efek unproven |
| B2 | slog2_curve.bin (2048 B, 1024×u16 LE) | Kurva S-Log2 hasil generate sendiri (rumus publik Sony) | Siap dipakai utk injeksi apapun. | ✅ aset |
| B3 | LUT hijack attempt 1 (7 Nov) | Dump 0xc0000000 count 0x98000 (nama file ≠ alamat), diff vs baseline | dump_changed/dump_contrast/dump_check_idle **identik MD5** → diff 974 region = churn, bukan Contrast. Scan monotonik 12-bit: 0 kandidat. | ❌ protokol salah |
| B4 | LUT hijack attempt 2 (9 Nov) — protokol benar | memdump + memdiff (on-camera), baseline A/A dulu | A/A identik (protokol sehat). Diff -3 vs +3 di 0x82df9000: hanya **0x5340C** (u16 0x3CE5→0x3D05, delta +0x20) + 32B log. | ⚠ kandidat scalar, perlu A==C |
| B5 | Diff region kecil -3 vs +3 | 0x82d90000 (IMCMN), 0x83438000, 0x8e6d1000, 0x82eae000, 0x965de000 | Semua **IDENTIK** — bukan lokasi kurva. | ✅ eliminasi |
| B6 | Dump jendela register 0xf1000000 (4 KB) | memdump region register ISP | **Kamera wedge 2×** (telnet reset, ADB dead, ping hilang; recovery power-cycle). | ❌ DILARANG |

## C. Temuan analisis (static, dual-confirmed)

| Temuan | Nilai | Bukti |
|---|---|---|
| Nilai S-Log2 domain MWF | **0x480** | Switch 12-nilai PP_GAMMA_TYPE @libObj 0x951c4c; tabel VALUEID ascending anchor 0x4da |
| Nilai di kabel SD | **indeks 10 (0x0A)** | byte[0x1a] wire format PP SET |
| Konversi arm7→linux | `Linux = ARM7 + 0x40000000` | liro.ko param adr=0x43541000 → 0x83541000 |
| PP IPC param gamma | 0x238 (struct +0x88 value, +0x8c flag) | disasm SystemEventIssuerCamera::setPictureProfile @0x118248 |
| Struct PP size | 0x160B byte, 43 pasang {value,flag} @+8n | idem |
| MSGID_SET_PICTURE_PROFILE_CMD | 0x40 (byte[3]), ObjMsg 0x75AE/0x75E6 | disasm converter @0x951900 |
| Dictionary base param ID | 0x9218 (anchor EX_GAMMA_TABLE_SW=0x9a18 @idx 2048) | tabel 3503 nama @0xed068c |
| Model ID A5100 | 0x42030036 | string LiroSeq ParamCheckErr |
| `VALUEID_PP_GAMMA_SLOG2=1165` | **SALAH/historis** | koreksi 7 Nov 09:15; 0x48d = param ID, bukan VALUEID |

## D. Data peta memori (ScalarDaemon PID 149, im.elf)

```
0x00015000-0x00016000 rw-p  im.elf data
0x00016000-0x00420000 rw-p  [heap] 4.2MB — BELUM dibedah (kandidat LUT host-side)
0x0402a000+                 libosal_uipc.so, libpthread, libjemalloc_ds, ...
0x04055000-0x0405d000 rw-s  phys 0x83438000  (32KB descriptor/log — churn log saja)
0x04061000,040af,040fd,0410 rw-s  phys 0x82d90000  ("IMCMN VER:109", identik -3/+3)
0x040aa000-0x040ae000 rw-s  phys 0xf1000000  (REG ISP — ⚠ DILARANG DUMP: wedge 2×)
0x04101000-0x04102000 rw-s  phys 0x8e6d1000  (4KB, identik)
0x04467000-0x04469000 rw-s  phys 0x965de/df  (identik)
0x0448c000-0x04508000 rw-s  phys 0x82df9000  (508KB PARAMETER ISP — kandidat utama)
0x049d0000-0x049d8000 rw-s  phys 0x82eae000  (filler 0x5A, identik)
0x0b380000-0x0b3fc000 rw-s  phys 0x82df9000  (mapping kedua region sama)
0x0d45a000-0x0d4f0000 rw-s  phys 0xC0000000  (608KB work memory ISP — bukan LUT)
```

Image plane liveview (bukan milik daemon, diakses DMA ISP): **0x92ec0000,
1.228.800 byte = 1280×960**, buffer ganda (pola berulang @+0x96000). Churn
besara saat Contrast berubah = bukti piksel berubah, bukan lokasi LUT.

## E. Dead-end & "jangan ulang" (rekap paling penting)

1. **setExtendedGammaTable** (GT10/GT20): tidak menyentuh still/video. FINAL.
2. **ptrace setPictureProfile dgn nilai 1165**: nilai salah; ACK≠efek.
3. **Dump tanpa protokol** (alamat tak dicatat, dump antar waktu): churn.
4. **ADB shell**: ditolak adbd stub.
5. **Dump 0xf1000000**: kamera wedge. DILARANG.
6. **Kirim pointer tabel baru ke ISP**: memicu `LiroSeq ParamCheckErr`.
   Hanya timpa in-place yang lolos validasi model.
7. **Android `CameraEx` untuk movie gamma**: tidak expose PP/S-Log (DEX dump).
8. **Kamera crash-loop via app** (GT18): jangan ulangi tanpa desain.
   Keputusan 6 Nov: analisis dulu, baru sentuh kamera.

## F. Konfigurasi runtime yang perlu diketahui

| Item | Nilai |
|---|---|
| IP kamera (berubah-ubah/DHCP) | .117 (7 Nov) → **.104** (9 Nov) — cek di Tweak |
| IP Mac (FTP server) | .113 (9 Nov), dulu .115/.110 |
| Login FTP | cam / cam |
| Login telnet | root langsung (telnetd Tweak, port 23), prompt `/ # ` |
| Kernel | Linux 3.0.27_nl-rt106 #1 SMP PREEMPT RT (2016-02-19) |
| ScalarDaemon | PID 149, binary im.elf |
| `/tmp` kamera | tmpfs kecil — `No space left on device` gampang; survive wedge+reboot |
| Battery pull | recovery terakhir kalau hard hang |
| Telnet Tweak | harus di-enable ulang tiap boot kamera |
| Baterai min | 50% (PMCA install menolak di bawah itu: "505 Low battery") |
