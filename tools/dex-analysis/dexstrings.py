import struct
raw = open('/Users/rudipradnyana/a5100-re/native/sony.cameraex.odex','rb').read()
i = raw.find(b'dex\n035')
data = raw[i:]
str_sz, str_off = struct.unpack_from('<II', data, 0x38)
def uleb128(buf, off):
    result = 0; shift = 0
    while True:
        b = buf[off]; off += 1
        result |= (b & 0x7f) << shift
        if not (b & 0x80): break
        shift += 7
    return result, off
out = []
for idx in range(str_sz):
    off = struct.unpack_from('<I', data, str_off + idx*4)[0]
    ln, off = uleb128(data, off)
    end = data.find(b'\x00', off)
    if end < 0: end = min(off+ln, len(data))
    out.append(data[off:end].decode('utf-8', errors='replace'))
open('/tmp/dexstrings.txt','w').write('\n'.join(out))
print(f"dumped {len(out)} strings")
