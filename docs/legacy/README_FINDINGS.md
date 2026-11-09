# Sony A5100 (ILCE-5100) S-Log2 Reverse Engineering & In-Memory Enabler
**Backup & Technical Findings Report — 7 November 2026**

---

## 1. Ringkasan Eksekutif & Temuan Utama
Proyek ini berhasil membongkar arsitektur internal kamera **Sony ILCE-5100 (Firmware v3.10)** dan merekayasa balik rantai pemrosesan sinyal gambar (ISP pipeline) untuk mengaktifkan kurva gamma **S-Log2 (Picture Profile)** secara native:

1. **Akar Masalah (Root Cause):** Sony memblokir Picture Profile dan Extended Gamma di tingkat firmware ISP Co-Processor (`av-cam.bin` yang berjalan pada RTOS ITRON di core ARM terpisah). Terdapat 3 fungsi gerbang kapabilitas (*capability gates*) yang memeriksa variabel context kamera (`context[0x140]`, `context[0x256]`, `context[0x260]`). Pada model A5100, field ini bernilai 0 sehingga perintah S-Log2 langsung ditolak (*invalid parameter / capability error*).
2. **Terobosan RAM Patching (`patch_avcam`):** Firmware ISP dimuat ke physical RAM di `0x83541000` oleh kernel driver `liro.ko`. Dengan memetakan `/dev/mem` langsung di kernel Linux, ketiga gerbang kapabilitas berhasil di-patch secara live di RAM dengan instruksi Thumb `movs r0, #0; bx lr` (`00 20 70 47`). **Zero brick risk**: tidak menyentuh flash storage kamera sama sekali.
3. **Injeksi Parameter Native (`slog2_direct`):** Melalui teknik ptrace RPC ke daemon kamera utama (`ScalarDaemon`, PID 149), parameter C++ `PictureProfileParameter` diinstansiasi secara native, diisi gamma S-Log2 (`VALUEID_PP_GAMMA_SLOG2 = 1165`), dan dipanggil melalui `setPictureProfile()`. **Hasil eksekusi: return 0 (SUCCESS)**.

---

## 2. Spesifikasi Target
- **Model:** Sony ILCE-5100 (A5100)
- **Model ID:** `0x42030036`
- **Firmware Version:** 3.10
- **SoC:** Sony CXD90014 (Dual Core ARM Cortex-A9 host + ARM7 ISP Co-Processor)
- **Host OS:** Sony CE Linux 8.2.1.9 (Kernel `3.0.27_nl-rt106` PREEMPT RT)
- **Sub-sistem:** Android 4.x HAL + ITRON Real-Time OS

---

## 3. Peta Memori Fisik & Driver `liro.ko`
Komunikasi antara Linux host dan ISP ITRON co-processor ditangani oleh modul kernel `liro.ko`:
- **Parameter Kernel:** `/sys/module/liro/parameters/adr` = `1129582592` (`0x43541000` dari perspektif ARM7)
- **Mapping Physical Linux:** `0x83541000`
- **Rumus Translasi:**
  - `Linux Phys = ARM7 Phys + 0x40000000`
  - `0x43541000 + 0x40000000 = 0x83541000`
- **Firmware ISP Image:** `av-cam.bin` (ukuran 11,264,692 bytes) dimuat tepat di alamat `0x83541000`.

---

## 4. Alamat Gerbang Kapabilitas & Patch Live RAM
Ketiga fungsi gerbang kapabilitas (*capability gates*) di ISP firmware yang diverifikasi:

| Target | Nama Simbol / Fungsi | Alamat Fisik RAM (Linux) | Byte Asli | Byte Patch (Bypass) |
|---|---|---|---|---|
| **Target 1** | `ExGammaTableOn` gate | `0x83813dc4` | `70 b5 0d 46` | `00 20 70 47` (`movs r0, #0; bx lr`) |
| **Target 2** | `ul_ExGammaTablePointer` gate | `0x83813dfc` | `70 b5 0e 46` | `00 20 70 47` (`movs r0, #0; bx lr`) |
| **Target 3** | `PictureProfile` gate | `0x838119d0` | `70 b5 0e 46` | `00 20 70 47` (`movs r0, #0; bx lr`) |

### Context Struct DSP
- **Alamat Fisik Context Struct:** `0x83e67020`
- `context[0x140]` = PictureProfile slot ID
- `context[0x256]` = `ExGammaTableOn` flag (1 bit)
- `context[0x260]` = `ul_ExGammaTablePointer` (32 bit)

---

