# STATUS REVIEW 2026-11-09 (OpenClaw/Friday)
Review menyeluruh atas: README_FINDINGS.md, DOCUMENTATION_TECHNICAL_SPEC.md, STATUS-HANDOFF.md, ANALISIS-NATIVE.md, catatan harian 2026-11-06/07, source code, dan artefak di a5100-re.

## 1. Integritas backup
Folder backup 20261107 dan a5100-re utuh, tidak ada yang berubah sejak 8 Nov (file terakhir: TECH SPEC 17:12, copy final 17:04). Kamera sendiri offline sejak berakhirnya sesi 7-8 Nov.

## 2. Yang sudah TERBUKTI (jangan diulang)
- Arsitektur lengkap terpetakan: av-cam.bin (ISP, RAM 0x83541000), liro.ko bridge, ScalarDaemon PID 149, rantai HAL -> libObj -> libScalarDaemon.
- patch_avcam (RAM, /dev/mem): 3 gate di-patch live, byte asli terverifikasi sebelum tulis. Zero brick risk. Kembali stock tiap reboot.
- setExtendedGammaTable (GT10/GT20): ACK + round-trip byte-perfect, tapi TIDAK berpengaruh ke foto maupun video (histogram identik). Jalur ini DEAD END, jangan disentuh lagi.
- Analisis statis 7 Nov (dual-confirmed): nilai S-Log2 di kabel = INDEKS 10 (0x0A); nilai mentah MWF = 0x480. Gate "ForLiro NON SUPPORTED" ternyata salah MsgID, bukan blokir model. Jalur sah: ObjMsg 0x75AE/0x75E6 param 0x9685, atau native setPictureProfile dgn struct 0x160B (gamma @+0x88, flag @+0x8c, IPC param 0x238, cmd model=2 id=0x40).

## 3. Inkonsistensi dokumen (koreksi)
- README_FINDINGS.md dan TECH SPEC masih menulis VALUEID_PP_GAMMA_SLOG2 = 1165 (0x48d). Nilai itu SALAH menurut analisis 7 Nov jam 09:15 (0x480 raw / indeks 10 di kabel). slog2_direct.c memang pakai 1165, jadi "return 0 SUCCESS" yang dicatat README dieksekusi dengan nilai yang salah. ACK return 0 bukan bukti S-Log2 aktif.
- "Hasil: return 0 (SUCCESS)" harus dibaca sebagai "IPC menerima pesan", bukan "gamma aktif di ISP". Bukti fisik (LCD/histogram) sampai hari ini NOL di semua jalur software.
- Mem dumps LUT-hijack: dump_changed.bin, dump_contrast.bin, dump_check_idle.bin ternyata identik byte-per-byte (MD5 sama). Artinya diff 974 region vs baseline adalah churn RAM/liveview antar waktu, BUKAN efek perubahan Contrast. Alamat sumber dump juga tidak tercatat konsisten (kimi playbook bilang 0xc0000000 count 0x98000, file nyata 0x12C000 byte, memdiff pakai base 0x92ec0000). Scan struktur LUT (monotonik 12-bit u16, 1024/2048 entri) di kedua dump: NOL kandidat. Kesimpulan: Langkah 0 strategi Kimi (lacak LUT hardware) BELUM berhasil dan harus diulang dengan protokol benar.

## 4. Peta status jalur
| Jalur | Status |
|---|---|
| patch_avcam (gate RAM) | Berjalan, tapi efektivitas belum terbukti |
| setExtendedGammaTable (GT10/GT20) | Dead end, terbukti empiris |
| slog2_direct ptrace (gamma 1165) | ACK saja, nilai salah, tanpa efek |
| GT21 (probe-only) | Dibangun, belum ada catatan run |
| GT22 (native inject gamma=10) | Dibangun 7 Nov 12:01, belum ada catatan run |
| LUT hijack Kimi (/dev/mem) | Langkah 0 gagal, dump salah/berantakan |
| Target akhir: clip video S-Log2 + YMIN >= 90 | BELUM pernah tercapai |

## 5. Langkah berikutnya (urutan eksekusi saat kamera online)
1. Siapkan host: catat IP Mac terkini (192.168.68.113 sekarang, dulu .115), jalankan ulang FTP server (launchd com.rudi.cameraftp exit 2, port 2121 mati), cek baterai kamera minimal 50%.
2. Install GT21.apk (pmca USB MTP) -> run -> baca GAMMALOG.TXT. Ini probe-only, aman.
3. Kalau probe OK: install GT22.apk -> run injectSLog2(10). Amati LCD, lalu tangkap log daemon/ISP (dmesg, /proc/kmsg, cari "LiroSeq ParamCheckErr") untuk melihat apakah gate masih menolak.
4. Kalau ACK + ada efek: rekam 3 detik, tarik clip, verifikasi `ffmpeg signalstats` YMIN >= 90. Baru klaim menang.
5. Kalau GT22 juga tanpa efek: kembali ke jalur LUT hijack dengan protokol benar: dump region sama dua kali dalam hitungan detik (Contrast -3 vs +3), catat alamat fisik dump di nama file, scan ulang (termasuk format 8/10-bit dan tabel per-channel 256 entri), uji reaktivitas devmem sebelum timpa.
6. Fallback terakhir GT22: variant gamma=0x480 mentah (source slog2_v2.c sudah siap dua-duanya).

## 6. Catatan kecil
- cam_cmd.py masih hardcode 192.168.68.114; IP kamera bisa berubah, cek di Tweak saat kamera nyala.
- GT21r.apk (11:23) lebih baru dari GT21.apk (09:12); pakai GT21r bila keduanya ada.
- Tidak ada bukti GT21/GT22 pernah dijalankan di kamera; ikon launcher tidak bisa dijadikan patokan, identifikasi run dari banner "vN start" di GAMMALOG.TXT.
