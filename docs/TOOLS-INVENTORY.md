# Tools Inventory — Semua Tool yang Dipakai

> Kategori: (H) host Mac, (C) berjalan di kamera via telnet, (A) app Android di kamera.
> Semua source ada di repo ini kecuali disebut lain.

---

## 1. Komunikasi kamera

| Tool | Jenis | Fungsi | Source |
|---|---|---|---|
| **Telnet Tweak** (OpenMemories) | app di kamera | Buka telnetd port 23 tiap boot — pintu masuk utama. Tweak bawaan OpenMemories: Tweak → telnet enable. | bukan repo kita (prasyarat) |
| **cam_cmd.py** | (H) Python | Eksekusi perintah telnet + auto-wait prompt `/ # `. Usage: `python3 cam_cmd.py IP=192.168.68.104 "uname -a" 20`. | `host-tools/cam_cmd.py` |
| **FTP server (pyftpdlib)** | (H) Python | Penerima file dari kamera. Login cam/cam, passive 60000–60099, port 2121. Jalankan: `python3 ftp_server.py` (background). Kamera push via `busybox ftpput -u cam -p cam -P 2121 <MAC_IP> <remote> <local>`. | `host-tools/ftp_server.py` |
| **busybox ftpget/ftpput** | (C) builtin | Transfer biner dua arah. **Sering gagal utk file besar** (1.2 MB putus) — solusi: chunk 4 KB (`dd bs=4096 skip=N count=1` + ftpput per chunk, reassembly di Mac). | — |
| **ADB over TCP (5555)** | ❌ DEAD END | Port terbuka tapi `adb shell` selalu `error: closed` — adbd stub pabrik, bukan jalur masuk. | — |

## 2. Dump & analisis memori (dibuat hari ini, 2026-11-09)

| Tool | Jenis | Fungsi | Source |
|---|---|---|---|
| **memdump** | (C) ARM static | Dump RAM fisik via `mmap2` + `/dev/mem`. Usage: `/tmp/memdump 0x82df9000 0x7C000 /tmp/out.bin`. **Kenapa dd tidak cukup:** dd `skip=` pakai off_t 32-bit kernel → max 2 GB. mmap2 offset dalam satuan halaman → bisa ke mana pun. | `camera-tools/src/memdump.c` |
| **memdiff** | (C) ARM static | Diff dua file DI KAMERA, output `DIFF start=0x… len=…` (gap<64 digabung). 508 KB ≈ 1 detik. Menghindari FTP file besar. | `camera-tools/src/memdiff.c` |
| **memdiff_lut.py** | (H) Python | Diff + scan struktur LUT (monotonik u16/12-bit, 1024/2048 entri, 8-bit run) untuk dump yang sudah di Mac. | `host-tools/memdiff_lut.py` |
| **lut_dump.sh** | (H) shell | Helper protokol dump lengkap: alamat di nama file + verifikasi ukuran. | `host-tools/lut_dump.sh` |

**Kenapa freestanding (no libc):** `native/libc.so` kamera punya symbol table
rusak (`invalid sh_info`) → ld.lld menolak link dinamis. Solusi: syscall wrapper
ARM EABI langsung (`svc #0`, nomor syscall di **r7** — bug pertama yang
ditemukan), `naked _start` yang baca argc/argv dari stack (`ldr r0,[sp]`,
`add r1,sp,#4` — bug kedua).

**Toolchain ARM yang terbukti (tanpa NDK/gcc):**
```bash
xcrun clang -target armv7-none-linux-eabi -mcpu=cortex-a9 -mthumb \
  -ffreestanding -fno-stack-protector -O2 -c tool.c -o tool.o
/opt/homebrew/opt/lld/bin/ld.lld --fix-cortex-a8 -static --entry _start \
  -o tool tool.o
```
(Perlu `brew install lld` — Apple clang tidak bawa lld; Apple ld Mach-O-only.)

## 3. Patch & injeksi (dari 7 Nov)

