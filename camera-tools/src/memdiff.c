/* memdiff: compare two files, print runs of differing bytes.
   Output: "DIFF start=0xOFFSET len=LEN" per merged run (gaps < 64 merged).
   Freestanding ARM, no libc. */
typedef unsigned int uint32_t;
typedef unsigned char uint8_t;
typedef int int32_t;

#define SYS_exit  1
#define SYS_read  3
#define SYS_write 4
#define SYS_open  5
#define SYS_close 6
#define BUFSZ 65536

static long sys(long n, long a, long b, long c, long d) {
    register long r0 __asm__("r0") = a;
    register long r1 __asm__("r1") = b;
    register long r2 __asm__("r2") = c;
    register long r3 __asm__("r3") = d;
    register long r7 __asm__("r7") = n;
    __asm__ volatile("svc #0" : "+r"(r0) : "r"(r1), "r"(r2), "r"(r3), "r"(r7) : "memory");
    return r0;
}

static void puts_(const char* s) { int n=0; while(s[n])n++; sys(SYS_write,1,(long)s,n,0); }
static void puthex(unsigned long v) {
    char b[11]; b[0]='0'; b[1]='x';
    for(int i=0;i<8;i++){int nib=(v>>((7-i)*4))&0xF;b[2+i]=nib<10?'0'+nib:'a'+nib-10;}
    b[10]=0; puts_(b);
}
static void putdec(unsigned long v) {
    char b[12]; int i=11; b[i]=0;
    if(!v){b[--i]='0';}
    while(v){b[--i]='0'+v%10;v/=10;}
    puts_(&b[i]);
}

static unsigned char fa[BUFSZ], fb[BUFSZ];

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
    if (argc < 3) { puts_("usage: memdiff <fileA> <fileB>\n"); sys(SYS_exit,1,0,0,0); }

    int a = (int)sys(SYS_open,(long)argv[1],0,0,0);
    int b = (int)sys(SYS_open,(long)argv[2],0,0,0);
    if (a<0||b<0){ puts_("[-] open failed\n"); sys(SYS_exit,1,0,0,0); }

    unsigned long off = 0;          /* global file offset of buf start */
    unsigned long pos = 0;          /* offset within current buffers */
    long na, nb;
    int inrun = 0; unsigned long runstart = 0, runlen = 0;
    int ndiff = 0;

    for (;;) {
        na = sys(SYS_read,a,(long)fa,BUFSZ,0);
        nb = sys(SYS_read,b,(long)fb,BUFSZ,0);
        if (na <= 0 && nb <= 0) break;
        long n = na < nb ? na : nb;
        if (n < 0) n = 0;
        for (long i = 0; i < n; i++) {
            if (fa[i] != fb[i]) {
                if (!inrun) { inrun = 1; runstart = off + i; runlen = 1; }
                else runlen = (off + i) - runstart + 1;
            } else if (inrun) {
                /* close run only when gap > 64 bytes: peek ahead is complex; use lazy close with pending gap */
                /* simpler: close immediately but merge by tracking lastdiff */
                inrun = 2; /* pending close */
            }
            if (inrun == 2) {
                if (fa[i] != fb[i]) inrun = 1;
                else {
                    unsigned long gap = (off + i) - (runstart + runlen);
                    if (gap > 64) {
                        puts_("DIFF start="); puthex(runstart);
                        puts_(" len="); putdec(runlen); puts_("\n");
                        ndiff++;
                        inrun = 0;
                    }
                }
            }
        }
        off += n;
        if (na <= 0 || nb <= 0) break;
    }
    if (inrun) {
        puts_("DIFF start="); puthex(runstart);
        puts_(" len="); putdec(runlen); puts_("\n");
        ndiff++;
    }
    puts_("TOTAL diffs="); putdec(ndiff); puts_(" compared="); putdec(off); puts_("\n");
    sys(SYS_close,a,0,0,0); sys(SYS_close,b,0,0,0);
    sys(SYS_exit,0,0,0,0);
}
