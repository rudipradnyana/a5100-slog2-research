import struct

raw = open('/Users/rudipradnyana/a5100-re/native/sony.cameraex.odex','rb').read()
i = raw.find(b'dex\n035')
data = raw[i:]

str_sz, str_off = struct.unpack_from('<II', data, 0x38)
typ_sz, typ_off = struct.unpack_from('<II', data, 0x40)
pro_sz, pro_off = struct.unpack_from('<II', data, 0x48)
fld_sz, fld_off = struct.unpack_from('<II', data, 0x50)
mth_sz, mth_off = struct.unpack_from('<II', data, 0x58)

def uleb128(buf, off):
    result = 0; shift = 0
    while True:
        b = buf[off]; off += 1
        result |= (b & 0x7f) << shift
        if not (b & 0x80): break
        shift += 7
    return result, off

def get_string(idx):
    if idx >= str_sz: return "<str%d?>" % idx
    off = struct.unpack_from('<I', data, str_off + idx*4)[0]
    if off >= len(data): return "<badoff>"
    ln, off = uleb128(data, off)
    end = data.find(b'\x00', off)
    if end < 0: end = min(off+ln, len(data))
    return data[off:end].decode('utf-8', errors='replace')

def get_type(idx):
    if idx >= typ_sz: return "<typ%d?>" % idx
    desc_idx = struct.unpack_from('<I', data, typ_off + idx*4)[0]
    return get_string(desc_idx)

# field_id = <class_idx:u16, type_idx:u16, name_idx:u32>
fields = {}
for idx in range(fld_sz):
    off = fld_off + idx*8
    if off + 8 > len(data): break
    cls_idx, typ_idx, name_idx = struct.unpack_from('<HHI', data, off)
    cname = get_type(cls_idx)
    fname = get_string(name_idx)
    ftype = get_type(typ_idx)
    fields.setdefault(cname, []).append((fname, ftype))

for cname in sorted(fields):
    if any(k in cname for k in ['GammaTable','VideoSource','AudioSource','MediaRecorder','CamcorderProfile','DeviceBuffer','DeviceMemory']):
        print("=== " + cname + " ===")
        for fname, ftype in sorted(fields[cname]):
            print(f"  {fname} : {ftype}")
