/* memdump: freestanding physical RAM dumper via mmap2 + /dev/mem.
   ARM EABI syscalls only, no libc (camera libc.so is malformed for lld).
   Usage: memdump <phys_hex> <count_hex> <outfile>           */

typedef unsigned int uint32_t;
typedef unsigned char uint8_t;

#define SYS_exit   1
#define SYS_write  4
#define SYS_open   5
#define SYS_close  6
#define SYS_mmap2  192

static long sys6(long n, long a, long b, long c, long d, long e, long f) {
    register long r0v __asm__("r0") = a;
    register long r1v __asm__("r1") = b;
    register long r2v __asm__("r2") = c;
    register long r3v __asm__("r3") = d;
    register long r4v __asm__("r4") = e;
    register long r5v __asm__("r5") = f;
    register long r7v __asm__("r7") = n;
    __asm__ volatile("svc #0" : "+r"(r0v) :
        "r"(r1v), "r"(r2v), "r"(r3v), "r"(r4v), "r"(r5v), "r"(r7v) : "memory");
    return r0v;
}

static long sys(long n, long a, long b, long c, long d) {
    register long r0 __asm__("r0") = a;
    register long r1 __asm__("r1") = b;
    register long r2 __asm__("r2") = c;
    register long r3 __asm__("r3") = d;
    register long r7 __asm__("r7") = n;
    __asm__ volatile("svc #0" : "+r"(r0) : "r"(r1), "r"(r2), "r"(r3), "r"(r7) : "memory");
    return r0;
}

static void puts_(const char* s) {
    int n = 0; while (s[n]) n++;
    sys(SYS_write, 1, (long)s, n, 0);
}

static void puthex(unsigned long v) {
    char b[11]; b[0]='0'; b[1]='x';
    for (int i = 0; i < 8; i++) {
        int nib = (v >> ((7-i)*4)) & 0xF;
        b[2+i] = nib < 10 ? '0'+nib : 'a'+nib-10;
    }
    b[10] = 0;
    puts_(b);
}

static unsigned long parse_hex(const char* s) {
    unsigned long v = 0;
    if (s[0]=='0' && (s[1]=='x'||s[1]=='X')) s += 2;
    while (*s) {
        char c = *s++;
        v = v*16 + (c<='9' ? c-'0' : (c|32)-'a'+10);
    }
    return v;
}

__attribute__((naked)) void _start(void) {
    __asm__ volatile(
        "ldr r0, [sp]\n"
        "add r1, sp, #4\n"
        "bl cmain\n"
        "mov r7, #1\n"
        "svc #0\n"
    );
}

void cmain(unsigned long argc, char** argv) {
    if (argc < 4) { puts_("usage: memdump <phys_hex> <count_hex> <out>\n"); sys(SYS_exit, 1, 0, 0, 0); }

    unsigned long phys  = parse_hex(argv[1]);
    unsigned long count = parse_hex(argv[2]);
    const char* outpath = argv[3];

    int fd = (int)sys(SYS_open, (long)"/dev/mem", 0 /*O_RDONLY*/, 0, 0);
    if (fd < 0) { puts_("[-] open /dev/mem failed\n"); sys(SYS_exit, 1, 0, 0, 0); }

    uint32_t page = phys & ~0xFFFUL;
    uint32_t off  = phys - page;
    uint32_t len  = (uint32_t)count + off;

    /* mmap2: offset in 4KB pages -> handles phys > 2GB fine */
    long r = sys6(SYS_mmap2, 0, len, 1 /*PROT_READ*/, 2 /*MAP_SHARED*/, fd, page >> 12);
    if (r & 0xF0000000UL) {
        puts_("[-] mmap failed ret="); puthex((unsigned long)r); puts_("\n");
        sys(SYS_exit, 2, 0, 0, 0);
    }
    volatile uint8_t* p = (volatile uint8_t*)(r + off);

    int out = (int)sys(SYS_open, (long)outpath, 0x241 /*O_WRONLY|O_CREAT|O_TRUNC*/, 0644, 0);
    if (out < 0) { puts_("[-] cannot create out\n"); sys(SYS_exit, 3, 0, 0, 0); }

    /* debug: params shown */
    puts_("[dbg] phys="); puthex(page); puts_(" len="); puthex(len); puts_(" fd="); puthex((unsigned long)fd); puts_(" pgoff="); puthex(page>>12); puts_("\n");
    puts_("[+] mapped phys="); puthex(page); puts_(" off="); puthex(off); puts_(" len="); puthex(len); puts_("\n");

    /* validate reads first page by touching; then copy in chunks */
    static uint8_t buf[65536];
    uint32_t done = 0;
    while (done < (uint32_t)count) {
        uint32_t chunk = (count - done) > sizeof(buf) ? sizeof(buf) : (uint32_t)(count - done);
        for (uint32_t i = 0; i < chunk; i++) buf[i] = p[done + i];
        long w = sys(SYS_write, out, (long)buf, chunk, 0);
        if (w != (long)chunk) { puts_("[-] short write\n"); sys(SYS_exit, 4, 0, 0, 0); }
        done += chunk;
    }
    sys(SYS_close, out, 0, 0, 0);
    sys(SYS_close, fd, 0, 0, 0);
    puts_("[+] dumped "); puthex(count); puts_(" bytes from phys "); puthex(phys); puts_("\n");
    sys(SYS_exit, 0, 0, 0, 0);
}
