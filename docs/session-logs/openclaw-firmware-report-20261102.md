# Sony A5100 + rekam LOG: hasil bedah firmware nyata

Tanggal: 2026-11-02. Dikerjakan: firmware resmi A5100 di-unpack, dibaca isinya, dicari jalur S-Log.

## 1. Yang dikerjakan (bukan teori)

- Firmware resmi Sony A5100 v3.10 (rilis 14/03/2016), 217 MB, diunduh dari
  `https://support.d-imaging.sony.co.jp/download/NEX/wSeH150Nb3/Update_ILCE5100V310.dmg?fm=eu`
  (link diambil dari halaman Sony UK Z0005409; path `di.update.sony.net/NEX/Update_ILCE5100V310.*` sudah 404, jadi pakai mirror d-imaging).
- Firmware di-unpack pakai fwtool.py (ma1co). Hasil: `FirmwareData_ILCE5100V310.dat` -> `firmware.tar` -> partisi `nflasha3/5/7/15/16` jadi filesystem (FAT, ext2, cramfs).
- Isi partisi dibaca: Linux ARM 32-bit (main system), Android 2.3.7 (ScalarA, board `dslr-diadem`), dan partition config.

## 2. Temuan kunci

### A. S-Log2 ternyata ADA di resource A5100, cuma tidak ditampilkan

- `share/app/string_english.uxc` berisi string menu Picture Profile lengkap:
  `Picture Profile`, `Gamma`, `Black Gamma`, `Knee`, `Color Mode`, `Detail`,
  `S-Log2`, dan deskripsi "Gamma curve equivalent to S-Log2".
- `lib/CautionConfig.so` punya 1847 node `PictureProfile*`, termasuk
  `cmnViewSettingNodePictureProfileGammaSLOG2` dan `...PropertyListPictureProfileSLOG21..26`.
- `etc/InhInfo_InhMgr.db` (Android) punya
  `INH_FACTOR_CAM_SET_GAMMA_TYPE_PP_GAMMA_SLOG2_TYPE_P` dan `..._CINEMA_TONE1/2_TYPE_P`.

Artinya: kapabilitas gamma S-Log2 sudah ikut tertanam di build, dan aplikasi Android
sudah mengerti istilahnya. Yang hilang cuma jalur menampilkannya di menu A5100.

### B. Ada tabel gamma yang bisa diganti runtime (jalur paling menjanjikan)

- `libObj.so` (pipeline kamera) menerima pesan `MSGID_SET_EX_GAMMA_TABLE_CONTROL_CMD`
  dengan parameter `PARAMID_EX_GAMMA_TABLE_SW` dan `PARAMID_EX_GAMMA_TABLE_POINTER`,
  plus `INFO_RET_SET_EXTENDED_GAMMA_TABLE` dan `extendedGammaTablePointerSet`.
- `av-cam.bin` (firmware ISP) punya `EX_GAMMA_TABLE_CONTROL[ON=%d][ForceOff=%d][Pointer=0x%x]`
  dan log `Gamma/EC_Gamma.cpp` serta pengecekan `LiroSeq ParamCheckErr ... [PP_GammaType]`,
  `[ExGammaTableOn]`, `[ul_ExGammaTablePointer]`, `[PictureEffectGammaForceOFF]`.
- Android membuka API-nya: `com/sony/scalar/hardware/CameraEx$GammaTable` dengan
  `native_createGammaTable`, `getExtendedGammaTable`, `setExtendedGammaTable(GammaTable;Z)`,
  `isPictureEffectGammaForceOff`, `setPictureEffectGammaForceOff` (stub lengkap ada di
  OpenMemories-Framework: `CameraEx.j`, `CameraEx$GammaTable.j`, versi "added in V3").
- Di sisi server ada `SCALARDAEMON::SystemEventIssuerCamera::setExtendedGammaTable(int,int,bool,bool)`
  di `libScalarDaemon.so`.

Jadi rantainya utuh: app Android -> JNI -> `libScalarDaemon` -> kamera -> tabel gamma logam.
Yang belum ada adalah sepotong kode yang mengirim tabel gamma S-Log2 dari app pihak ketiga.

### C. Kenapa Magic Lantern bisa dan ini tidak

Magic Lantern main di firmware yang tidak ditandatangani (DIGIC). A5100 (arsitektur CXD90014)
untuk jalur update pakai `updatershell`, dan update firmware A5100 hanya kena partisi
setting/backup, bukan firmware kamera. Jalur app resmi Sony (PMCA) terbatas karena app
jalan di Android dan tidak boleh mengganti tabel gamma pipeline. Jalur "extended gamma table"
di atas adalah yang secara arsitektur paling dekat dengan yang dipakai Magic Lantern
(mengganti LUT, bukan membongkar kernel).

## 3. Kenapa hasilnya belum S-Log hari ini

- Mengirim tabel gamma S-Log2 butuh (a) menulis app Android PMCA, (b) memanggil
  `setExtendedGammaTable`, (c) tahu format isi tabel `GammaTable` (ukuran, jumlah entri,
  skala). Format isi belum di-reverse.
- A5100 pakai Android 2.3.7 dengan sertifikat debug diterima, jadi jalur app legal dan tidak
  menyentuh firmware. Tapi app yang ada sekarang (OpenMemories Tweak) hanya mengubah setting,
  bukan gamma.
- Setelah ditest, kalau `PP_GammaType` di AV-Cam memang dibatasi untuk A5100, jalur
  tabel gamma masih terbuka karena terpisah dari PP menu.

## 4. Jalur realistis kalau mau lanjut

1. **Paling ringan:** pakai Picture Profile bawaan (Off/Still/Movie) + set contrast -3,
   saturation -2, detail turun, lalu expose ke kanan. Ini yang sudah dipakai untuk film
   finalis Sony. Bukan LOG asli, tapi grading-friendly.
2. **Menengah:** cek lewat `adb` apakah `CameraEx.getExtendedGammaTable()` ada di A5100
   (panggil dari app test). Kalau ada, kita sudah tahu pintunya terbuka.
3. **Penuh:** bikin app PMCA yang menggambar ulang tabel gamma mendekati kurva S-Log2,
   kirim lewat `setExtendedGammaTable`, lalu validasi dengan waveform di Resolve.
   Butuh reverse format `GammaTable` dulu (di `libScalarDaemon.so` / `libcameraex_jni.so`).
4. **Alternatif non-RE:** rekam S-Log dari A6300/A6100 yang punya PP? Catatan lama: A6100
   juga tidak punya S-Log. Jadi bukan jalan keluar.

## 5. Video yang dikirim

- `S8Z9gL5-leE` = Magnus Video Productions, "Magic Lantern Is Back? I Asked AI to Hack My 5D Mark IV", 29 Okt 2026, 10:48.
- `sJYBs5CLywk` = Breno Loves Cameras, "Hacking Camera Software with AI is Getting Crazy", 29 Okt 2026, 8:35.
- `lbtUnjWh1_c` = Tony Northrup, "I hacked my camera with AI (and so can you)", sudah dianalisis 1 Nov; tidak menyebut A5100/A6000/S-Log.

## 6. File kerja

- Firmware unpack: `/tmp/sony5100/extract/` (nflasha15 = main Linux system, nflasha16 = Android)
- Bukti string: `share/app/string_english.uxc`, `lib/CautionConfig.so`, `etc/InhInfo_InhMgr.db`,
  `libObj.so`, `av-cam.bin`, `libScalarDaemon.so`, `libcameraex_jni.so`
- Stub API: `/tmp/omf/stubs/src/main/jasmin/com/sony/scalar/hardware/CameraEx*.j`