## 5. Alamat Simbol Native di `ScalarDaemon` (PID 149)
Dianalisis langsung dari ELF `libScalarDaemon.so` (1.7MB) dan `/proc/149/maps`:
- **Base Address PID 149:** `0x12e4f000`
- **Base Address Libc PID 149:** `0x04251000`
- **C1 Ctor `PictureProfileParameter::PictureProfileParameter()`:**
  - File offset: `0x117eb8`
  - Runtime Address (Thumb): `0x12e4f000 + 0x117eb8 + 1 = 0x12f66eb9`
- **`setPictureProfile(PictureProfileParameter*)`:**
  - File offset: `0x118248`
  - Runtime Address (Thumb): `0x12e4f000 + 0x118248 + 1 = 0x12f67249`
- **`resetPictureProfile()`:**
  - File offset: `0x118830`
- **`setExtendedGammaTable(int, int, bool, bool)`:**
  - File offset: `0x118ada`
- **Konstanta Gamma ID:**
  - `VALUEID_PP_GAMMA_SLOG2` = `1165` (`0x48d`)
  - `VALUEID_PP_GAMMA_STANDARD` = `1160` (`0x488`)

---

## 6. Struktur Direktori Backup Ini
Folder ini berisikan seluruh aset kerja hasil riset hari ini:
```
a5100-slog2-backup-20261107/
├── README_FINDINGS.md                 # Dokumen teknis komprehensif ini
├── camera_reverse_engineering_core/   # [BEDAH TOTAL DALEMAN KAMERA] Analisis biner ISP & Android HAL
│   ├── libObj.so                      # 16.3MB Shared object inti pipeline ISP kamera
│   ├── libObj.rea                     # Workspace project REA (Reverse Engineer Anything) untuk libObj
│   ├── libcamera.so                   # Driver native Android Camera HAL
│   ├── cameraex.dex                   # Framework Java Sony CameraEx yang di-deodex
│   ├── sony.cameraex.odex             # ODEX asli dari filesystem kamera
│   ├── libScalarDaemon.snapshot       # Snapshot pemetaan 9.352 fungsi & segmen Ghidra 12.1.4
│   ├── libcamera.snapshot             # Snapshot pemetaan fungsi & segmen Camera HAL Ghidra
│   ├── ghidra_scripts/                # Skrip otomatisasi Ghidra (decomp_pp.py, find_pp_refs.py)
│   ├── dex_analysis/                  # Dump string, method, dan field Sony Camera framework
│   └── device_specs/                  # devices.yml & apps.yaml spesifikasi arsitektur Sony
├── openclaw_research_and_memory/      # [DARI AWAL] Riset & bedah firmware OpenClaw
│   ├── openclaw-firmware-report-20261102.md  # Bedah firmware v3.10 Sony resmi pertama kali
│   ├── openclaw-re-report-20261102.md        # RE Ghidra 12.1.4: libScalarDaemon, libObj, av-cam
│   ├── 2026-11-07.md                         # Log sesi harian: penemuan enum 0x480 & gate statis
│   ├── 2026-11-03.md                         # Log investigasi awal Android HAL
│   └── MEMORY.md                             # Arsip memori persistent OpenClaw
├── kimi_strategy/                     # [SARAN KIMI] Panduan pivot strategi
│   └── kimi-lut-strategy.md                  # In-Place LUT Hijack via /dev/mem (0xc0000000)
├── historical_analysis/               # Dokumen analisis native & handoff lama
│   ├── ANALISIS-NATIVE.md                    # Analisis mendalam fungsi native & parameter wire
│   ├── RE-GAMMA-CHAIN.md                     # Rantai gamma dari HAL ke BIONZ X
│   ├── STATUS-HANDOFF.md                     # Status handoff sesi sebelumnya
│   └── gammatest_STATUS.md                   # Catatan uji coba APK GT10/GT20
├── sources/                           # Seluruh source code C & Python terbaru
│   ├── patch_avcam.c                  # Patcher live RAM /dev/mem 3 gerbang kapabilitas
│   ├── find_avcam.c                   # Scanner RAM untuk mencari av-cam.bin
│   ├── slog2_direct.c                 # Injektor ptrace RPC setPictureProfile(S-Log2)
│   ├── slog2_live_clean.c             # Versi alternatif ptrace
│   ├── slog2_ptrace.c                 # Driver ptrace dasar
│   ├── slog2_rpc.c                    # Implementasi RPC mmap + call
│   ├── stub_glibc.c                   # Stubs runtime glibc untuk standalone ARM toolchain
│   ├── cam_cmd.py                     # Python telnet executor untuk komunikasi kamera
│   └── memdiff_lut.py                 # Tool visualisasi perbedaan kurva memori
├── binaries_arm/                      # Binary ELF ARMv7 siap jalan di kamera
│   ├── patch_avcam                    # Menembus 3 gerbang kapabilitas via /dev/mem
│   ├── slog2_direct                   # Menembak S-Log2 via PID 149 ScalarDaemon
│   ├── find_avcam                     # Scanner signature av-cam
│   └── stub_libc.so.6                 # Dynamic runtime helper
├── camera_firmware_and_drivers/       # Dump firmware & driver kernel dari kamera
│   ├── av-cam.bin                     # 11.2MB Firmware lengkap ISP Co-Processor
│   ├── liro.ko                        # Kernel driver bridge Linux <-> ITRON
│   ├── liro_syms.txt                  # Simbol exported liro.ko
│   ├── backup.ko                      # Kernel driver EEPROM / property manager
│   ├── backup_syms.txt                # Simbol exported backup.ko
│   ├── Backup.bin                     # Dump full EEPROM (855KB)
│   ├── avcam_hdr.bin                  # Header 4KB firmware ISP
│   └── slog2_curve.bin                # 1024-point 16-bit uint LUT S-Log2
├── camera_native_libs/                # Shared libraries native kamera (dengan simbol lengkap)
│   ├── libScalarDaemon.so             # 1.7MB Binary ScalarDaemon (berisi semua fungsi kamera)
│   ├── libSysDef.so                   # Library definisi sistem Sony
│   ├── libbackup.so                   # Native EEPROM accessor
│   ├── libBizFw.so, libBizFw2.so      # Sony Business Framework libraries
│   ├── libMWF.so                      # Media Middleware Framework
│   ├── libsdcom.so                    # Scalar Daemon Communication
│   └── cmnview*.so                    # View model libraries
├── mem_dumps/                         # Dump RAM fisik dari kamera
│   ├── ctx_page.bin                   # Halaman memori context ISP
│   ├── exgamma_page.bin               # Halaman gerbang Extended Gamma
│   ├── dump_baseline.bin              # Baseline RAM saat idle
│   ├── dump_changed.bin               # Perubahan RAM setelah parameter diubah
│   └── uipc_dump.bin                  # IPC shared memory ring buffer
└── test_clips/                        # Sample clip video yang direkam
    └── C0107_chunk1.bin               # Header XAVC/MP4 clip C0107
```

