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

/* Base and offsets in PID 149 */
#define SCALAR_BASE       0x12e4f000UL
#define SET_PP_OFFSET     0x118248UL
#define SET_PP_ADDR       (SCALAR_BASE + SET_PP_OFFSET)       /* 0x12f67248 */
#define SET_PP_THUMB      (SET_PP_ADDR | 1)                   /* 0x12f67249 */
#define TRAMPOLINE_ADDR   (SCALAR_BASE + 0x118840UL)          /* 0x12f67840 (code page) */

void _start() {
    int pid = 149;
    puts("=== Sony A5100 S-Log2 Live Injector (Clean Trampoline) ===");

    /* 1. ATTACH */
    long r = syscall4(SYS_ptrace, PTRACE_ATTACH, pid, 0, 0);
    if (r < 0) {
        printf("ATTACH failed: %ld\n", r);
        exit(1);
    }
    int status = 0;
    while (1) {
        long w = my_waitpid(pid, &status, __WALL);
        if (w > 0 && WIFSTOPPED(status)) break;
    }
    printf("Attached to PID %d. Status=0x%04x (sig=%d)\n",
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

    /* 3. WRITE PictureProfileParameter ON REMOTE STACK */
    uint32_t struct_addr = (orig_regs.uregs[REG_SP] - 0x200) & ~7;
    uint32_t pp_buf[0x160 / 4];
    memset(pp_buf, 0, sizeof(pp_buf));

    /* PP index = 1 (slot PP1) */
    pp_buf[0x00 / 4] = 1;
    pp_buf[0x04 / 4] = 1;

    /* Gamma type = VALUEID_PP_GAMMA_SLOG2 (1165 = 0x48d) */
    pp_buf[0x88 / 4] = 1165;
    pp_buf[0x8c / 4] = 1;

    printf("Writing PictureProfileParameter (PP=1, Gamma=1165 [0x48d / S-Log2]) to stack 0x%08x...\n", struct_addr);
    if (write_mem(pid, struct_addr, pp_buf, sizeof(pp_buf)) < 0) {
        puts("Failed to write struct to remote stack!");
        syscall4(SYS_ptrace, PTRACE_DETACH, pid, 0, 0);
        exit(3);
    }

    /* 4. BACKUP 24 BYTES AT TRAMPOLINE CODE LOCATION */
    uint32_t backup_bytes[6];
    if (read_mem(pid, TRAMPOLINE_ADDR, backup_bytes, sizeof(backup_bytes)) < 0) {
        puts("Failed to backup code memory!");
        syscall4(SYS_ptrace, PTRACE_DETACH, pid, 0, 0);
        exit(4);
    }
    printf("Backed up 24 bytes from 0x%08x\n", (uint32_t)TRAMPOLINE_ADDR);

    /* 5. WRITE ARM TRAMPOLINE AT TRAMPOLINE_ADDR (in executable code segment) */
    uint32_t tramp[6] = {
        0xe59f0008,          /* 0x00: ldr r0, [pc, #8] (struct_addr) */
        0xe59fc008,          /* 0x04: ldr ip, [pc, #8] (SET_PP_THUMB) */
        0xe12fff3c,          /* 0x08: blx ip           (calls setPictureProfile!) */
        0xe1200070,          /* 0x0c: bkpt #0          (traps in ARM mode) */
        struct_addr,         /* 0x10: struct_addr */
        (uint32_t)SET_PP_THUMB /* 0x14: SET_PP_THUMB */
    };
    if (write_mem(pid, TRAMPOLINE_ADDR, tramp, sizeof(tramp)) < 0) {
        puts("Failed to write trampoline!");
        syscall4(SYS_ptrace, PTRACE_DETACH, pid, 0, 0);
        exit(5);
    }
    puts("Trampoline written to executable memory OK");

    /* 6. SET UP REGISTERS TO RUN TRAMPOLINE */
    arm_regs_t call_regs = orig_regs;
    call_regs.uregs[REG_PC]      = TRAMPOLINE_ADDR;   /* Run trampoline in ARM mode */
    call_regs.uregs[REG_SP]      = struct_addr;       /* SP below struct */
    call_regs.uregs[REG_CPSR]    = orig_regs.uregs[REG_CPSR] & ~0x20; /* ARM mode (T=0) */
    call_regs.uregs[REG_ORIG_R0] = 0xffffffff;        /* Prevent syscall restart */

    r = syscall4(SYS_ptrace, PTRACE_SETREGS, pid, 0, (long)&call_regs);
    if (r < 0) {
        printf("SETREGS failed: %ld\n", r);
        write_mem(pid, TRAMPOLINE_ADDR, backup_bytes, sizeof(backup_bytes));
        syscall4(SYS_ptrace, PTRACE_DETACH, pid, 0, 0);
        exit(6);
    }

    /* 7. RESUME AND WAIT FOR BKPT */
    puts("Resuming PID 149 to execute trampoline...");
    syscall4(SYS_ptrace, PTRACE_CONT, pid, 0, 0);

    status = 0;
    while (1) {
        long w = my_waitpid(pid, &status, __WALL);
        if (w > 0) {
            int sig = WSTOPSIG(status);
            printf("Received signal %d (status=0x%04x)\n", sig, status);
            if (sig == 5) {
                puts(">>> Hit BKPT #0 (SIGTRAP)! Execution completed! <<<");
                break;
            }
            if (sig == 11) {
                puts("Segmentation fault (SIGSEGV)!");
                break;
            }
            /* Discard other signals and resume */
            syscall4(SYS_ptrace, PTRACE_CONT, pid, 0, 0);
        }
    }

    /* 8. INSPECT RETURN VALUE IN R0 */
    arm_regs_t ret_regs;
    syscall4(SYS_ptrace, PTRACE_GETREGS, pid, 0, (long)&ret_regs);
    printf("Return PC=0x%08x, R0 (Return Value)=%d (0x%x)\n",
           ret_regs.uregs[REG_PC], ret_regs.uregs[REG_R0], ret_regs.uregs[REG_R0]);

    if (ret_regs.uregs[REG_R0] == 0) {
        puts("************************************************************");
        puts("* SUCCESS: setPictureProfile(PP=1, S-Log2) RETURNED 0!      *");
        puts("* S-Log2 Gamma Event Successfully Processed by Camera DSP! *");
        puts("************************************************************");
    } else {
        printf("setPictureProfile returned: %d\n", ret_regs.uregs[REG_R0]);
    }

    /* 9. RESTORE ORIGINAL CODE & REGISTERS */
    puts("Restoring original 24 bytes of code memory...");
    write_mem(pid, TRAMPOLINE_ADDR, backup_bytes, sizeof(backup_bytes));

    puts("Restoring original CPU registers...");
    syscall4(SYS_ptrace, PTRACE_SETREGS, pid, 0, (long)&orig_regs);

    puts("Detaching from PID 149...");
    syscall4(SYS_ptrace, PTRACE_DETACH, pid, 0, 0);

    puts("=== Done. PID 149 restored and running normally! ===");
    exit(0);
}

