# Sony ILCE-5100 Architecture & Extended Gamma Pipeline Specification
**Technical Architecture & Operating Reference Manual**

---

## 1. Executive Summary & Architecture Overview
This technical documentation details the signal processing pipeline and hardware abstraction layer (HAL) of the **Sony ILCE-5100 (Firmware v3.10)** for embedded Linux research and color pipeline configuration:

1. **Subsystem Architecture:** 
   The platform utilizes a dual-processor architecture. The primary application core runs Sony CE Linux on an ARM Cortex-A9 host, while image signal processing (ISP) tasks run on a dedicated ARM co-processor governed by an ITRON Real-Time OS (`av-cam.bin`).
2. **Capability Validation Subsystem:**
   Hardware profile capabilities are validated at the co-processor level. Three runtime validation checks evaluate internal context parameters (`context[0x140]`, `context[0x256]`, `context[0x260]`). In the ILCE-5100 platform profile, these fields default to inactive, causing standard profile requests to return invalid parameter status.
3. **Volatile Memory Adaptation (`patch_avcam`):**
   The ISP co-processor firmware image is mapped into physical system RAM at `0x83541000` via the kernel bridge driver `liro.ko`. By accessing physical memory addresses through `/dev/mem`, the three validation routines are dynamically configured in volatile memory to allow extended curve processing (`movs r0, #0; bx lr`). All changes reside purely in volatile RAM and persist only until system shutdown.
4. **Native HAL Parameter Dispatch (`slog2_direct`):**
   Through inter-process communication with the primary camera daemon (`ScalarDaemon`, PID 149), native C++ structures (`PictureProfileParameter`) are initialized with the target gamma transfer function (`VALUEID_PP_GAMMA_SLOG2 = 1165`) and committed via `setPictureProfile()`. The call successfully executes with return code `0` (`SUCCESS`).

---

## 2. Hardware & Operating Environment Specifications
- **Hardware Platform:** Sony ILCE-5100 (A5100)
- **Platform Identifier:** `0x42030036`
- **Firmware Version:** 3.10
- **SoC Architecture:** Sony CXD90014 (Dual Core ARM Cortex-A9 host + ARM7 ISP co-processor)
- **Host Operating System:** Sony Consumer Electronics Linux 8.2.1.9 (Kernel `3.0.27_nl-rt106` PREEMPT RT)
- **Subsystems:** Android 4.x Camera HAL + ITRON RTOS Co-Processor

---

## 3. Memory Mapping & Kernel Interconnect (`liro.ko`)
Communication between Linux host space and the co-processor is routed through the kernel driver module `liro.ko`:
- **Kernel Module Parameter:** `/sys/module/liro/parameters/adr` = `1129582592` (`0x43541000` in co-processor addressing space)
- **Physical Address Translation (Linux Space):** `0x83541000`
- **Address Translation Formula:**
  - `Linux Physical Address = Co-processor Address + 0x40000000`
  - `0x43541000 + 0x40000000 = 0x83541000`
- **Firmware Image Mapping:** The complete binary image `av-cam.bin` (size: 11,264,692 bytes) resides at base address `0x83541000`.

---

## 4. Co-Processor Validation Routine Addresses & Instructions
The three validation checkpoints inside the ISP firmware image correspond to the following physical memory locations:

| Entry Point | Functional Description | Physical Memory Address | Stock Instruction | Dynamic Configuration |
|---|---|---|---|---|
| **Target 1** | `ExGammaTableOn` validation | `0x83813dc4` | `70 b5 0d 46` | `00 20 70 47` (`movs r0, #0; bx lr`) |
| **Target 2** | `ul_ExGammaTablePointer` validation | `0x83813dfc` | `70 b5 0e 46` | `00 20 70 47` (`movs r0, #0; bx lr`) |
| **Target 3** | `PictureProfile` validation | `0x838119d0` | `70 b5 0e 46` | `00 20 70 47` (`movs r0, #0; bx lr`) |

### Context Data Structure
- **Context Base Physical Address:** `0x83e67020`
- `context[0x140]`: PictureProfile Profile Slot Identifier
- `context[0x256]`: `ExGammaTableOn` status register
- `context[0x260]`: `ul_ExGammaTablePointer` reference pointer

---

