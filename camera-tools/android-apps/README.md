# Android Apps, GT-series (GammaTest)

Aplikasi uji yang dibangun selama riset (build recipe: docs/legacy/STATUS-HANDOFF.md).

| App | Fase | Fungsi | Status |
|---|---|---|---|
| `gt6-10-archive/` | 6 Nov awal | Eksplorasi CameraEx API (GT6 s.d. GT10); GT10 = tes setExtendedGammaTable pertama | Dead end empiris |
| `gt13-18-archive/` | 6 Nov | MediaRecorder + OpenOptions movie mode (GT17: prepare/start OK pertama; GT18: hard crash) | Dokumentasi kegagalan |
| `gt21/` | 7 Nov | **Probe-only**, enumerasi PP capability, log ke /sdcard/GAMMALOG.TXT, TIDAK memanggil setter | ⏸ Belum pernah dijalankan |
| `gt22/` | 7 Nov | `injectSLog2(10)`, panggil native setPictureProfile (struct 0x160B, gamma@+0x88=10, flag@+0x8c=1) via JNI `libppinjector.so` | ⏸ Belum pernah dijalankan |

**Keystore signing:** `tools/keystore/gammatest-debug.keystore` (pass: android).
**Analisis DEX CameraEx:** `tools/dex-analysis/` (dexdump.txt = semua method CameraEx, dexstrings.txt = semua string; sumber klaim "CameraEx tidak expose PP").

## Urutan yang disarankan kontributor:
1. Install & jalankan **GT21r** → baca GAMMALOG.TXT (probe aman, read-only)
2. Kalau capability terbaca → **GT22** → amati LCD + tangkap log daemon
3. Detail ekspektasi: docs/NEXT-STEPS.md Prioritas 3
