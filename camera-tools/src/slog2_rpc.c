int puts(const char* s);
int printf(const char* fmt, ...);
void exit(int code);

void* memcpy(void* dst, const void* src, unsigned long n) {
    char* d = (char*)dst;
    const char* s = (const char*)src;
    while (n--) *d++ = *s++;
    return dst;
}

void* memset(void* dst, int c, unsigned long n) {
    char* d = (char*)dst;
    while (n--) *d++ = (char)c;
    return dst;
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
typedef struct {
    uint32_t uregs[18];
} arm_regs_t;

#define REG_R0   0
#define REG_R1   1
#define REG_R2   2
#define REG_R3   3
#define REG_SP   13
#define REG_LR   14
#define REG_PC   15
#define REG_CPSR 16

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

/* Addresses in PID 149 */
#define SCALAR_BASE    0x12e4f000UL
#define SET_PP_OFFSET  0x118248UL
#define SET_PP_ADDR    (SCALAR_BASE + SET_PP_OFFSET)  /* 0x12f67248 */
#define BKPT_ADDR      (SCALAR_BASE + 0xa2ccUL)       /* 0x12e592cc (bkpt #0x1f) */

static int write_mem(int pid, uint32_t addr, const void* data, int len) {
    const uint32_t* p = (const uint32_t*)data;
    int words = (len + 3) / 4;
    for (int i = 0; i < words; i++) {
        long r = syscall4(SYS_ptrace, PTRACE_POKEDATA, pid, addr + i*4, p[i]);
        if (r < 0) {
            printf("POKEDATA failed at 0x%08x: %ld\n", addr + i*4, r);
            return -1;
        }
    }
    return 0;
}

void _start() {
    int pid = 149;
    puts("=== LIVE PID 149 S-Log2 Remote Procedure Call ===");

    /* 1. ATTACH */
    long r = syscall4(SYS_ptrace, PTRACE_ATTACH, pid, 0, 0);
    if (r < 0) {
        printf("PTRACE_ATTACH failed: %ld\n", r);
        exit(1);
    }
    int status = 0;
    for (int retry = 0; retry < 50; retry++) {
        long w = my_waitpid(pid, &status, __WALL);
        if (w > 0 && WIFSTOPPED(status)) break;
    }
    printf("Attached to PID %d. Stopped status=0x%04x (sig=%d)\n",
           pid, status, WSTOPSIG(status));

    /* 2. GET ORIGINAL REGS */
    arm_regs_t orig_regs;
    r = syscall4(SYS_ptrace, PTRACE_GETREGS, pid, 0, (long)&orig_regs);
    if (r < 0) {
        printf("GETREGS failed: %ld\n", r);
        syscall4(SYS_ptrace, PTRACE_DETACH, pid, 0, 0);
        exit(2);
    }
    printf("Original: PC=0x%08x LR=0x%08x SP=0x%08x CPSR=0x%08x\n",
           orig_regs.uregs[REG_PC], orig_regs.uregs[REG_LR],
           orig_regs.uregs[REG_SP], orig_regs.uregs[REG_CPSR]);

    /* 3. PREPARE PictureProfileParameter STRUCT ON REMOTE STACK */
    /* Size: 0x160 bytes. Move SP down by 0x200 bytes */
    uint32_t struct_addr = (orig_regs.uregs[REG_SP] - 0x200) & ~7;
    uint32_t buf[0x160 / 4];
    for (int i = 0; i < 0x160 / 4; i++) buf[i] = 0;

    /* PP index = 1 (slot PP1) */
    buf[0x00 / 4] = 1;
    /* flag byte at +0x04 */
    buf[0x04 / 4] = 1;

    /* Gamma type at +0x88 = VALUEID_PP_GAMMA_SLOG2 (1165 = 0x48d) */
    buf[0x88 / 4] = 1165;
    /* flag byte at +0x8c */
    buf[0x8c / 4] = 1;

    printf("Writing PictureProfileParameter (PP=1, Gamma=1165 [0x48d / S-Log2]) to 0x%08x...\n", struct_addr);
    if (write_mem(pid, struct_addr, buf, sizeof(buf)) < 0) {
        puts("Failed to write struct to remote stack!");
        syscall4(SYS_ptrace, PTRACE_DETACH, pid, 0, 0);
        exit(3);
    }

    /* 4. SET UP REGISTERS FOR REMOTE CALL */
    arm_regs_t call_regs = orig_regs;
    call_regs.uregs[REG_R0]   = struct_addr;         /* param: PictureProfileParameter* */
    call_regs.uregs[REG_SP]   = struct_addr;         /* SP at allocated area */
    call_regs.uregs[REG_LR]   = (BKPT_ADDR & ~1) | 1;/* Return address -> BKPT (Thumb) */
    call_regs.uregs[REG_PC]   = SET_PP_ADDR & ~1;    /* Target func: setPictureProfile (even address) */
    call_regs.uregs[REG_CPSR] |= 0x20;               /* Thumb mode (T bit = 1) */

    printf("Calling setPictureProfile: PC=0x%08x LR=0x%08x R0=0x%08x...\n",
           call_regs.uregs[REG_PC], call_regs.uregs[REG_LR], call_regs.uregs[REG_R0]);

    r = syscall4(SYS_ptrace, PTRACE_SETREGS, pid, 0, (long)&call_regs);
    if (r < 0) {
        printf("SETREGS failed: %ld\n", r);
        syscall4(SYS_ptrace, PTRACE_DETACH, pid, 0, 0);
        exit(4);
    }

    /* 5. RESUME THREAD & WAIT FOR BKPT (SIGTRAP = 5) */
    puts("Resuming PID 149...");
    syscall4(SYS_ptrace, PTRACE_CONT, pid, 0, 0);

    status = 0;
    while (1) {
        long w = my_waitpid(pid, &status, __WALL);
        if (w > 0) {
            int sig = WSTOPSIG(status);
            printf("Received signal %d (status=0x%04x)\n", sig, status);
            if (sig == 5) {
                puts("Hit BKPT (SIGTRAP)!");
                break;
            }
            if (sig == 11) {
                puts("Segmentation fault (SIGSEGV) in target!");
                break;
            }
            /* Other signals: forward 0 and continue */
            syscall4(SYS_ptrace, PTRACE_CONT, pid, 0, 0);
        }
    }
    printf("PID 149 finished execution. Final status=0x%04x\n", status);

    /* 6. INSPECT RESULT */
    arm_regs_t ret_regs;
    syscall4(SYS_ptrace, PTRACE_GETREGS, pid, 0, (long)&ret_regs);
    printf("Return PC=0x%08x, R0 (Return Value)=%d (0x%x)\n",
           ret_regs.uregs[REG_PC], ret_regs.uregs[REG_R0], ret_regs.uregs[REG_R0]);

    if (ret_regs.uregs[REG_R0] == 0) {
        puts(">>> SUCCESS! setPictureProfile returned 0 (EVENT DISPATCHED TO CAMERA)! <<<");
    } else {
        printf("setPictureProfile returned %d\n", ret_regs.uregs[REG_R0]);
    }

    /* 7. RESTORE ORIGINAL STATE & DETACH */
    puts("Restoring original registers...");
    syscall4(SYS_ptrace, PTRACE_SETREGS, pid, 0, (long)&orig_regs);

    puts("Detaching from PID 149...");
    syscall4(SYS_ptrace, PTRACE_DETACH, pid, 0, 0);

    puts("=== Remote Call Complete! Check Camera LCD Now! ===");
    exit(0);
}