## 5. Camera Daemon (`ScalarDaemon`, PID 149) API Symbols
Mapped dynamic symbols resolved from `libScalarDaemon.so` (1.7 MB) and `/proc/149/maps`:
- **Daemon Virtual Base Address:** `0x12e4f000`
- **Libc Virtual Base Address:** `0x04251000`
- **Constructor `PictureProfileParameter::PictureProfileParameter()`:**
  - Binary Offset: `0x117eb8`
  - Runtime Address (Thumb): `0x12e4f000 + 0x117eb8 + 1 = 0x12f66eb9`
- **Function `setPictureProfile(PictureProfileParameter*)`:**
  - Binary Offset: `0x118248`
  - Runtime Address (Thumb): `0x12e4f000 + 0x118248 + 1 = 0x12f67249`
- **Function `resetPictureProfile()`:**
  - Binary Offset: `0x118830`
- **Function `setExtendedGammaTable(int, int, bool, bool)`:**
  - Binary Offset: `0x118ada`
- **Gamma Parameter Constants:**
  - `VALUEID_PP_GAMMA_SLOG2` = `1165` (`0x48d`)
  - `VALUEID_PP_GAMMA_STANDARD` = `1160` (`0x488`)

---

## 6. Project Directory Structure
All project assets and binaries are structured under `/Users/rudipradnyana/Documents/a5100-slog2-backup-20261107/`:
- `README_FINDINGS.md`: Original research notes.
- `DOCUMENTATION_TECHNICAL_SPEC.md`: This standardized technical architecture reference.
- `camera_reverse_engineering_core/`: Decompiled framework libraries, Ghidra database snapshots (`libScalarDaemon.snapshot`, `libcamera.snapshot`), and system profile definitions (`devices.yml`, `apps.yaml`).
- `sources/`: C source routines and utilities (`patch_avcam.c`, `slog2_direct.c`, `find_avcam.c`, `cam_cmd.py`, `memdiff_lut.py`).
- `binaries_arm/`: Compiled ARMv7 ELF binaries (`patch_avcam`, `slog2_direct`, `find_avcam`).
- `camera_firmware_and_drivers/`: Firmware dumps (`av-cam.bin`), kernel modules (`liro.ko`, `backup.ko`), and LUT tables (`slog2_curve.bin`).
- `camera_native_libs/`: Sony core framework shared libraries (`libScalarDaemon.so`, `libSysDef.so`, `libBizFw.so`, `libMWF.so`).
- `mem_dumps/`: Captured physical memory states (`dump_baseline.bin`, `dump_changed.bin`, `dump_contrast.bin`).
- `openclaw_research_and_memory/`: Historical technical logs and system progression reports.
- `kimi_strategy/`: In-place lookup table (LUT) mapping analysis.

---

## 7. Execution & Configuration Procedure
Because all modifications are applied in volatile RAM, the camera hardware retains 100% factory state upon reboot. To initialize the extended gamma curve:

1. **Establish Remote Network Session:** Ensure the camera device is connected via local Wi-Fi interface (e.g. `192.168.68.114`).
2. **Deploy Runtime Binaries to `/tmp`:**
   ```bash
   python3 /Users/rudipradnyana/Documents/a5100-re/cam_cmd.py "busybox ftpget -u cam -p cam -P 2121 192.168.68.110 /tmp/patch_avcam patch_avcam && chmod +x /tmp/patch_avcam"
   python3 /Users/rudipradnyana/Documents/a5100-re/cam_cmd.py "busybox ftpget -u cam -p cam -P 2121 192.168.68.110 /tmp/slog2_direct slog2_direct && chmod +x /tmp/slog2_direct"
   ```
3. **Execute Configuration Utilities:**
   ```bash
   python3 /Users/rudipradnyana/Documents/a5100-re/cam_cmd.py "/tmp/patch_avcam && /tmp/slog2_direct"
   ```
   *Expected Return:*
   - `patch_avcam`: Validation confirmation for memory offsets `0x83813dc4`, `0x83813dfc`, `0x838119d0`.
   - `slog2_direct`: `setPictureProfile(S-Log2)` returns `0` (Success).

---

## 8. Verification & Validation Protocol
1. **Live Preview Verification:** Observe camera LCD display; black level elevates to characteristic S-Log2 flat tone curve response.
2. **Video Capture & Extraction:** Record sample sequence to internal SD card (`/android/mnt/sdcard/PRIVATE/M4ROOT/CLIP/`).
3. **Luminance Histogram Validation:**
   ```bash
   ffmpeg -i <CLIP>.MP4 -vf signalstats -f null - 2>&1 | grep YMIN
   ```
   *Validation Threshold:* `YMIN >= 90` confirms standard S-Log2 ~10% IRE black floor elevation.

