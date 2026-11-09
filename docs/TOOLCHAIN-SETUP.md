# TOOLCHAIN SETUP — Instalasi dari Nol (Host Mac)

> Semua tool pihak ketiga yang dipakai proyek ini, dengan versi persis yang
> terbukti bekerja + jebakan instalasinya. Tool-tool ini TIDAK disertakan di
> repo (lisensi & ukuran) — setiap kontributor install sendiri.

---

## 1. Ringkasan cepat (copy-paste friendly)

```bash
# Core
xcode-select --install                # Apple clang + SDK
brew install lld                      # ELF linker (Apple ld = Mach-O only!)
brew install openjdk                  # JDK 25+ untuk Ghidra (21 DITOLAK)
brew install ffmpeg                   # verifikasi histogram clip
pip3 install pyftpdlib capstone pyelftools numpy
npm install -g rea-agents             # REA v6.7.0 (CLI RE otomatis)

# Ghidra 12.1.4 — BUKAN 12.2 (lihat §2)
# download: https://github.com/NationalSecurityAgency/ghidra/releases
# tar ke ~/tools/ghidra_12.1.4_PUBLIC

# Java untuk build APK (javac 1.8 target)
# JBR-17 dipakai: /Library/Java/JavaVirtualMachines/jbr-17.0.14 (dari Android Studio)

# Android build tools (aapt, d8, apksigner)
# sdkmanager "build-tools;34.0.0" atau copy dari repo backup lokal
```

## 2. Ghidra 12.1.4 + jebakan mac_arm_64

**Versi adalah penting:** REA 6.7.0 menolak Ghidra 12.2 (hanya menerima 12.1.x).
Tapi rilis Ghidra 12.1.4 untuk macOS **tidak menyertakan binary decompiler
native `mac_arm_64`**. Fix yang dipakai (transplant dari 12.2):

```bash
# 1. install kedua versi sementara
brew install ghidra          # ini 12.2.x — hanya untuk mencuri decompilernya
# atau download manual kedua rilis dari GitHub NSA/ghidra releases

# 2. salin binary decompiler 12.2 → 12.1.4
cp <ghidra-12.2>/Ghidra/Features/Decompiler/build/os/mac_arm_64/decompile \
   ~/tools/ghidra_12.1.4_PUBLIC/Ghidra/Features/Decompiler/build/os/mac_arm_64/

# 3. uninstall 12.2 kalau mau bersih
```

Env:
```bash
export GHIDRA_INSTALL_DIR=~/tools/ghidra_12.1.4_PUBLIC
export JAVA_HOME=/opt/homebrew/opt/openjdk   # JDK 25+ wajib
```

Verifikasi: `rea` harus bisa membuka project Ghidra tanpa error
`application.java.min` dan tanpa "decompiler missing".

## 3. REA (rea-agents 6.7.0)

```bash
npm install -g rea-agents     # symlink: /opt/homebrew/bin/rea
rea --version                 # 6.7.0
```

- Fungsi dalam proyek ini: import otomatis ELF ke Ghidra headless, inventory
  fungsi/segment/string, pencarian symbol by regex — output JSON "evidence
  bundle" yang jadi dasar laporan `docs/firmware-notes/`.
- Snapshot hasil analisis tersedia di `tools/rea-snapshots/` (libScalarDaemon:
  9.352 prosedur, 13.145 string; libcamera; libObj.rea) — kontributor bisa
  mulai dari alamat yang sudah diketahui tanpa re-analisis penuh.

## 4. Toolchain C untuk kamera (freestanding ARM — PENTING)

`libc.so` kamera **rusak buat lld modern** (`invalid sh_info` in symbol table)
→ link dinamis menolak. Semua tool kita statis + freestanding:

```bash
xcrun clang -target armv7-none-linux-eabi -mcpu=cortex-a9 -mthumb \
  -ffreestanding -fno-stack-protector -O2 -c tool.c -o tool.o
/opt/homebrew/opt/lld/bin/ld.lld --fix-cortex-a8 -static --entry _start \
  -o tool tool.o
```

Tiga pitfall yang sudah mahal dipelajari:
1. **Apple ld tidak bisa** — Mach-O only, butuh `ld.lld` (brew install lld).
2. **ARM EABI: nomor syscall di r7** — syscall wrapper tanpa set r7 = EINVAL
   misterius (mmap2 "gagal" padahal argumen benar).
3. **`_start` harus `naked`** — argc/argv dibaca dari stack (`ldr r0,[sp]`,
   `add r1,sp,#4`) via prolog eksplisit; kalau compiler buat prolog sendiri
   sebelum inline-asm, stack sudah bergeser → argv kacau.

## 5. Android build tools (APK GT-series)

Sumber yang dipakai: `~/tools/android-14/` (build-tools: aapt, d8, apksigner,
zipalign) + `android-2.2_r03-linux/android.jar` (froyo — kamera jalan di
Android 2.3.7, API lama!) + JBR-17 untuk javac (`-source 1.8 -target 1.8`).

Alternatif resmi: `sdkmanager "build-tools;34.0.0"` — aapt/d8/apksigner
setara. android.jar froyo bisa dari repo `Sable/android-platforms` di GitHub.

Recipe lengkap build+sign: `docs/legacy/STATUS-HANDOFF.md` §"Build recipe GT21".
Keystore: `tools/keystore/gammatest-debug.keystore` (pass: android).

## 6. PMCA (install APK via USB)

- Repo: https://github.com/ma1co/pmca-re (57 MB di host; clone sendiri)
- Dipakai untuk: install OpenMemories: Tweak + GT-series ke kamera via USB MTP
- Butuh: `devices.yml` + `apps.yaml` (ada di `tools/pmca-config/`) — bukti
  `ILCE-5100: arch CXD90014, apps 2.7, updatershell yes`
- Jebakan: install menolak kalau baterai < 50% ("505 Low battery")

## 7. Python untuk analisis dump

```bash
pip3 install capstone pyelftools numpy pyftpdlib
# capstone 5.0.9: disasm Thumb-2 manual (Apple objdump TIDAK support force-thumb)
# pyelftools: konversi VMA↔file offset via PT_LOAD (VMA==offset untuk segmen
#             LOAD pertama semua lib kamera)
# numpy: range-scan literal pool (pola PIC ldr rX,[pc] + add rX,pc — alamat
#         absolut tidak ada di kode kamera; persamaan target = W + align4(A+4))
```

## 8. Yang TIDAK dipakai (dan kenapa)

| Tool | Alasan |
|---|---|
| Hopper |alternatif Ghidra — tidak dipakai (Ghidra + rea cukup) |
| NDK / arm-linux-gnueabi-gcc | tidak ada di Mac; freestanding clang+lld sudah cukup |
| adb shell | adbd kamera stub — `error: closed` selalu |
| objdump Apple | tidak support `force-thumb` untuk Thumb-2 manual |
| Ghidra 12.2 | rea 6.7.0 menolak (hanya 12.1.x) |

## 9. Struktur folder host yang dipakai dokumen ini

```
~/Documents/a5100-re/            # working dir (host, tidak di-commit)
├── tools/android-14/            # build-tools
├── tools/froyo/                 # android.jar lama
├── pmca-re/                     # PMCA console
├── ghidra-proj/                 # project Ghidra (a5100obj)
├── native/                      # libObj.so, libScalarDaemon.so, ... (ekstrak sendiri!)
└── ftp-root/                    # FTP exchange dgn kamera
~/tools/ghidra_12.1.4_PUBLIC/    # Ghidra (879 MB)
```
