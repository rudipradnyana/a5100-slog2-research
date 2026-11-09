int puts(const char* s);
int printf(const char* fmt, ...);
void exit(int code);

void* memcpy(void* dst, const void* src, unsigned long n) {
    char* d = (char*)dst; const char* s = (const char*)src;
    while (n--) *d++ = *s++; return dst;
}

#define SYS_ptrace  26
#define SYS_wait4   114
#define SYS_exit    1
#define PTRACE_ATTACH    16
#define PTRACE_DETACH    17
#define PTRACE_PEEKDATA  2
#define PTRACE_POKEDATA  5
#define PTRACE_GETREGS   12
#define PTRACE_SETREGS   13
#define PTRACE_CONT      7
#define __WALL           0x40000000
#define WIFSTOPPED(s)    (((s) & 0xff) == 0x7f)
#define WSTOPSIG(s)      (((s) >> 8) & 0xff)

typedef unsigned int uint32_t;
typedef struct { uint32_t uregs[18]; } arm_regs_t;
#define REG_R0 0
#define REG_R1 1
#define REG_R2 2
#define REG_SP 13
#define REG_LR 14
#define REG_PC 15
#define REG_CPSR 16
#define REG_ORIG_R0 17

static long syscall4(long n, long a, long b, long c, long d) {
    register long r0 __asm__("r0") = a;
    register long r1 __asm__("r1") = b;
    register long r2 __asm__("r2") = c;
    register long r3 __asm__("r3") = d;
    register long r7 __asm__("r7") = n;
    __asm__ volatile("swi #0" : "+r"(r0) : "r"(r1),"r"(r2),"r"(r3),"r"(r7) : "memory");
    return r0;
}
static long my_waitpid(int pid, int* status, int options) {
    return syscall4(SYS_wait4, pid, (long)status, options, 0);
}
static int write_mem(int pid, uint32_t addr, const void* data, int len) {
    const uint32_t* p = (const uint32_t*)data;
    int words = (len + 3) / 4;
    for (int i = 0; i < words; i++) {
        long r = syscall4(SYS_ptrace, PTRACE_POKEDATA, pid, addr + i*4, p[i]);
        if (r < 0) return -1;
    }
    return 0;
}

#define LIBC_BASE         0x04251000UL
#define GETPID_ADDR       (LIBC_BASE + 0x0000b660UL)
#define SCALAR_BASE       0x12e4f000UL
#define CTOR_PP_ADDR      (SCALAR_BASE + 0x117eb8UL)
#define SET_PP_ADDR       (SCALAR_BASE + 0x118248UL)

#define VALUEID_PP_GAMMA_SLOG2    1165
#define VALUEID_PP_MODE_ON        1306

static int remote_call(int pid, uint32_t target_func, int is_thumb,
                       uint32_t arg0, uint32_t arg1, uint32_t arg2,
                       uint32_t sp, uint32_t* out_r0) {
    arm_regs_t regs;
    syscall4(SYS_ptrace, PTRACE_GETREGS, pid, 0, (long)&regs);
    regs.uregs[REG_R0] = arg0;
    regs.uregs[REG_R1] = arg1;
    regs.uregs[REG_R2] = arg2;
    regs.uregs[REG_SP] = sp & ~7;
    regs.uregs[REG_LR] = 0;
    regs.uregs[REG_PC] = target_func & ~1;
    regs.uregs[REG_ORIG_R0] = 0xffffffff;
    if (is_thumb) regs.uregs[REG_CPSR] |= 0x20; else regs.uregs[REG_CPSR] &= ~0x20;
    if (syscall4(SYS_ptrace, PTRACE_SETREGS, pid, 0, (long)&regs) < 0) return -1;
    syscall4(SYS_ptrace, PTRACE_CONT, pid, 0, 0);
    int status = 0;
    while (1) {
        long w = my_waitpid(pid, &status, __WALL);
        if (w > 0 && WIFSTOPPED(status)) {
            int sig = WSTOPSIG(status);
            if (sig == 11 || sig == 7) {
                arm_regs_t rr;
                syscall4(SYS_ptrace, PTRACE_GETREGS, pid, 0, (long)&rr);
                if (out_r0) *out_r0 = rr.uregs[REG_R0];
                return 0;
            }
            syscall4(SYS_ptrace, PTRACE_CONT, pid, 0, 0);
        }
    }
}

void _start() {
    int pid = 149;
    puts("=== slog2_full: gamma(+0x88) AND valueid(+0x120) pairs ===");
    if (syscall4(SYS_ptrace, PTRACE_ATTACH, pid, 0, 0) < 0) { puts("ATTACH fail"); exit(1); }
    int status = 0;
    while (1) { long w = my_waitpid(pid, &status, __WALL); if (w > 0 && WIFSTOPPED(status)) break; }
    arm_regs_t orig;
    syscall4(SYS_ptrace, PTRACE_GETREGS, pid, 0, (long)&orig);
    uint32_t sp = (orig.uregs[REG_SP] - 0x1000) & ~7;
    uint32_t st = sp + 0x400;

    uint32_t dummy = 0;
    if (remote_call(pid, GETPID_ADDR, 0, 0, 0, 0, sp, &dummy) == 0)
        printf("getpid check: %d\n", dummy);

    if (remote_call(pid, CTOR_PP_ADDR, 1, st, 0, 0, sp, &dummy) != 0) { puts("CTOR fail"); goto out; }
    puts("ctor ok");

    uint32_t pair0[2] = { 1, 1 };
    uint32_t pairG[2] = { VALUEID_PP_GAMMA_SLOG2, 1 };
    uint32_t pairV[2] = { VALUEID_PP_GAMMA_SLOG2, 1 };
    uint32_t pairM[2] = { VALUEID_PP_MODE_ON, 1 };
    write_mem(pid, st + 0x00, pair0, 8);
    write_mem(pid, st + 0x88, pairG, 8);
    write_mem(pid, st + 0x120, pairV, 8);
    /* PP MODE ON: which slot? try +0x08 (param 0x1f8) and +0x10 (0x1fc) as mode candidates */
    write_mem(pid, st + 0x08, pairM, 8);
    write_mem(pid, st + 0x10, pairM, 8);

    uint32_t ret = 999;
    if (remote_call(pid, SET_PP_ADDR, 1, st, 0, 0, sp, &ret) == 0)
        printf("setPictureProfile ret=%d (0x%x)\n", ret, ret);
    else
        puts("setPictureProfile RPC fail");

out:
    syscall4(SYS_ptrace, PTRACE_SETREGS, pid, 0, (long)&orig);
    syscall4(SYS_ptrace, PTRACE_DETACH, pid, 0, 0);
    puts("done");
    exit(0);
}
