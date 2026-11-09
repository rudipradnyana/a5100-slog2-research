# REPRODUCE, Menyiapkan & Menjalankan dari Nol

> Dokumen ini untuk kontributor yang punya A5100 sendiri dan mau ulang semua eksperimen. Tidak ada langkah yang mengubah flash.

---

## 0. Prasyarat

**Hardware:**
- Sony A5100 (firmware 3.10, versi lain belum dites), baterai penuh/≥50%
- SD card terpasang (harus ada untuk mode kamera; jangan diformat oleh eksperimen)
- Mac/Linux host di WiFi yang sama
- Kabel USB data (USB-A ke micro-USB) untuk install APK via PMCA, opsional
  untuk fase lanjutan

**Host tools:**
- Python 3 dengan `pyftpdlib` (`pip install pyftpdlib`)
- `ffmpeg` (verifikasi clip)
- Untuk build ARM: `clang` (Xcode CLT) + `brew install lld`
- Untuk analisis statis: Ghidra 12.1.x + JDK 25+ (opsional, fase lanjutan)
- `adb` (opsional, untuk membuktikan sendiri bahwa shell ditolak, §dead-end)

**Kamera, aktifkan pintu masuk:**
1. Kamera ke WiFi rumah (menu → WLAN).
2. Install **OpenMemories: Tweak** (app PMCA), satu-satunya cara legal buka
   telnet. Sumber: https://github.com/ma1co/OpenMemories-Tweak (bukan repo ini).
3. Di Tweak: enable **telnet**. **Ini harus diulang tiap boot kamera.**
4. Cek IP kamera di layar Tweak (bisa berubah-ubah via DHCP, hari ini .104,
   kemarin .117).

---

## 1. Verifikasi koneksi

```bash
CAM_IP=192.168.68.104      # ganti sesuai layar Tweak
MAC_IP=192.168.68.113      # IP Mac di WiFi sama

# FTP server di Mac (untuk push/pull file)
cd host-tools && python3 ftp_server.py &

# telnet shell
python3 cam_cmd.py IP=$CAM_IP "uname -a" 15
# Harus keluar: Linux localhost 3.0.27_nl-rt106 ... armv7l
```

## 2. Upload tool ARM ke kamera

```bash
python3 cam_cmd.py IP=$CAM_IP \
  "busybox ftpget -u cam -p cam -P 2121 $MAC_IP /tmp/memdump memdump && chmod +x /tmp/memdump"
python3 cam_cmd.py IP=$CAM_IP \
  "busybox ftpget -u cam -p cam -P 2121 $MAC_IP /tmp/memdiff memdiff && chmod +x /tmp/memdiff"
# build sendiri: camera-tools/src/memdump.c + memdiff.c (lihat §6)
```

## 3. Patch capability gates (RAM, hilang tiap reboot)

```bash
python3 cam_cmd.py IP=$CAM_IP \
  "busybox ftpget -u cam -p cam -P 2121 $MAC_IP /tmp/patch_avcam patch_avcam && chmod +x /tmp/patch_avcam && /tmp/patch_avcam"
# Harapkan: [+++] ALL SONY A5100 S-LOG2 / PICTURE PROFILE CAPABILITY GATES
#           SUCCESSFULLY PATCHED IN RAM!
```
Catat: patch ini ≠ S-Log2 aktif. Ini hanya membuka gate; efek akhirnya belum
terbukti (lihat EXPERIMENTS.md B1, NEXT-STEPS.md P4).

## 4. Dump & diff memori (protokol benar)

```bash
# A/A stability check dulu (WAJIB):
python3 cam_cmd.py IP=$CAM_IP "/tmp/memdump 0x82df9000 0x7C000 /tmp/A.bin"
python3 cam_cmd.py IP=$CAM_IP "/tmp/memdump 0x82df9000 0x7C000 /tmp/A2.bin"
python3 cam_cmd.py IP=$CAM_IP "/tmp/memdiff /tmp/A.bin /tmp/A2.bin"
# Harus: TOTAL diffs=0

# Diff kontras (-3 vs +3), minta orang lain ubah setting di menu kamera,
# atau pakai remote control:
#   (kamera: Contrast -3) → dump B
#   (kamera: Contrast +3) → dump C
python3 cam_cmd.py IP=$CAM_IP "/tmp/memdump 0x82df9000 0x7C000 /tmp/B.bin"
# ... ubah kontras ...
python3 cam_cmd.py IP=$CAM_IP "/tmp/memdump 0x82df9000 0x7C000 /tmp/C.bin"
python3 cam_cmd.py IP=$CAM_IP "/tmp/memdiff /tmp/B.bin /tmp/C.bin"
```

