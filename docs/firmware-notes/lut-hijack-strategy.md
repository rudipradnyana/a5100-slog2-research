# Strategi Baru: Direct Hardware ISP Tone Curve Overwrite (In-Place LUT Hijack)
**Tanggal:** 2026-11-07  
**Fokus:** Bypass Capability Gate Sony BIONZ X via In-Place Overwrite di `/dev/mem` (Metode Magic Lantern / Fuji X70)

---

## 1. Ground Truth Hasil Evaluasi 4 Hari
1. **Jalur Android HAL (`setExtendedGammaTable` via GT10.apk):**
   - Software return OK, buffer terbaca kembali.
   - **Fisik (Foto/Video):** Nol perubahan. Histogram `normal_DSC04691.JPG` vs `gamma_DSC04692.JPG` via `ffmpeg signalstats` 100% identik (`YMIN=0`, `YLOW=78`, `YAVG=212`).
2. **Jalur Middleware Linux (`setPictureProfile` via slog2_direct ptrace):**
   - Injeksi langsung ke `im.elf` (PID 149), panggil `setPictureProfile(S-Log2)` return `0` (Success, event `0x40` masuk queue).
   - **Fisik (LCD):** Nol perubahan.
3. **Penyebab Utama (Root Cause):**
   - Di firmware ISP sub-core ARM7 (`av-cam.bin`), log `LiroSeq ParamCheckErr` memblokir parameter PP dan alokasi pointer baru karena Model ID A5100 (`0x42030036`) tidak memiliki capability flag tersebut.

---

## 2. Inti Strategi Kimi (In-Place LUT Overwrite)

### Prinsip Utama:
- **Jangan kirim pointer baru:** Mengganti pointer tabel memicu pengecekan validasi model oleh `LiroSeq` yang berujung `ParamCheckErr`.
- **Timpa buffer yang sedang berjalan (In-Place Overwrite):**
  - Kamera saat ini terus-menerus merender video/liveview menggunakan profil bawaan (**Creative Style: Neutral / Standard**).
  - Chip ISP BIONZ X membaca tabel kurva gamma Creative Style tersebut langsung dari area memori DMA host (`/dev/mem`, range fisik `0xc0000000`).
  - Jika kita menimpa isi memori tabel tersebut langsung dengan tabel angka S-Log2 (1024/2048 u16), ISP yang membaca DMA per-frame akan **langsung memproses kurva S-Log2** tanpa perlu persetujuan atau handshake middleware Sony.

---

## 3. Langkah Pelaksanaan (Playbook Teknis)

### Langkah 0: Pelacakan Lokasi LUT Aktif via Memdiff
Pakai tracer **Creative Style Contrast (-3 vs +3)** karena perubahan Contrast murni perubahan slope/kemiringan matematis kurva tone tanpa mengubah matriks warna:
1. Di kamera (telnet):
   ```bash
   # Kondisi 1: Creative Style Standard, Contrast 0 (atau -3)
   dd if=/dev/mem of=/tmp/lut_baseline.bin bs=1 skip=$((0xc0000000)) count=$((0x98000))
   
   # Ubah Contrast di menu kamera ke +3 (atau ubah style ke Monochrome/Vivid)
   # Kondisi 2:
   dd if=/dev/mem of=/tmp/lut_changed.bin bs=1 skip=$((0xc0000000)) count=$((0x98000))
   ```
2. Analisis Diff di Mac/Python:
   - Cari blok memori yang berubah (1024 atau 2048 entri uint16).
   - Verifikasi bentuk kurva: nilai harus naik terus secara teratur (monotonik) dan melengkung (kurva tone).
   - Offset tersebut adalah **alamat fisik Hardware Tone Curve LUT**.

### Langkah 1: Uji Reaktivitas via `devmem`
Uji apakah penulisan ke alamat fisik tersebut langsung berefek ke layar:
```bash
devmem 0xC00XXXXX 16 0x0FFF
```
- Jika liveview LCD berkedip / terpengaruh $\rightarrow$ terbukti direct DMA read aktif tanpa double-buffer lock.

### Langkah 2: Injeksi Penuh S-Log2
- Tulis seluruh 2048 bytes kurva S-Log2 (`slog2_curve.bin`) ke alamat target yang ditemukan.
- Hasil: Creative Style bawaan berubah fungsi menjadi kurva S-Log2 murni di tingkat pemrosesan hardware ISP.

---

## 4. Keunggulan & Keamanan
- **S-Log2 Asli:** Diterapkan di tingkat hardware sensor ISP sebelum kompresi AVC/H.264 video.
- **Nol Risiko Brick:** Penulisan hanya terjadi pada RAM fisik sementara, hilang secara otomatis jika kamera restart atau cabut baterai.

