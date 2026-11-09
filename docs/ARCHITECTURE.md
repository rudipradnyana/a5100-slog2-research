# ARCHITECTURE, Arsitektur Sony A5100 (ILCE-5100, fw 3.10)

> Rudi susun dokumen ini dari bedah firmware resmi, decompile statis dengan Ghidra dan capstone, dan observasi runtime. Tiap klaim punya sumber: dokumen legacy atau eksperimen yang bisa kamu ulang. Kamu cek EXPERIMENTS.md untuk status bukti.

---

## 1. Diagram blok

```
┌─────────────────────────── CXD90014 SoC ───────────────────────────┐
│                                                                    │
│  ┌──────────────────────────┐      ┌────────────────────────────┐  │
│  │  Core 1: ARM Cortex-A9   │      │  Core 2: ARM7 (sub-core)   │  │
│  │  Linux 3.0.27_nl-rt106   │      │  ITRON RTOS                │  │
│  │  + Android 2.3.7 (HAL)   │      │  ISP firmware: av-cam.bin  │  │
│  │                          │      │  ("Liro" imaging engine)   │  │
│  │  ScalarDaemon (im.elf)   │◄────►│                            │  │
│  │  PID 149, libObj.so      │ liro │  loads di RAM 0x83541000   │  │
│  │  libScalarDaemon.so      │ .ko  │  (ARM7 addr = Linux -      │  │
│  │                          │      │   0x40000000)              │  │
│  └──────────────────────────┘      └────────────────────────────┘  │
│         │                                                          │
│  ┌──────▼────────────────┐        ┌──────────────────────────────┐ │
│  │ App Android pihak-3   │        │ RAM fisik (Linux view)       │ │
│  │ (PMCA, telnet, ADB)   │        │ 0x80000000-0xbfffffff System │ │
│  └───────────────────────┘        │ 0xc0000000-0xffffffff RAM/   │ │
│                                   │   peripheral (⚠ region       │ │
│                                   │   0xf1000000 = register ISP, │ │
│                                   │   dump = kamera wedge)       │ │
│                                   └──────────────────────────────┘ │
└────────────────────────────────────────────────────────────────────┘
```

## 2. Rantai komunikasi app → ISP (jalur gamma)

```
Java app (GT-series APK)
  └─ com.sony.scalar.hardware.CameraEx          [DEX, Android HAL]
      ├─ open(OpenOptions)          - WORKS (background thread + Looper!)
      ├─ createGammaTable() → GammaTable extends DeviceBuffer
      │    └─ getSize/read/write, setPictureEffectGammaForceOff(bool)
      └─ setExtendedGammaTable(GammaTable)
          └─ JNI: libcamera.so  CameraHardwareDiadem::setExtendedGammaTable
              @0x321e8 → SDComMsg 0x103e {id4=mode, id0x26=mode!=0, id0x7f=ptr}
              └─ libScalarDaemon.so  CommandProcessorCameraSetExtendedGammaTable
                  Sync::execute  @0x140452
                  Async::execute @0x140570
                      └─ getTicketPhysicalAddr → shared-memory ticket
                          └─ SystemEventIssuerCamera::setExtendedGammaTable
                              @0x128ada → SysParamList
                              {0x101=job, 0x251=physAddr, 0x252/253=flag}
                              → issueEvent(0x13, 0x14)   [ke ISP]
                                  └─ libObj.so CamUser::SequenceSetExGammaTableControl
                                      params: 0x9a18 SW, 0x9a19 PE mode, 0x9a1a pointer
                                      → ObjMsg ke ISP (0x76ac/0x75e6)
                                          └─ av-cam.bin EX_GAMMA_TABLE_CONTROL
                                              [ON=%d][ForceOff=%d][Pointer=0x%x]
                                              ⚠ LiroSeq ParamCheckErr bila
                                                pointer baru (model check)
```

**Kunci arsitektur #1 (ticket system):** tabel gamma tidak dikirim inline.
hidup di shared-memory ticket; parameter "pointer" = alamat fisik ticket.
`getTicketPhysicalAddr(handle, 6, ...)` yang menerjemahkan handle→alamat.

**Kunci arsitektur #2 (tiga jalur gamma terpisah):**

| Jalur | Param/simbol | Fungsi | Status di A5100 |
|---|---|---|---|
| PP_GAMMA (+BLACKGAMMA/KNEE) | MWF 0x9685, param IPC 0x238 | **gamma movie (S-Log/Cine)** | ADA di firmware, menu disembunyikan, gate di ISP |
| setExtendedGammaTable | 0x9a18/0x9a19/0x9a1a | gamma **Picture Effect (still)** | Dead end empiris (histogram identik) |
| PictureEffectGammaForceOff | CameraEx$GammaTable | saklar "pakai kurva custom" | Flip sukses, tak berpengaruh output |

**Kunci arsitektur #3 (gate PP adalah routing, bukan capability):** string
"SetPictureProfileForLiro NON SUPPORTED" = default-case handler konverter.
Pola umum semua converter CamMsgConv:

```
if   msgId == khusus(X) → proses
elif msgId == 0x75E6    → proses   (generic set)
else → log "...ForLiro NON SUPPORTED MsgID[0x%04x]"
```

