# Test Photos — GT20 histogram analysis set (6-7 Nov)

Foto hasil uji `setExtendedGammaTable` (GT20, still mode). Resolusi asli
6000×3376 (~4 MB/foto) disimpan di host riset; di repo ini preview 1600px.

| File | Kondisi | Catatan |
|---|---|---|
| `analysis_DSC04579_preview.jpg` | baseline | unik (MD5 c2ce0b5e...) |
| `analysis_DSC04581_preview.jpg` | test A | identik dengan DSC04582 (MD5 9fb28db6...) |
| `analysis_DSC04583_preview.jpg` | test B | unik (MD5 4af378ba...) |

**Hasil analisis kuantitatif** (lihat docs/legacy/ANALISIS-NATIVE.md §UPDATE):
mean luma identik 212.0, std 68.1 — tabel gamma TIDAK mengubah output.
Ini bukti fisik dead-end jalur Picture Effect.

Catatan: pasangan DSC04691/04692 (tes definitif 7 Nov) juga di host,
angka histogram lengkap di ANALISIS-NATIVE.md.
