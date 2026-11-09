import socket, time
def fresh():
    s=socket.create_connection(('192.168.68.117',23), timeout=8); s.settimeout(5)
    return s
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
s=fresh(); drain(s)
r=cmd(s, 'ls /private/sdcard/PRIVATE/M4ROOT/CLIP/ 2>/dev/null | cat || ls /android/mnt/sdcard/PRIVATE/M4ROOT/CLIP/ | cat', 6)
names=[l.strip() for l in r.replace(b'\r\n',b'\n').decode(errors='replace').splitlines() if l.strip().endswith('.MP4')]
print('clips:', len(names), 'last2:', names[-2:])
newest = names[-1]
r=cmd(s, 'ls -la /android/mnt/sdcard/PRIVATE/M4ROOT/CLIP/%s' % newest, 4)
print(r.strip())
s.close()