PP menerima **0x75AE** (khusus) / **0x75E6**. Selama ini yang salah adalah
MsgID yang dikirim, bukan penolakan model.

## 3. Format pesan & nilai kunci

**Wire PP SET** (libObj @0x951900, worker 0x951940..0x952150):
```
pesan byte: [0]=5, [2]=sub-id, [3]=0x40 (MSGID_SET_PICTURE_PROFILE_CMD)
byte[0x1a] = indeks gamma (0..11)
```
Peta indeks → nama: 0 STD, 1 CINEMA_TONE1, 2 CINEMA_TONE2, 3 ITU709,
4 STD_MOV, 5 STD_STL, 6 CINE1, 7 CINE2, 8 CINE3, 9 CINE4, **10 SLOG2**,
11 ITU709_800.

**Native setPictureProfile** (libScalarDaemon @0x118248):
```
struct 0x160B byte, 43 pasang {value@+8n, flag@+8n+4}
gamma: value @+0x88, flag @+0x8c → IPC param 0x238
akhir: sendEvent(model=2, id=0x40)
resetPictureProfile: default gamma=0 (STD)
```

**Dictionary param ID:** tabel 3503 nama @0xed068c (libObj). Rumus:
`MWF_ID = 0x9218 + (index − 2048)` (anchor PARAMID_EX_GAMMA_TABLE_SW=0x9a18
@idx 2048). PARAMID terpakai: 0x9685 PP_GAMMA_TYPE, 0x9686 PP_KNEE_MODE,
0x96a4 PP_GAMMA_TYPE_SET, 0x9a53 PP_MODE, 0x9a47 PP_COLOR_CORRECT_TYPE,
0x999c MOVIE_SIZE_H.

## 4. Peta memori fisik (yang relevan)

| Alamat | Isi | Catatan |
|---|---|---|
| 0x80000000-0x8fffffff | System RAM | Kernel text 80038000, data 80362000 |
| **0x83541000** | av-cam.bin (11.2 MB) | ISP firmware; liro.ko memuat di sini |
| 0x838119d0 / 0x83813dc4 / 0x83813dfc | 3 capability gate | offset dalam av-cam image |
| **0x83e67020** | context struct DSP | [0x140]=PP slot, [0x256]=ExGammaOn, [0x260]=ptr |
| 0x82df9000 (+508 KB) | parameter ISP | KANDIDAT kunci; diff Contrast hanya @0x5340C |
| 0x92ec0000 (1.228.800) | image plane liveview 1280×960 | buffer ganda (+0x96000); churn besar |
| 0xC0000000 (608 KB) | work memory ISP | baris 425-entri nilai 0-24 (bukan LUT) |
| 0x82d90000 (16 KB) | "IMCMN VER:109" + param | identik antar kontras |
| 0x83438000 (32 KB) | descriptor/log | churn log saja |
| **0xf1000000** | **register ISP** | ⚠ DILARANG dump, wedge 2× terkonfirmasi |
| 0x8e6d1000 / 0x965de000 | kecil, identik | eliminasi |
| 0x82eae000 | filler 0x5A | eliminasi |

`/proc/iomem` kamera: RAM region 0x93000000-0x9fffffff semua "System RAM"
(16 bank × 8 MB), heap daemon & kandidat LUT host-side di sini.

## 5. Startup & recovery alur (penting buat eksperimen)

1. Boot kamera → Linux naik → `liro.ko` load av-cam.bin ke 0x83541000 →
   ISP (ARM7/ITRON) mulai jalan → ScalarDaemon (PID 149) naik → telnetd
   **jalan hanya kalau Tweak aktif** (setiap boot harus enable ulang).
2. Semua patch RAM (gates, LUT, devmem) **hilang** saat power-off.
3. **Power-off bersih menyimpan setting menu** (kontras, dst);
   **crash-reboot (battery pull) me-reset** beberapa setting (teramati: kontras→0).
4. Wedge state (jaringan hidup, userspace mati): telnet connect lalu reset,
   ADB `error: closed`, ping kadang hidup kadang hilang. Recovery: power cycle.

## 6. Referensi file firmware (kontributor harus ekstrak sendiri)

| File | Sumber | Kegunaan |
|---|---|---|
| `Update_ILCE5100V310.dmg` | support.d-imaging.sony.co.jp (Z0005409) | firmware resmi 3.10 |
| unpack via `fwtool.py` | repo ma1co | → partisi nflasha3/5/7/15/16 |
| `nflasha15` | firmware | Linux main system (libObj.so 16 MB, libScalarDaemon.so 1.8 MB, av-cam.bin, liro.ko) |
| `nflasha16` | firmware | Android 2.3.7 (libcamera.so, sony.cameraex.odex, InhInfo_InhMgr.db) |
| `string_english.uxc` | partisi share | string menu PP (artefak basis kode) |
| `CautionConfig.so` | partisi lib | 1847 node PictureProfile* |
| `Backup.bin` (855 KB) | dump EEPROM kamera | belum dianalisis (ide: flag capability model) |

Kontributor: jangan commit firmware binari ke repo publik. Ekstrak dari
firmware resmi milikmu sendiri.