| Tool | Jenis | Fungsi | Source |
|---|---|---|---|
| **patch_avcam** | (C) ARM dyn | Patch 3 capability gate di RAM `av-cam.bin` (via /dev/mem). Verifikasi byte asli dulu, baru tulis `00 20 70 47`. Output: `[+++] ALL ... GATES SUCCESSFULLY PATCHED`. **Efek hilang tiap reboot — jalankan ulang.** | `camera-tools/src/patch_avcam.c` |
| **find_avcam** | (C) ARM dyn | Scan RAM cari signature `av-cam.bin` → alamat load aktual. | `camera-tools/src/find_avcam.c` |
| **slog2_direct** | (C) ARM dyn | ptrace RPC ke ScalarDaemon (PID 149): instantiate `PictureProfileParameter`, set gamma, panggil `setPictureProfile()`. Return 0 = ACK IPC saja (nilai 1165 salah — lihat koreksi). | `camera-tools/src/slog2_direct.c` |
| **slog2_v2/v3/v4, _standalone, _rpc, _ptrace, _live_clean, _injector, _full** | (C) | Varian eksperimen jalur injeksi (perkembangan iteratif). slog2_v4 = versi terakhir 7 Nov. | `camera-tools/src/` |
| **GT21 / GT21r** | (A) APK | Probe-only: load libScalarDaemon, buka CameraEx still mode, enumerasi `SystemEventIssuerCamera` + `PictureProfileParameter`, log semua ke `/sdcard/GAMMALOG.TXT`. **TIDAK memanggil setPictureProfile. Belum pernah dijalankan.** | `camera-tools/android-apps/gt21/` |
| **GT22** | (A) APK | `injectSLog2(10)`: panggil native setPictureProfile dgn struct 0x160B (gamma@+0x88=10, flag@+0x8c=1). **Belum pernah dijalankan.** | `camera-tools/android-apps/gt22/` |

## 4. Analisis statis (host)

| Tool | Fungsi |
|---|---|
| **Ghidra 12.1.4** + decompiler mac_arm_64 (dari 12.2) + JDK 25+ | Bedah libObj.so/libScalarDaemon.so/libcamera.so/av-cam.bin. rea menolak 12.2. |
| **rea (Reverse Engineer Anything)** | Otomatisasi Ghidra; snapshot: libScalarDaemon.snapshot (9352 fungsi), libcamera.snapshot. |
| **capstone 5.0.7** (pip) | Disasm Thumb-2 manual (Apple objdump tak support force-thumb). |
| **pyelftools** | ELF parsing: PT_LOAD konversi VMA↔file offset (VMA==offset utk segmen LOAD pertama semua lib). |
| **fwtool.py** (ma1co) | Unpack firmware resmi .dmg → partisi nflasha*. |
| **pmca-console** | Install APK ke kamera via USB (MTP). Butuh baterai ≥50% (error "505 Low battery"). |
| **d8/aapt/apksigner** (android-14 build-tools) + **jbr-17 javac** + **android-2.2 android.jar** | Build GT-series APK (recipe di STATUS-HANDOFF.md). |
| **ffmpeg signalstats** | Verifikasi histogram clip: `ffmpeg -i clip.MP4 -vf signalstats -f null - 2>&1 \| grep YMIN`. Target: YMIN ≥ 90. |
| **Ghidra scripts** | decomp_pp.py, find_pp_refs.py (camera_reverse_engineering_core/ghidra_scripts di backup). |

## 5. Yang TIDAK dipakai (dan kenapa)

- **OpenMemories Tweak lainnya** — hanya ubah setting, tidak sentuh gamma.
- **Toko PMCA Sony** — mati (503), tapi tak dibutuhkan (USB install works).
- **Sony SDK resmi** — tidak expose PP/gamma.
- **Sony firmware updater** — hanya sentuh partisi setting/backup, bukan
  firmware kamera (dan signed — jalur update tertutup).
- **Magic Lantern-style flash patch** — firmware signed; dan kita tidak mau
  brick risk. RAM-only adalah pilihan desain.

## 6. Kenapa semua tool harus dibuat sendiri (existing solutions preflight)

- Tidak ada tool ARM freestanding untuk kamera ini di publik (A5100 = arsitektur
  CXD90014, komunitas kecil).
- BusyBox kamera TIDAK punya: tail, wc, base64, timeout, md5sum (ada di luar
  /bin tapi tak selalu), devmem (ada tapi bisa hang utk region tertentu).
- dd kernel ini: off_t 32-bit → max 2 GB offset.
- pyftpdlib dipilih karena FTP server bawaan kamera read-only dan tidak bisa
  diandalkan untuk push; busybox ftpput memang bekerja ke server eksternal.
