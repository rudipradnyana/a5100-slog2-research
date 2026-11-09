import socket, time, sys

import os
DEFAULT_IP = os.environ.get("A5100_IP", "192.168.68.104")  # set A5100_IP or pass IP=x.x.x.x

def run_cmd(cmd, ip=DEFAULT_IP, timeout=30):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(timeout)
    try:
        s.connect((ip, 23))
        buf = b""
        start = time.time()
        while b"/ # " not in buf and time.time() - start < 5.0:
            try:
                chunk = s.recv(1024)
                if not chunk:
                    break
                buf += chunk
            except socket.timeout:
                break
        s.sendall((cmd + "\n").encode('utf-8'))
        out = b""
        start = time.time()
        while time.time() - start < timeout:
            try:
                chunk = s.recv(4096)
                if not chunk:
                    break
                out += chunk
                if b"/ # " in out:
                    break
            except socket.timeout:
                break
        res = out.decode('utf-8', errors='replace')
        lines = res.splitlines()
        clean = []
        for line in lines:
            ls = line.strip()
            if ls == '/ #' or ls.startswith(cmd[:20]):
                continue
            clean.append(line)
        return "\n".join(clean).strip()
    finally:
        s.close()

if __name__ == '__main__':
    args = sys.argv[1:]
    ip = DEFAULT_IP
    if args and args[0].startswith("IP="):
        ip = args[0][3:]
        args = args[1:]
    timeout = 30
    if args and args[-1].isdigit():
        timeout = int(args[-1])
        args = args[:-1]
    c = " ".join(args) if args else "uname -a"
    print(run_cmd(c, ip=ip, timeout=timeout))
