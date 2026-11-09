# A5100 Extended Gamma Table - Native Chain (RE 2026-11-06)

## Stack (top -> hardware)
1. Java: com.sony.scalar.hardware.CameraEx (sony.cameraex.odex)
   - open(int, OpenOptions) -> instance [WORKS, needs android.permission.CAMERA in app manifest]
   - createGammaTable() -> GammaTable{mPictureEffectGammaForceOff}
   - setExtendedGammaTable(GammaTable) / getExtendedGammaTable()
2. libcamera.so: android::CameraHardwareDiadem::setExtendedGammaTable(int,int) @0x221e8(raw)/0x321e8(older offset note)
   - SDComMsg MsgId 0x103e, params: id4=int A, id0x26=bool(A!=0), id0x7f=int B
3. libScalarDaemon.so: CommandProcessorCameraSetExtendedGammaTable
   - extract: job+0x34 = A(table handle/ticket), job+0x38 = bool SW, job+0x39 = bool
   - Sync: checks ParameterStorage 0x3001; number==2 -> migrateToAsync
   - Async: if handle==0 -> ticket=0; else getTicketPhysicalAddr(handle, 6, &vec, &vec) -> ticket=vec[0]
   - -> SystemEventIssuerCamera::setExtendedGammaTable(jobId, ticketPhysAddr, b1, b2)
     - SYS event model=0x13 id=0x14, params: 0x101=jobId, 0x251=ticketAddr, 0x252=(b1?2:1), 0x253=(b2?2:1)
4. libObj.so (16MB, stripped): CamMsgConv SetExGammaTableControlForLiro FUN (Ghidra 0x94d654)
   - consumes MWF ObjMsg MsgId 0x76ac / 0x75e6
   - params: 0x9a18=PARAMID_EX_GAMMA_TABLE_SW (values 0x4da=off / 0x4db=on -> struct[8])
            0x9a19=PARAMID_PICTURE_EFFECT_GAMMA_MODE (0x4dc/0x4dd -> struct[9])
            0x9a1a=PARAMID_EX_GAMMA_TABLE_POINTER (32-bit VALUE -> struct[10..13])
   - builds struct: [0]=5, [2]=0xf0, [3]=0x40, [4..7]=param_3, [8]=sw, [9]=peMode, [10..13]=pointerValue
5. Ticket system: SystemEventIssuerCommon::createTicket(p1,p2,p3,dim(p4,p5),p6,b?,p7,p9,p10,p11)
   - event model=1 id=2, params 0x101=p1, 0x202=p2, 0x302=PrmDimension(p4,p5), 0x204=p3, 0x215=p6, 0x214=p7, opt 0x308=bool+0x309/0x30a/0x30b
   - getTicketPhysicalAddr(handle,6,&vec,&vec) -> physical addr for the camera pipeline

## KEY INSIGHT
The "gamma table" is NOT passed inline - it lives in a SHARED-MEMORY TICKET. The 32-bit "pointer" param is the ticket physical address. Java GammaTable(int,int) ctor likely = (type, ticketId) or (size, handle).

## Camera IP list (pulled via busybox ftpput -> ~/a5100-re/native/)
- libObj.so 16,354,724 B md5 80056ac1db6a9ea85ba35a28b0abed4f
- libScalarDaemon.so 1,867,936 B md5 051fe1eab60d5b1b245563222e4ffbbb
- libcamera.so 198,780 B md5 7da774baffd7ac29d4c11099df424085

## Next steps
- Decompile ParamStd_Template<int> param 0x9a1a producer on daemon side (event 0x13/0x14 param 0x251 -> MWF msg translation) to see what fills struct[10..13]
- Find who calls createTicket for gamma (0x302 dimension = table rows/cols?)
- Repull sony.cameraex.odex from camera (CAMERAEX.TXT on SD) once telnet is back; decompile GammaTable(int,int) ctor semantics

## 2026-11-06 static curve hunt result
- libObj has ~32 zeroed 512-byte table SLOTS (0xd14f6e stride 0x200) = runtime-filled gamma LUT slots (currently empty in static file)
- PP_GAMMA enum list found: STD/STD_MOV/STD_STL, CINE1-4, ITU709, ITU709_800, SLOG2, CINEMA_TONE1/2; PP_COLOR_SGAMUT; black gamma + knee sets
- libcamera has NO PP gamma key - only pro-color-mode (neutral/vivid) + extended-gamma-table-supported
- CONCLUSION: presets are filled at runtime from elsewhere (maybe libkmod DSP or computed); live-test path is faster than static hunting
