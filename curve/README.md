# slog2_curve.bin, 2048 bytes, 1024×u16 LE

Kurva S-Log2 12-bit hasil generate dari rumus publik Sony (bukan hasil ekstraksi
firmware Sony, aman didistribusikan).

Formula S-Log2 (Sony public whitepaper):
- Input: linear sensor signal x (0.0-1.0, di atas 18% grey 0.18)
- Bisa direproduksi: LUT[i] untuk i di 0..1023 → nilai 12-bit (0..4095)

Pemakaian:
- Target timpa in-place ke buffer kurva aktif ISP (protokol: NEXT-STEPS.md P2)
- Atau kirim via jalur yang belum teruji (EXPERIMENTS.md A7)

Verifikasi bentuk: file harus 2048 byte; u16 pertama ~0x0B3, terakhir ~0xFFF,
monotonik naik dengan slope kecil di shadow (flat black floor khas S-Log2).
