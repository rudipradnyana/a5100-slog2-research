# NEXT STEPS, Roadmap Kontributor

> Roadmap ini turun dari semua eksperimen. Kamu baca EXPERIMENTS.md dulu. Urutan di sini ikut nilai informasi per biaya. Kamera saat ini stock setelah reboot, Contrast +3.

---

## PRIORITAS 1, Selesaikan verifikasi kandidat scalar (murah, cepat)

**Kandidat:** `0x82E4C40C` (= base 0x82df9000 + offset 0x5340C), u16 LE.
Data sejauh ini: `-3 → 0x3CE5`, `+3 → 0x3D05`, delta `0x20` per 6 step Contrast
(≈0x0A/step?), stabil lintas reboot di +3.

### Protokol A==C (WAJIB sebelum klaim):
```
1. Kamera Contrast -3  → dump A  (memdump 0x82df9000 0x7C000 /tmp/A.bin)
2. Dump lagi → A2      → verifikasi A == A2 (memdiff)  [stability check]
3. Ubah Contrast ke +3 → dump B
4. memdiff A B         → harus cuma 0x5340C (±log noise)
5. Ubah balik ke -3    → dump C
6. memdiff A C         → HARUS "TOTAL diffs=0"
   Kalau A==C dan A!=B → offset ini DIPASTIKAN memikul state Contrast.
7. Bonus: -3 → -2 (1 step) → dump → delta per step = data bonus.
```

### Kalau terkonfirmasi:
- **Bukan LUT**, itu scalar (anchor/shoulder). Dua kemungkinan:
  a. **Scalar control:** ISP membaca scalar ini sebagai input perhitungan kurva.
     Eksperimen lanjutan: tulis nilai ekstrem (0x0000 / 0xFFFF) via devmem →
     amati LCD. Kalau berubah → kita punya **tulis-kanal-parameter langsung**;
     S-Log2 butuh kurva → cari tabelnya (lanjut P2).
  b. **Mirror/copied state:** hanya salinan RAM dari setting menu; tulisan ke
     sini tak berpengaruh. Eksperimen devmem akan membuktikan (tidak berubah).

## PRIORITAS 2, Temukan buffer LUT aktif (target timpa)

### 2a. Bedah heap daemon `[heap] 0x16000-0x420000` (4.2 MB, belum disentuh!)
- LUT host-side kemungkinan besar di sini (ScalarDaemon yang build tabel dari
  parameter). Protocol: dump heap -3/+3 diff (chunked, /proc/149/mem, lihat
  REPRODUCE.md §dump-heap). Scan pola monotonik 1024/2048 entri u16 LE
  (memdiff_lut.py sudah support).

### 2b. Static analysis libObj.so / av-cam.bin (Ghidra), cari konsumen LUT
- String anchor: `Gamma/EC_Gamma.cpp` di av-cam.bin → fungsi pembaca tabel.
- Cari akses ke context struct `0x83e67020` field `+0x260` (ul_ExGammaTablePointer)
  Siapa yang DEREference pointer itu per-frame.
- Cari loop yang copy 2048 byte dari area DMA (polanya: load u16, store u16,
  counter 1024/2048).
- libObj ~32 slot zeroed 512-byte @0xd14f6e stride 0x200 (temuan 6 Nov) =
  kandidat tabel runtime, perlu ditelusuri siapa yang menulisnya.

### 2c. Diff berbasis gambar (paling langsung tapi butuh scene terkontrol)
- Arahkan kamera ke grayscale chart/monitor gradient statis di tripod.
- Dump image plane 0x92ec0000 (1280×960) dua setting → perbandingan nilai
  pixel per-level bisa REKONSTRUKSI kurva aktif dari data (inverse problem).
- Kalau kurva hasil rekonstruksi -3 vs +3 beda shape signifikan → LUT harus
  ada di suatu tempat; bandingkan histogram input→output untuk mencari titik
  LUT berada (before/after黑白level).

## PRIORITAS 3, Jalur wire yang belum diuji (alternatif full-curve)

A7 di EXPERIMENTS.md: kirim ObjMsg 0x75AE/0x75E6 param 0x9685 = 0x480 langsung.
- Implementasi: butuh endpoint IPC (telnet + file descriptor daemon), atau
  GT22 dengan penambahan kirim ObjMsg.
- Risiko rendah, informasi tinggi: kalau ISP menerima tanpa ParamCheckErr →
  seluruh jalur PP terbuka tanpa LUT hijack.
- GT21r/GT22 **belum pernah dijalankan**, jalankan dulu probe-nya sebelum
  menulis kode baru.

## PRIORITAS 4, Memahami mengapa gate-patch belum berpengaruh

B1 terbukti works (patch RAM sukses) tapi tidak ada bukti S-Log2 aktif setelahnya.
Hipotesis:
1. Gate hanya salah satu lapisan, setelah gate terbuka, IPC punya validasi
   kedua (misal Model ID check di handler, bukan di gate).
2. Pembacaan context struct (0x83e67020) terjadi di sisi ITRON, byte gate kita
   patch di region yang benar untuk 2 dari 3 target (perlu re-verify offset
   0x838119d0 = PictureProfile gate, bukan fungsi lain yang kebetulan dekat).
3. Perlu re-run setelah patch: slog2_direct dengan nilai BENAR (indeks 10),
   atau kirim ObjMsg, belum pernah dicombinasi.

## Eksperimen yang jangan diulang (rekap, detail di EXPERIMENTS.md §E)

1. setExtendedGammaTable, dead end FINAL (histogram identik).
2. Dump region `0xf1000000`, kamera wedge 2×. DILARANG.
3. Dump tanpa mencatat alamat / antar waktu, churn.
4. ADB shell, adbd stub.
5. Kirim pointer LUT baru via IPC, ParamCheckErr.
6. Crash-loop via app, desain dulu di kertas (kesepakatan 6 Nov).

## Konvensi eksperimen (biar data kontributor konsisten)

- Nama file dump: `dump_<alamat-fisik>_<state>.bin`, contoh
  `dump_0x82df9000_c-3.bin`. Alamat fisik di NAMA, bukan di catatan terpisah.
- Setiap sesi: mulai dengan A/A stability check, catat IP kamera & IP Mac,
  catat waktu tiap dump.
- Kontras setting: -3 → 0 → +3 (jangan lompat, biar delta per step bisa diukur).
- Setelah tiap eksperimen: catat di EXPERIMENTS.md (PR welcome).
- Wedge recovery: power cycle. Catat kejadian + aksi yang memicunya.

## Ide lanjutan (belum dieksplorasi sama sekali)

- **A6300 firmware diff:** unduh firmware a6300 (punya S-Log2 resmi, Bionz X
  sama), cari tabel S-Log2 asli Sony + matrix S-Gamut di dalamnya → cross-check
  dengan slog2_curve.bin buatan sendiri, dan cari nama fungsi/handler yang
  sama di kedua firmware untuk menemukan LUT loader di A5100.
- **updatershell path:** devices.yml bilang ILCE-5100 punya updatershell ,
  jalur eksekusi kode level lain yang belum disentuh.
- **Backup.bin EEPROM (855 KB, sudah ada dump):** cari flag capability model
  di EEPROM yang dibaca saat boot (kalau flag-nya di situ, mungkin bisa
  diubah lewat jalur setting, bukan RAM patch).
- **Liveview 4K→FHD path:** kurva mungkin beda antara path still vs movie ,
  uji Contrast diff saat MODE MOVIE vs MODE STILL (protokol sama).
