# CONTRIBUTING

Terima kasih sudah mau ikut mengerjakan. Proyek ini dokumentasi dulu. Dead end sama nilainya dengan temuan. PR yang menambah tabel EXPERIMENTS.md selalu welcome, walau hasilnya tidak berhasil.

## Aturan mutlak

1. **RAM-only.** Tidak ada patch flash, tidak ada firmware updater, tidak ada tulis EEPROM. Proposal yang menyentuh flash ditolak.
2. **Bukti fisik.** `return 0` atau ACK bukan bukti. Kamu klaim fitur aktif hanya dengan perubahan LCD yang bisa kamu foto, atau histogram clip (`YMIN >= 90` target).
3. **Jangan dump region `0xf1000000`.** Dua wedge terkonfirmasi di region itu. Detail: EXPERIMENTS.md B6.
4. **Uji di kamera milik sendiri.** Kamu tanggung semua risiko sendiri.
5. **Jangan commit firmware Sony atau library proprietary.** Kamu lihat .gitignore. Kamu ekstrak sendiri dari firmware resmi. Kamu baca REPRODUCE.md bagian awal.

## Protokol eksperimen

- Nama file dump: `dump_<alamat-fisik>_<state>.bin`. Contoh: `dump_0x82df9000_c-3.bin`.
- Kamu mulai tiap sesi dengan A/A stability check. Dua dump kondisi sama harus `TOTAL diffs=0`.
- Kamu catat IP kamera, IP Mac, waktu, level baterai, posisi kontras.
- Kamu ubah setting satu per satu (-3 ke 0 ke +3). Jangan lompat.
- Kamu kirim hasil, termasuk hasil negatif, sebagai PR ke `docs/EXPERIMENTS.md` dengan format tabel yang sudah ada.

## Build

```bash
# tool ARM (kamu baca REPRODUCE.md bagian 6 untuk pitfall)
xcrun clang -target armv7-none-linux-eabi -mcpu=cortex-a9 -mthumb \
  -ffreestanding -fno-stack-protector -O2 -c <tool>.c -o <tool>.o
/opt/homebrew/opt/lld/bin/ld.lld --fix-cortex-a8 -static --entry _start \
  -o <tool> <tool>.o

# APK GT-series (kamu baca STATUS-HANDOFF di docs/legacy untuk recipe lengkap)
```

## Gaya dokumen

- Bahasa: Indonesia atau Inggris, dua-duanya diterima. Istilah teknis tetap Inggris.
- Kamu tautkan sumber tiap klaim teknis. Sumber berisi alamat, file, dan tanggal sesi.
- Kalau kamu koreksi dokumen lama, jangan hapus. Kamu tambah koreksi bertanggal. Kamu lihat pola koreksi di MASTER-TIMELINE.md.

## Ide PR yang paling dibutuhkan saat ini

Kamu baca `docs/NEXT-STEPS.md`. Prioritas 1 (protokol A==C) bisa kamu kerjakan dengan kamera dan 30 menit.