---

## 7. Prosedur Eksekusi Ulang dari Nol (Quick Start Pasca-Reboot)
Karena semua patch berada di **RAM (in-memory)**, kamera akan kembali 100% stock pabrik setiap kali dimatikan/reboot. Untuk mengaktifkan kembali S-Log2 setelah reboot:

### Langkah 1: Hubungkan Kamera ke WiFi & Telnet
Pastikan kamera online (misal di `192.168.68.114`).

### Langkah 2: Unggah Binary ke Kamera (jika `/tmp` bersih)
```bash
python3 /Users/rudipradnyana/a5100-re/cam_cmd.py "busybox ftpget -u cam -p cam -P 2121 192.168.68.110 /tmp/patch_avcam patch_avcam && chmod +x /tmp/patch_avcam"
python3 /Users/rudipradnyana/a5100-re/cam_cmd.py "busybox ftpget -u cam -p cam -P 2121 192.168.68.110 /tmp/slog2_direct slog2_direct && chmod +x /tmp/slog2_direct"
```

### Langkah 3: Jalankan RAM Patcher & S-Log2 Injector
```bash
python3 /Users/rudipradnyana/a5100-re/cam_cmd.py "/tmp/patch_avcam && /tmp/slog2_direct"
```
**Output sukses:**
- `patch_avcam`: `[+++] ALL SONY A5100 S-LOG2 / PICTURE PROFILE CAPABILITY GATES SUCCESSFULLY PATCHED IN RAM!`
- `slog2_direct`: `* SUCCESS: setPictureProfile(S-Log2) RETURNED 0! *`

---

## 8. Langkah Verifikasi Terakhir (Next Steps)
1. **Verifikasi Visual LCD:** Amati layar LCD kamera saat `slog2_direct` dieksekusi — bayangan hitam (*blacks*) akan terangkat (*lifted grey* / *flat profile* khas S-Log2).
2. **Pengambilan File Video:**
   - Rekam video 3–5 detik.
   - Pindahkan file `.MP4` dari `/android/mnt/sdcard/PRIVATE/M4ROOT/CLIP/` (bisa lewat card reader di Mac agar super cepat 100MB/s, atau via FTP chunk).
3. **Analisa Histogram (ffmpeg signalstats):**
   ```bash
   ffmpeg -i <CLIP>.MP4 -vf signalstats -f null - 2>&1 | grep YMIN
   ```
   **Kriteria Keberhasilan Mutlak:** `YMIN >= 90` (IRE ~10% black floor lift untuk S-Log2 standar).

