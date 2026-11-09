# Memory Dumps — Research Evidence

> Alamat fisik ada di NAMA FILE (protokol baru sejak 9 Nov). Dump lama (7 Nov)
> tidak mencatat alamat dengan konsisten — ditandai `state-unknown`.
> **Bukan firmware Sony** — ini snapshot RAM hasil kerja kita, aman distribusi.

## Region parameter ISP (kandidat utama, 508 KB @ 0x82df9000)

| File | Kondisi | Catatan |
|---|---|---|
| `param_0x82df9000_contrast-3_20261109.bin` | Contrast **-3** | utuh 507.904 B; MD5 ba649302fa78524e58bc793629316509; sumber diff A/B |
| `param_0x82df9000_contrast+3_partial_20261109.bin` | Contrast **+3** | 401.408 B (transfer kepotong — chunk 98..123 missing); 2 diff pertama terverifikasi |
| `param_0x82df9000_state-unknown_20261107.bin` | tidak tercatat | dump 7 Nov (protokol lama) |

**Diff -3 vs +3 yang terverifikasi (via memdiff on-camera):**
- offset `0x5340C` (fisik `0x82E4C40C`): u16 LE `0x3CE5` → `0x3D05` (delta +0x20)
- offset `0x574F5`: 32 byte log text daemon (noise, abaikan)
- Reassemble utuh: `cat` chunk urut numerik — lihat REPRODUCE.md §7

## Pasangan region kecil -3/+3 (semua IDENTIK = eliminasi lokasi kurva)

| Alamat fisik | Isi |
|---|---|
| `0x82d90000` | "IMCMN VER:109" + magic 55aa55aa + param table |
| `0x83438000` | descriptor/log buffer (churn log saja) |
| `0x8e6d1000` | structured 3.2KB |
| `0x82eae000` | filler 0x5A |
| `0x965de000` | unknown, identik |

## Region lain

| File | Isi |
|---|---|
| `ispwork_0xC0000000_622KB_20261109.bin` | work memory ISP (baris 425-entri nilai 0-24 — bukan LUT) |
| `ctx_page_0x83e67020_20261107.bin` | context struct DSP ([0x140] PP slot, [0x256] ExGammaOn, [0x260] ptr) |
| `exgate_page_0x83813xxx_20261107.bin` | halaman capability gate (patch target) |
| `uipc_ring_20261107.bin` | IPC shared-memory ring buffer |

## Area yang BELUM ada dump-nya (tugas kontributor)

- Heap daemon `[heap]` vm 0x16000-0x420000 (4.2 MB) — kandidat LUT host-side P2a
- Image plane `0x92ec0000` (1.228.800 B = 1280×960) — churn besar saat kontras
  berubah; berguna untuk rekonstruksi kurva (P2c) tapi file besar, tidak di-commit
- ⚠ `0xf1000000` register ISP — **DILARANG** (2× kamera wedge)
