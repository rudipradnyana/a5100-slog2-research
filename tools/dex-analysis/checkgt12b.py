import socket, time, re
s=socket.create_connection(('192.168.68.117',23), timeout=8); s.settimeout(5)
def drain(s):
    out=b''
    try:
        while True:
            d=s.recv(65536)
            if not d: break
            out+=d
    except socket.timeout: pass
    return out
def cmd(s, c, t=6.0):
    s.sendall((c+'\n').encode()); time.sleep(t); return drain(s)
s=fresh=s
drain(s)
r=cmd(s, 'ls -la /android/mnt/sdcard/GAMMALOG.TXT', 4)
print(r.strip())
mm = re.findall(rb'(\d+) Jan  1', r)
size = int(mm[-1]) if mm else 0
print('log size:', size)
r=cmd(s, 'dd if=/android/mnt/sdcard/GAMMALOG.TXT bs=1 skip=%d 2>/dev/null' % (size-900 if size>900 else 0), 7)
print(r.replace(b'\r\n', b'\n').decode(errors='replace')[:1500])
s.close()
