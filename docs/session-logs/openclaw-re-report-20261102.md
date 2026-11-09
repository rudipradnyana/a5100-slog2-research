# A5100 log recording: hasil reverse engineering nyata (2026-11-02)

Pakai rea + Ghidra 12.1.4. Semua klaim di bawah dari decompile, bukan dari tebakan.

## Setup

- Ghidra 12.2 dari brew tidak diterima rea (rea minta 12.1.x). Ghidra 12.1.4 resmi diunduh ke
  `/Users/rudipradnyana/tools/ghidra_12.1.4_PUBLIC`.
- Rilis 12.1.4 untuk macOS tidak menyertakan native decompiler `mac_arm_64`. Dipakai binary dari
  Ghidra 12.2 (ditempatkan di `Ghidra/Features/Decompiler/build/os/mac_arm_64/decompile`).
- `GHIDRA_INSTALL_DIR=/Users/rudipradnyana/tools/ghidra_12.1.4_PUBLIC`
- `JAVA_HOME=/opt/homebrew/opt/openjdk@25`
- Target: `libScalarDaemon.so`, `libObj.so`, `av-cam.bin` (ARM 32-bit, ELF).

## Temuan 1: jalur tabel gamma benar-benar ada dan lengkap

`SCALARDAEMON::SystemEventIssuerCamera::setExtendedGammaTable(int jobId, int physAddr, bool, bool)`
di `0x128ada` membangun `SysParamList` lalu memanggil `issueEvent(0x13, 0x14, list)`.

Parameternya:
- `0x101` = job id
- `0x251` = alamat fisik buffer tabel gamma
- `0x252` = flag 1/2 (param_3)
- `0x253` = flag 1/2 (param_4)

## Temuan 2: jalur upload tabel gamma utuh sampai ke kamera

`CommandProcessorCameraSetExtendedGammaTable::AsyncProcessor::execute()` di `0x150570`:

1. Kalau ada alamat fisik buffer, ia panggil
   `SystemEventIssuerCommon::getTicketPhysicalAddr(ptr, 6, vecA, vecB)`.
2. Ambil alamat fisik hasilnya via `vector::at(0)`.
3. Baru panggil `setExtendedGammaTable(...)`.
4. Kalau berhasil, state jadi 2 dan tunggu event balasan `0x1e16`.
5. Balasan dicek di parameter `0x1c51` (job id) dan `0x1c52` (status == 1 berarti sukses).

Artinya: daemon punya mesin lengkap untuk mengirim tabel gamma ke kamera dan menunggu konfirmasi.

## Temuan 3: Android sudah membuka API-nya ke app

- `com.sony.scalar.hardware.CameraEx$GammaTable` extends `DeviceBuffer`, punya `getSize/read/write`
  (stub OpenMemories-Framework, ditandai "added in V3").
- `CameraEx.createGammaTable()`, `getExtendedGammaTable()`, `setExtendedGammaTable(GammaTable)`.
- `libcamera.so` (Android HAL) memuat key parameter `extended-gamma-table-supported`.

## Temuan 4: yang masih belum diketahui

- Apakah kamera A5100 benar-benar menjawab "didukung" untuk key itu. Itu baru bisa dijawab dengan
  kamera terpasang.
- Format isi tabel gamma (jumlah entri, skala, per-channel atau tidak). Belum ada konstanta ukuran
  yang ketemu di binary.

## Kesimpulan

Jalur tabel gamma bukan spekulasi. Kode server, jalur pesan, dan API Android-nya ada di firmware
A5100 v3.10. Yang belum terbukti cuma dua: kamera mengaku mendukung atau tidak, dan format tabelnya.

Catatan lama yang perlu dikoreksi: klaim "S-Log2 ada di A5100, cuma disembunyikan" salah. S-Log2 di
resource cuma artefak basis kode bareng. S-Log3, HLG, dan PP10 nol kemunculan.

## File

- Ghidra: `/Users/rudipradnyana/tools/ghidra_12.1.4_PUBLIC`
- Snapshot rea: `/Users/rudipradnyana/a5100-re/libScalarDaemon.snapshot`
- Firmware unpack: `/tmp/sony5100/extract`

---

## Lanjutan: jalur lengkap app -> kamera (2026-11-02)

### libcamera.so (HAL Android)

`android::CameraHardwareDiadem::setExtendedGammaTable(int, int)` di `0x321e8` mengirim
`SDComMsg_Basic(param_1, 0x103e, vector)` dengan tiga `SDComMsgIntParam`:

- id `4`   = param_2 (mode)
- id `0x26` = (param_2 != 0)
- id `0x7f` = param_3 (pointer)

MESSAGE_ID `0x103e` = jalur gamma di sisi HAL.

### libObj.so (sisi kamera, CamUser)

- `CamUser::SequenceSetExGammaTableControl`
- `MSGID_SET_EX_GAMMA_TABLE_CONTROL_CMD` / `_CMP`
- `MSGID_NOTIFY_EX_GAMMA_TABLE_CONTROL_EVT` / `_REPLY_EVT`
- `[M_SCALAR_CAMERAPATCH] ... out: extendedGammaTableModeSet=%d`
- `[M_SCALAR_CAMERAPATCH] ... out: extendedGammaTablePointerSet=0x%08x`

Dua string terakhir adalah **jalur baca-balik** (readback) mode + pointer tabel. Ini pasangan
dari `CameraEx.getExtendedGammaTable()`.

### API Android yang tersedia (ParametersModifier)

- `isExtendedGammaTableSupported()` -> jawaban langsung "didukung atau tidak".

### Pintu masuk ke kamera

Dari `fwtool.py/devices.yml`, resmi: **ILCE-5100 / arch CXD90014 / apps 2.7 / updatershell ya**.
Jadi dua jalur masuk: install app Android lewat `pmca-console`, dan updatershell.
Toko Sony (`sony-pmca.appspot.com`) sudah 503, tapi tidak diperlukan.

`pmca-console` siap di `/tmp/pmca-re` (venv + deps terpasang).

### Catatan koreksi

- "Liro" BUKAN nama platform A5100. Muncul di 221 string `*ForLiro NON SUPPORTED` (SetColorMode,
  SetNRLevel, SetBrightness, dst), jadi itu cabang error generik. Tidak dipakai sebagai bukti.
- A5100 memang dukung PMCA. Klaim awal "toko tutup berarti tidak bisa" terlalu cepat.
