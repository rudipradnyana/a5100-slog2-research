# CONTRIBUTING

Terima kasih mau ikut mengerjakan! Proyek ini dokumentasi-first: **dead end
pentingnya sama dengan temuan**, jadi PR yang menambah tabel EXPERIMENTS.md
selalu welcome walau hasilnya "tidak berhasil".

## Aturan mutlak

1. **RAM-only.** Tidak ada patch flash, tidak ada firmware updater, tidak ada
   tulis EEPROM. Proposal yang menyentuh flash → ditolak.
2. **Bukti fisik.** `return 0`/ACK bukan bukti. Klaim fitur aktif hanya dengan:
   perubahan LCD yang bisa difoto, atau histogram clip (`YMIN ≥ 90` target).
3. **Jangan dump region `0xf1000000`.** 2× kamera wedge terkonfirmasi.
   (Detail: EXPERIMENTS.md B6.)
4. **Uji di kamera milik sendiri.** Semua risiko tanggung sendiri.
5. **Jangan commit firmware Sony / library proprietary.** Lihat .gitignore.
   Kontributor ekstrak sendiri dari firmware resmi (REPRODUCE.md §awal).

## Protokol eksperimen

- Nama file dump: `dump_<alamat-fisik>_<state>.bin` (contoh:
  `dump_0x82df9000_c-3.bin`).
- Mulai tiap sesi dengan A/A stability check: dua dump kondisi sama harus
  `TOTAL diffs=0`.
- Catat: IP kamera, IP Mac, waktu, level baterai, posisi kontras.
- Ubah setting satu-satu (-3 → 0 → +3), jangan lompat.
- Hasil (termasuk negatif) → PR ke `docs/EXPERIMENTS.md` dengan format tabel
  yang sudah ada.

## Build

```bash
# tool ARM (lihat REPRODUCE.md §6 untuk pitfall)
xcrun clang -target armv7-none-linux-eabi -mcpu=cortex-a9 -mthumb \
  -ffreestanding -fno-stack-protector -O2 -c <tool>.c -o <tool>.o
/opt/homebrew/opt/lld/bin/ld.lld --fix-cortex-a8 -static --entry _start \
  -o <tool> <tool>.o

# APK GT-series (lihat STATUS-HANDOFF di docs/legacy untuk recipe lengkap)
```

## Gaya dokumen

- Bahasa: Indonesia atau Inggris sama-sama diterima; istilah teknis tetap Inggris.
- Setiap klaim teknis harap tautkan sumbernya (alamat + file + tanggal sesi).
- Kalau mengoreksi dokumen lama, jangan hapus — tambahkan koreksi bertanggal
  (contoh pola koreksi di MASTER-TIMELINE.md).

## Ide PR yang paling dibutuhkan saat ini

Lihat `docs/NEXT-STEPS.md` — Prioritas 1 (protokol A==C) bisa dikerjakan
siapa pun yang punya kamera + 30 menit.
