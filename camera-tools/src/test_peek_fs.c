int puts(const char* s);
int printf(const char* fmt, ...);
void exit(int code);

#define SYS_ptrace  26
#define SYS_waitpid 7
#define SYS_exit    1

#define PTRACE_ATTACH   16
#define PTRACE_DETACH   17
#define PTRACE_PEEKDATA 2

static long syscall4(long n, long a, long b, long c, long d) {
    register long r0 __asm__("r0") = a;
    register long r1 __asm__("r1") = b;
    register long r2 __asm__("r2") = c;
    register long r3 __asm__("r3") = d;
    register long r7 __asm__("r7") = n;
    __asm__ volatile("swi #0" : "+r"(r0) : "r"(r1),"r"(r2),"r"(r3),"r"(r7) : "memory");
    return r0;
}

static long syscall3(long n, long a, long b, long c) {
    register long r0 __asm__("r0") = a;
    register long r1 __asm__("r1") = b;
    register long r2 __asm__("r2") = c;
    register long r7 __asm__("r7") = n;
    __asm__ volatile("swi #0" : "+r"(r0) : "r"(r1),"r"(r2),"r"(r7) : "memory");
    return r0;
}

void _start() {
    int pid = 149;
    long r = syscall4(SYS_ptrace, PTRACE_ATTACH, pid, 0, 0);
    if (r < 0) {
        printf("ATTACH failed: %ld\n", r);
        exit(1);
    }
    int status = 0;
    syscall3(SYS_waitpid, pid, (long)&status, 0);

    /* In Linux kernel sys_ptrace:
     * generic_ptrace_peekdata(child, addr, data)
     * writes the word to (void*)data!
     */
    unsigned int val = 0;
    r = syscall4(SYS_ptrace, PTRACE_PEEKDATA, pid, 0x12f67248, (long)&val);
    printf("PEEK 0x12f67248 ret=%ld val=0x%08x\n", r, val);

    syscall4(SYS_ptrace, PTRACE_DETACH, pid, 0, 0);
    exit(0);
}