## 5. Dump heap daemon (belum dipakai, area kandidat P2a)

```bash
# /proc/149/mem: dd bs=4096 skip=N count=M di sini AMAN (file offset, bukan /dev/mem)
# heap = vm 0x16000..0x420000 → page (0x16000/4096)=22 s.d. (0x420000/4096)=1056
python3 cam_cmd.py IP=$CAM_IP \
  "busybox dd if=/proc/149/mem of=/tmp/heap.bin bs=4096 skip=22 count=1034"
# lalu ftpput chunked atau memdiff dua kondisi on-camera
```

## 6. Build tool ARM sendiri

```bash
cd camera-tools/src
xcrun clang -target armv7-none-linux-eabi -mcpu=cortex-a9 -mthumb \
  -ffreestanding -fno-stack-protector -O2 -c memdump.c -o memdump.o
/opt/homebrew/opt/lld/bin/ld.lld --fix-cortex-a8 -static --entry _start \
  -o memdump memdump.o
file memdump   # ELF 32-bit LSB executable, ARM, statically linked
```
Pitfall yang sudah terjadi (jangan ulang):
- libc.so kamera **rusak buat lld** (`invalid sh_info`) → freestanding only.
- ARM EABI: nomor syscall di **r7**, wrapper tanpa r7 = EINVAL misterius.
- `_start` harus `naked`; argc/argv dari stack (`ldr r0,[sp]`), bukan register.

## 7. Menarik file besar dari kamera (chunked, anti-putus)

```bash
# di kamera: pecah 4KB per chunk lalu push satu-satu
for i in $(seq 0 123); do
  python3 cam_cmd.py IP=$CAM_IP \
    "busybox dd if=/tmp/big.bin of=/tmp/cc bs=4096 skip=$i count=1 2>/dev/null; \
     busybox ftpput -u cam -p cam -P 2121 $MAC_IP cc_$i /tmp/cc"
done
# di Mac: cat cc_0 cc_1 ... cc_123 > big.bin  (urutan NUMERIK, bukan leksikografis!)
python3 -c "import glob;print(sorted(glob.glob('cc_*'),key=lambda p:int(p.split('_')[-1])))"
```
Catatan: `/tmp` kamera tmpfs kecil, hapus chunk setelah dipakai
(`rm /tmp/cc /tmp/cc_*`), dan hati-hati "No space left on device".

## 8. Verifikasi clip video (target akhir)

```bash
# rekam 3-5 detik di kamera (mode movie), lalu tarik clip:
# /android/mnt/sdcard/PRIVATE/M4ROOT/CLIP/C####.MP4  (via FTP chunked atau card reader)
ffmpeg -i C0107.MP4 -vf signalstats -f null - 2>&1 | grep YMIN
```
**Kriteria:** `YMIN ≥ 90` = black floor terangkat ~10% IRE = tanda kurva flat
S-Log2. Sampai hari ini: belum pernah tercapai via jalur software manapun.

## 9. Recovery

| Situasi | Aksi |
|---|---|
| Telnet connect tapi reset / diam | tunggu 30s, coba lagi; kalau tetap → power cycle |
| Ping hilang | power cycle kamera (wedge, biasanya setelah dump region terlarang) |
| `/tmp` penuh | `rm -f /tmp/cc* /tmp/chunk*` atau dump yang tidak perlu |
| ADB `error: closed` | memang tidak akan pernah jalan (adbd stub), bukan error kamu |
| Setting kontras ter-reset 0 | itu tanda crash-reboot; power-off bersih menyimpan setting |

## 10. Checklist kebersihan sebelum publish (untuk maintainer)

- [ ] Tidak ada firmware Sony (av-cam.bin, libObj.so, dll) di repo
- [ ] Tidak ada IP privat/email/username personal di source & docs
- [ ] Log sesi sudah difilter (lihat session-logs/2026-11-03.md, sudah dibersihkan)
- [ ] LICENSE ditambahkan
- [ ] EXPERIMENTS.md diperbarui dengan hasil terbaru
