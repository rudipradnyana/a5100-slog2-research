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
    off = struct.unpack_from('<I', data, str_off + idx*4)[0]
    ln, off = uleb128(data, off)
    end = data.find(b'\x00', off)
    if end < 0: end = min(off+ln, len(data))
    return data[off:end].decode('utf-8', errors='replace')

def get_type(idx):
    desc_idx = struct.unpack_from('<I', data, typ_off + idx*4)[0]
    return get_string(desc_idx)

# proto: <shorty_idx:u32, return_idx:u32, params_off:u32>
def get_proto(idx):
    shorty, ret, poff = struct.unpack_from('<III', data, pro_off + idx*12)
    if poff == 0: return "()"
    n = struct.unpack_from('<I', data, poff)[0]
    types = [get_type(struct.unpack_from('<H', data, poff + 4 + 2*k)[0]) for k in range(n)]
    return "(" + ", ".join(types) + ")"

# method_id: <class_idx:u16, proto_idx:u16, name_idx:u32>
for idx in range(mth_sz):
    off = mth_off + idx*8
    cls_idx, proto_idx, name_idx = struct.unpack_from('<HHI', data, off)
    cname = get_type(cls_idx)
    if 'OpenOptions' in cname or 'GammaTable' in cname:
        print(f"{cname}  ->  {get_string(name_idx)} {get_proto(proto_idx)}")

print("=== fields RECORDING/MOVIE mode ===")
# field_id: <class_idx:u16, type_idx:u16, name_idx:u32>
for idx in range(fld_sz):
    off = fld_off + idx*8
    cls_idx, typ_idx, name_idx = struct.unpack_from('<HHI', data, off)
    fname = get_string(name_idx)
    if fname in ('RECORDING_MODE_MOVIE','RECORDING_MODE_STILL','RECORDING_MODE_CURRENT','MOVIE_MODE_SINGLE','MOVIE_MODE_MULTI_STREAMING','MOVIE_MODE_CURRENT'):
        print(f"{get_type(cls_idx)}  {fname} : {get_type(typ_idx)}")
