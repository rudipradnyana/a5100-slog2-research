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

#define REG_R0       0
#define REG_R1       1
#define REG_R2       2
#define REG_R3       3
#define REG_SP       13
#define REG_LR       14
#define REG_PC       15
#define REG_CPSR     16
#define REG_ORIG_R0  17

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

static int read_mem(int pid, uint32_t addr, void* data, int len) {
    uint32_t* p = (uint32_t*)data;
    int words = (len + 3) / 4;
    for (int i = 0; i < words; i++) {
        uint32_t val = 0;
        long r = syscall4(SYS_ptrace, PTRACE_PEEKDATA, pid, addr + i*4, (long)&val);
        if (r < 0) return -1;
        p[i] = val;
    }
    return 0;
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

/* Constants for PID 149 */
#define LIBC_BASE         0x04251000UL
#define GETPID_ADDR       (LIBC_BASE + 0x0000b660UL)  /* ARM mode */

#define SCALAR_BASE       0x12e4f000UL
#define CTOR_PP_ADDR      (SCALAR_BASE + 0x117eb8UL)  /* Thumb: 0x117eb9 */
#define SET_PP_ADDR       (SCALAR_BASE + 0x118248UL)  /* Thumb: 0x118249 */
#define RESET_PP_ADDR     (SCALAR_BASE + 0x118830UL)  /* Thumb: 0x118831 */

/* Value IDs */
#define VALUEID_PP_GAMMA_SLOG2    1165  /* 0x48d */
#define VALUEID_PP_COLOR_SGAMUT   1298  /* 0x512 */

static int remote_call(int pid, uint32_t target_func, int is_thumb,
                       uint32_t arg0, uint32_t arg1, uint32_t arg2,
                       uint32_t sp, uint32_t* out_r0) {
    arm_regs_t regs;
    syscall4(SYS_ptrace, PTRACE_GETREGS, pid, 0, (long)&regs);

    regs.uregs[REG_R0] = arg0;
    regs.uregs[REG_R1] = arg1;
    regs.uregs[REG_R2] = arg2;
    regs.uregs[REG_SP] = sp & ~7;
    regs.uregs[REG_LR] = 0; /* NULL return -> faults with SIGSEGV */
    regs.uregs[REG_PC] = target_func & ~1;
    regs.uregs[REG_ORIG_R0] = 0xffffffff;

    if (is_thumb) {
        regs.uregs[REG_CPSR] |= 0x20;
    } else {
        regs.uregs[REG_CPSR] &= ~0x20;
    }

    long r = syscall4(SYS_ptrace, PTRACE_SETREGS, pid, 0, (long)&regs);
    if (r < 0) {
        printf("SETREGS failed: %ld\n", r);
        return -1;
    }

    /* Resume tracee */
    syscall4(SYS_ptrace, PTRACE_CONT, pid, 0, 0);

    /* Wait for return fault */
    int status = 0;
    while (1) {
        long w = my_waitpid(pid, &status, __WALL);
        if (w > 0 && WIFSTOPPED(status)) {
            int sig = WSTOPSIG(status);
            if (sig == 11 || sig == 7) { /* SIGSEGV or SIGBUS from PC=0 */
                arm_regs_t ret_regs;
                syscall4(SYS_ptrace, PTRACE_GETREGS, pid, 0, (long)&ret_regs);
                if (out_r0) *out_r0 = ret_regs.uregs[REG_R0];
                return 0;
            }
            /* Other unexpected signal: forward or resume */
            printf("Received signal %d while waiting for return\n", sig);
            syscall4(SYS_ptrace, PTRACE_CONT, pid, 0, 0);
        }
    }
}

void _start() {
    int pid = 149;
    puts("=== Sony A5100 S-Log2 Zero-Code Live Injector ===");

    /* 1. ATTACH */
    long r = syscall4(SYS_ptrace, PTRACE_ATTACH, pid, 0, 0);
    if (r < 0) {
        printf("ATTACH to PID %d failed: %ld\n", pid, r);
        exit(1);
    }

    int status = 0;
    while (1) {
        long w = my_waitpid(pid, &status, __WALL);
        if (w > 0 && WIFSTOPPED(status)) break;
    }
    printf("Attached to PID %d. Status=0x%04x (sig=%d)\n", pid, status, WSTOPSIG(status));

    /* 2. SAVE ORIGINAL REGS */
    arm_regs_t orig_regs;
    syscall4(SYS_ptrace, PTRACE_GETREGS, pid, 0, (long)&orig_regs);
    printf("Original: PC=0x%08x LR=0x%08x SP=0x%08x CPSR=0x%08x\n",
           orig_regs.uregs[REG_PC], orig_regs.uregs[REG_LR],
           orig_regs.uregs[REG_SP], orig_regs.uregs[REG_CPSR]);

    /* Allocate scratch area safely below remote stack */
    uint32_t scratch_sp = (orig_regs.uregs[REG_SP] - 0x1000) & ~7;
    uint32_t struct_addr = scratch_sp + 0x400;

    /* 3. TEST RPC: CALL getpid() */
    uint32_t ret_pid = 0;
    puts("[TEST] Executing remote getpid()...");
    if (remote_call(pid, GETPID_ADDR, 0, 0, 0, 0, scratch_sp, &ret_pid) == 0) {
        printf("[TEST] remote getpid() returned: %d (expected %d) -> %s\n",
               ret_pid, pid, (ret_pid == pid) ? "OK!" : "MISMATCH!");
    } else {
        puts("[TEST] remote getpid() failed!");
        goto cleanup;
    }

    /* 4. CALL PictureProfileParameter CONSTRUCTOR */
    puts("[STEP 1] Initializing PictureProfileParameter on stack via C1 constructor...");
    uint32_t dummy = 0;
    if (remote_call(pid, CTOR_PP_ADDR, 1, struct_addr, 0, 0, scratch_sp, &dummy) == 0) {
        puts("[STEP 1] PictureProfileParameter constructor completed OK!");
    } else {
        puts("[STEP 1] Constructor call failed!");
        goto cleanup;
    }

    /* 5. OVERRIDE FIELDS: PP slot 1, Gamma = VALUEID_PP_GAMMA_SLOG2 (1165 = 0x48d) */
    puts("[STEP 2] Setting PP=1, Gamma=1165 (VALUEID_PP_GAMMA_SLOG2)...");
    uint32_t patch_pp[2] = { 1, 1 };      /* [0x00]=1, [0x04]=1 (valid) */
    uint32_t patch_gamma[2] = { 1165, 1 }; /* [0x88]=1165 (S-Log2), [0x8c]=1 (valid) */

    write_mem(pid, struct_addr + 0x00, patch_pp, sizeof(patch_pp));
    write_mem(pid, struct_addr + 0x88, patch_gamma, sizeof(patch_gamma));

    /* 6. CALL setPictureProfile(struct_addr) */
    puts("[STEP 3] Calling setPictureProfile(PictureProfileParameter*)...");
    uint32_t set_ret = 999;
    if (remote_call(pid, SET_PP_ADDR, 1, struct_addr, 0, 0, scratch_sp, &set_ret) == 0) {
        printf("[STEP 3] setPictureProfile returned: %d (0x%x)\n", set_ret, set_ret);
        if (set_ret == 0) {
            puts("*****************************************************************");
            puts("* SUCCESS: setPictureProfile(S-Log2) RETURNED 0!               *");
            puts("* S-Log2 Gamma Event Successfully Pushed to Camera DSP!         *");
            puts("*****************************************************************");
        }
    } else {
        puts("[STEP 3] setPictureProfile RPC call failed!");
    }

cleanup:
    /* 7. RESTORE REGISTERS AND DETACH */
    puts("Restoring original CPU registers...");
    syscall4(SYS_ptrace, PTRACE_SETREGS, pid, 0, (long)&orig_regs);

    puts("Detaching from PID 149...");
    syscall4(SYS_ptrace, PTRACE_DETACH, pid, 0, 0);

    puts("=== Finished. PID 149 resumed normally! ===");
    exit(0);
}

