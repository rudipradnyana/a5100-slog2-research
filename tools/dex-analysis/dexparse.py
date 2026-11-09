import struct, sys

raw = open('/Users/rudipradnyana/a5100-re/native/sony.cameraex.odex','rb').read()
i = raw.find(b'dex\n035')
data = raw[i:]
print("dex at", hex(i), "len", len(data))

# DEX header per reference: string_ids @0x38, type_ids @0x40, proto_ids @0x48, field_ids @0x50, method_ids @0x58, class_defs @0x60
str_sz, str_off = struct.unpack_from('<II', data, 0x38)
typ_sz, typ_off = struct.unpack_from('<II', data, 0x40)
pro_sz, pro_off = struct.unpack_from('<II', data, 0x48)
fld_sz, fld_off = struct.unpack_from('<II', data, 0x50)
mth_sz, mth_off = struct.unpack_from('<II', data, 0x58)
cls_sz, cls_off = struct.unpack_from('<II', data, 0x60)
print(f"strings={str_sz}@{hex(str_off)} types={typ_sz}@{hex(typ_off)} protos={pro_sz}@{hex(pro_off)} fields={fld_sz}@{hex(fld_off)} methods={mth_sz}@{hex(mth_off)} classes={cls_sz}@{hex(cls_off)}")
for n,(s,o) in [("str",(str_sz,str_off)),("typ",(typ_sz,typ_off)),("mth",(mth_sz,mth_off))]:
    if o + s*8 > len(data) and n != "str":
        print(f"WARNING {n} out of bounds")

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

def get_proto(idx):
    if idx >= pro_sz: return "?"
    shorty, ret, params_off = struct.unpack_from('<III', data, pro_off + idx*12)
    ret_t = get_type(ret)
    params = []
    if params_off and params_off + 4 <= len(data):
        cnt = struct.unpack_from('<I', data, params_off)[0]
        for k in range(min(cnt, 16)):
            ti = struct.unpack_from('<H', data, params_off + 4 + k*2)[0]
            params.append(get_type(ti))
    return "(" + ",".join(params) + ")" + ret_t

# Dump ALL methods grouped by class
classes = {}
for idx in range(mth_sz):
    off = mth_off + idx*8
    if off + 8 > len(data): break
    cls_idx, proto_idx, name_idx = struct.unpack_from('<HHI', data, off)
    cname = get_type(cls_idx)
    name = get_string(name_idx)
    sig = get_proto(proto_idx)
    classes.setdefault(cname, []).append((name, sig))

# Print classes of interest
INTEREST = ['CameraEx;', 'ParametersModifier', 'GammaTable', 'DeviceBuffer', 'MediaRecorder', 'CamcorderProfile', 'DeviceMemory']
for cname in sorted(classes):
    if any(k in cname for k in INTEREST):
        print("\n=== " + cname + " ===")
        for name, sig in sorted(classes[cname]):
            print(f"  {name}{sig}")
