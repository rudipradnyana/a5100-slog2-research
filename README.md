# Sony A5100 S-Log2 Research — Open Reverse Engineering Project

![status](https://img.shields.io/badge/status-WIP%20%7C%20video%20not%20yet%20achieved-orange)
![target](https://img.shields.io/badge/target-Sony%20ILCE--5100%20fw%203.10-blue)
![risk](https://img.shields.io/badge/risk-RAM%20only%2C%20zero%20brick-success)

> **Tujuan:** mengaktifkan kurva gamma **S-Log2** di kamera Sony A5100 (yang tidak
> punya Picture Profile) lewat reverse engineering firmware, tanpa memodifikasi
> flash — semua patch RAM-only, hilang sendiri saat reboot, nol risiko brick.
>
> **Status jujur:** arsitektur sudah terpetakan sampai akar, tiga capability gate
> firmware bisa di-patch live, nilai parameter S-Log2 sudah ditemukan (dual-confirmed
> static), tapi **satu clip video dengan kurva S-Log2 asli belum pernah tercapai**.
> Dokumen ini mendokumentasikan semuanya — termasuk semua dead end — supaya
> kontributor berikutnya tidak mengulang kesalahan yang sama.

---

## Kenapa proyek ini menarik

- **A5100 = kamera entry-level dengan ISP senior.** Firmware-nya memuat seluruh
  resource Picture Profile (termasuk string S-Log2), tapi jalur menampilkannya
  dihapus. Pertanyaannya bukan "apakah kamera bisa" tapi "di mana kuncinya".
- **Arsitektur dual-processor yang jarang dibedah publik:** Linux Cortex-A9
  (host, Android 2.3.7 HAL) + **ITRON RTOS di ARM7 sub-core** yang menjalankan
  ISP firmware `av-cam.bin` — dikomunikasikan lewat kernel driver `liro.ko`
  dengan skema translasi alamat `+0x40000000`.
- **Semua metodologi reusable** untuk kamera Sony arsitektur CXD90014 lain
  (A5000, A6000, NEX-6, dst).

## Peta dokumen

| Dokumen | Isi |
|---|---|
| [`docs/MASTER-TIMELINE.md`](docs/MASTER-TIMELINE.md) | **Mulai di sini.** Kronologi lengkap 2026-11-02 → 11-09, semua fase, semua koreksi |
| [`docs/EXPERIMENTS.md`](docs/EXPERIMENTS.md) | Tabel semua eksperimen + hasil + status, peta memori, dead-end list |
| [`docs/TOOLS-INVENTORY.md`](docs/TOOLS-INVENTORY.md) | Semua tool (host/kamera/app), kenapa dibuat sendiri, toolchain ARM yang works |
| [`docs/NEXT-STEPS.md`](docs/NEXT-STEPS.md) | Roadmap buat kontributor: urutan kerja berikutnya + protokol eksperimen |
| [`docs/REPRODUCE.md`](docs/REPRODUCE.md) | Step-by-step menyiapkan kamera & menjalankan tool dari nol |
| [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) | Arsitektur teknis: dual-core, liro.ko, rantai IPC, alamat penting |
| [`docs/session-logs/`](docs/session-logs/) | Log mentah per tanggal (sumber historis) |
| [`docs/legacy/`](docs/legacy/) | Dokumen fase awal (beberapa sudah terkoreksi — baca koreksinya) |
| [`docs/firmware-notes/`](docs/firmware-notes/) | Analisis native, rantai gamma, strategi LUT hijack |

## Struktur kode

```
host-tools/         Python & shell yang jalan di Mac (cam_cmd.py, ftp_server.py, ...)
camera-tools/
  src/              Source C tool yang jalan DI KAMERA (memdump, memdiff, patch_avcam, ...)
  prebuilt/         Binary ARMv7 statis/dinamis siap-upload
  android-apps/     GT21 (probe) & GT22 (inject) APK + source
curve/              slog2_curve.bin (2048 B, kurva S-Log2 publik)
firmware-notes/     Catatan bedah firmware
docs/               Dokumentasi (lihat peta di atas)
evidence/           Sampel data (header clip, dump referensi)
```

## Prinsip kerja (WAJIB dibaca sebelum menyentuh kamera)

1. **RAM-only.** Tidak ada tulisan ke flash. Kamera 100% stock setelah
   matikan/nyalakan. Kalau ada proposal patch flash — tolak di repo ini.
2. **Bukti fisik, bukan return code.** `return 0` dari IPC = pesan diterima,
   BUKAN fitur aktif. Satu-satunya bukti: perubahan pada LCD/histogram/clip.
   Target akhir yang disepakati: `ffmpeg -i clip.MP4 -vf signalstats` →
   **YMIN ≥ 90**.
3. **Protokol dump:** catat alamat fisik di nama file, dump A/A dulu (harus
   identik), baru diff kondisi berbeda. Dump antar-waktu tanpa kontrol = churn.
4. **Kamera gampang wedge.** Gejala: telnet connect→reset, ADB `error: closed`,
   ping hilang. Recovery: power cycle. **Region `0xf1000000` DILARANG di-dump**
   (2× wedge terkonfirmasi). Baterai ≥50%, jangan format SD.
5. **Dokumentasikan negatif.** Dead end sama bernilainya dengan temuan —
   lihat tabel EXPERIMENTS.md §E.

## Quick start (kamera sudah siap, lihat REPRODUCE.md untuk detail)

```bash
# di Mac
cd host-tools
python3 ftp_server.py &        # FTP utk transfer ke/dari kamera

# 1. patch capability gates di RAM (hilang tiap reboot — jalankan ulang)
python3 cam_cmd.py IP=<CAM_IP> "busybox ftpget -u cam -p cam -P 2121 <MAC_IP> /tmp/patch_avcam patch_avcam && chmod +x /tmp/patch_avcam && /tmp/patch_avcam"

# 2. dump & diff memori (tool freestanding)
python3 cam_cmd.py IP=<CAM_IP> "busybox ftpget ... /tmp/memdump memdump && /tmp/memdump 0x82df9000 0x7C000 /tmp/a.bin"

# 3. verifikasi clip (target mutlak)
ffmpeg -i C0107.MP4 -vf signalstats -f null - 2>&1 | grep YMIN   # harus >= 90
```

## Temuan inti (TL;DR teknis)

- ISP firmware `av-cam.bin` (11.2 MB, ITRON/ARM7) di-load di RAM fisik
  **0x83541000** oleh `liro.ko` (ARM7 addr + 0x40000000).
- Tiga capability gate (ExGammaTableOn `0x83813dc4`, ul_ExGammaTablePointer
  `0x83813dfc`, PictureProfile `0x838119d0`) bisa di-patch → `movs r0,#0; bx lr`.
- Nilai S-Log2: **0x480** (domain MWF) = **indeks 10** di kabel SD.
  Wire PP SET: `[0]=5, [3]=0x40, byte[0x1a]=indeks`. IPC param gamma = 0x238.
- `setExtendedGammaTable` (jalur Android) = Picture Effect **still** — dead end
  untuk video (bukti histogram identik).
- Kandidat parameter Contrast (scalar, bukan LUT): `0x82E4C40C`, u16
  `0x3CE5`(-3)↔`0x3D05`(+3), delta `0x20`/step — perlu protokol A==C.
- Buffer LUT aktif (target timpa in-place): **belum ketemu** — ini tugas
  kontributor #1 (lihat NEXT-STEPS.md).

## Lisensi & legalitas

- Kode & dokumentasi di repo ini: milik proyek (pilih lisensi open-source saat publish).
- **TIDAK menyertakan firmware Sony.** File seperti `av-cam.bin`, `libObj.so`,
  `libScalarDaemon.so` adalah milik Sony — jangan di-commit. Kontributor harus
  mengekstrak sendiri dari kamera/firmware resmi miliknya (lihat REPRODUCE.md).
- Cukup disertakan: alamat, offset, nilai konstanta, source code tulisan sendiri.
- Gunakan pada kamera milik sendiri, dengan risiko sendiri.

## Kredit

- Rudi Pradnyana (@rudipradnyana) — owner, kamera, arah riset
- Friday (OpenClaw) — analisis, tooling, dokumentasi
- Kimi — strategi in-place LUT overwrite
- Komunitas yang membuka jalan: Magic Lantern, OpenMemories/PMCA-RE
