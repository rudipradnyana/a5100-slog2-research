/*
 * slog2_ptrace.c — Inject setPictureProfile call into live im.elf (PID 149)
 * via ptrace: attach, inject shellcode into writable memory, run it, detach.
 *
 * ARM Thumb, freestanding, links against camera's real libc/libdl/libpthread.
 *
 * Strategy:
 *   1. ptrace(ATTACH) to PID 149
 *   2. Find libScalarDaemon.so base from /proc/149/maps (we know: 0x12e4f000)
 *   3. Read PID 149 data section to find SystemEventIssuerCamera singleton ptr
 *   4. Write PictureProfileParameter struct + shellcode into PID 149's writable
 *      data section (0x12ffc000+)
 *   5. Redirect PC to shellcode via PTRACE_SETREGS
 *   6. PTRACE_CONT, wait, restore, detach
 *
 * For now: use a simpler approach — write shellcode that:
 *   - Builds PP struct on stack
 *   - Calls setPictureProfile at its known address in PID 149
 *   - Calls a syscall (exit-group or raise) to signal completion
 */

/* ---- Declarations (no libc headers) ---- */
typedef long               ptrdiff_t;
typedef unsigned long      size_t;
typedef unsigned int       uint32_t;
typedef unsigned char      uint8_t;
typedef int                pid_t;

/* syscall numbers for ARM EABI */
#define SYS_ptrace          26
#define SYS_waitpid         7
#define SYS_open            5
#define SYS_read            3
#define SYS_write           4
#define SYS_lseek           19
#define SYS_close           6
#define SYS_exit            1
#define SYS_kill            37

/* ptrace requests */
#define PTRACE_ATTACH       16
#define PTRACE_DETACH       17
#define PTRACE_PEEKDATA     2
#define PTRACE_POKEDATA     5
#define PTRACE_GETREGS      12
#define PTRACE_SETREGS      13
#define PTRACE_CONT         7
#define PTRACE_SINGLESTEP   9

#define SIGSTOP  19
#define SIGCONT  18
#define SIGTRAP  5
#define WNOHANG  1

/* ARM user_regs_struct (18 uint32s) */
typedef struct {
    uint32_t uregs[18];  /* r0-r15, cpsr, orig_r0 */
} arm_regs_t;
#define REG_PC  15
#define REG_SP  13
#define REG_LR  14
#define REG_R0   0
#define REG_R1   1
#define REG_CPSR 16

int puts(const char* s);
int printf(const char* fmt, ...);
void exit(int code);

static long my_syscall3(long n, long a, long b, long c) {
    register long r0 __asm__("r0") = a;
    register long r1 __asm__("r1") = b;
    register long r2 __asm__("r2") = c;
    register long r7 __asm__("r7") = n;
    __asm__ volatile("swi #0" : "+r"(r0) : "r"(r1),"r"(r2),"r"(r7) : "memory");
    return r0;
}

static long my_syscall4(long n, long a, long b, long c, long d) {
    register long r0 __asm__("r0") = a;
    register long r1 __asm__("r1") = b;
    register long r2 __asm__("r2") = c;
    register long r3 __asm__("r3") = d;
    register long r7 __asm__("r7") = n;
    __asm__ volatile("swi #0" : "+r"(r0) : "r"(r1),"r"(r2),"r"(r3),"r"(r7) : "memory");
    return r0;
}

#define O_RDWR   2
#define O_RDONLY 0

static long my_open(const char* path, int flags) {
    return my_syscall3(SYS_open, (long)path, flags, 0);
}
static long my_read(int fd, void* buf, int len) {
    return my_syscall3(SYS_read, fd, (long)buf, len);
}
static long my_write(int fd, const void* buf, int len) {
    return my_syscall3(SYS_write, fd, (long)buf, len);
}
static long my_close(int fd) {
    return my_syscall3(SYS_close, fd, 0, 0);
}
static long my_lseek(int fd, long off, int whence) {
    return my_syscall3(SYS_lseek, fd, off, whence);
}
static long my_ptrace(int req, pid_t pid, long addr, long data) {
    return my_syscall4(SYS_ptrace, req, pid, addr, data);
}
static long my_waitpid(pid_t pid, int* status, int opts) {
    return my_syscall3(SYS_waitpid, pid, (long)status, opts);
}
static long my_kill(pid_t pid, int sig) {
    return my_syscall3(SYS_kill, pid, sig, 0);
}

void* memcpy(void* d, const void* s, size_t n) {
    char* dd=(char*)d; const char* ss=(const char*)s;
    while(n--) *dd++=*ss++;
    return d;
}
void* memset(void* d, int c, size_t n) {
    char* dd=(char*)d; while(n--) *dd++=(char)c; return d;
}

/* Poke 4 bytes into remote process */
static int poke32(pid_t pid, uint32_t addr, uint32_t val) {
    long r = my_ptrace(PTRACE_POKEDATA, pid, addr, val);
    return (int)r;
}

/* Peek 4 bytes from remote process */
static uint32_t peek32(pid_t pid, uint32_t addr) {
    return (uint32_t)my_ptrace(PTRACE_PEEKDATA, pid, addr, 0);
}

/*
 * Shellcode (ARM Thumb) to inject into PID 149's writable memory.
 * It will:
 *   1. Build a PictureProfileParameter struct on stack
 *   2. Call setPictureProfile(this=???, &param)
 *   3. BKPT to signal completion
 *
 * We hardcode the addresses from our analysis:
 *   libScalarDaemon base in PID 149: 0x12e4f000
 *   setPictureProfile offset: 0x118249 (Thumb) → addr = 0x12e4f000+0x118248 = 0x13067248
 *   But we need the 'this' pointer (SystemEventIssuerCamera singleton)
 *
 * For now, use a simpler approach:
 *   We inject code that searches the .data section for the singleton,
 *   OR we call a simpler function that doesn't need 'this'.
 *
 * ALTERNATIVE SIMPLER APPROACH:
 *   We know MWF framework functions are in libMWF.so (also loaded in PID 149)
 *   libMWF base in PID 149: 0x04347000
 *   We can call MWF::ParamBoard functions directly OR
 *   We can search the data section for the gamma param value and overwrite it.
 */

/* Known addresses in PID 149 */
#define SCALAR_BASE   0x12e4f000UL
#define SCALAR_DATA   0x12ffc000UL  /* rw data section of libScalarDaemon */
#define SCALAR_DATA_END 0x1301f000UL
#define MWF_BASE      0x04347000UL
#define MWF_DATA      0x04387000UL  /* rw data of libMWF */

/* setPictureProfile = Thumb function at offset 0x118249 in binary */
/* In Thumb, to call a Thumb func: address must have bit0 set */
#define SET_PP_ADDR   (SCALAR_BASE + 0x118248UL)  /* actual address (even) */
#define SET_PP_THUMB  (SET_PP_ADDR | 1)             /* Thumb call address */

/*
 * ARM Thumb-2 shellcode that:
 * 1. Allocates 0x180 bytes on stack
 * 2. Zeros the PictureProfileParameter struct
 * 3. Sets PP=1 (offset 0x00), gamma=10 (offset 0x88) with flags
 * 4. Loads 'this' pointer from first word of ScalarDaemon .data
 *    (singleton pointer is usually first global in .data)
 * 5. Calls setPictureProfile(this, &buf)
 * 6. BKPT #0 (triggers SIGTRAP back to us)
 *
 * We'll try first with 'this'=0 and step back if crash.
 */

/* Thumb-2 shellcode bytes — assembled manually */
/* We'll write this as a C array and poke it in */
static const uint8_t shellcode[] = {
    /* PUSH {r4-r7, lr} */
    0x2d, 0xe9, 0xf0, 0x41,
    /* SUB sp, sp, #0x180  (allocate struct) */
    0xad, 0xf2, 0x80, 0x01,  /* Thumb-2: SUBW sp, sp, #0x180 */
    /* MOV r0, sp (ptr to struct = r1 for setPP, r0 = this) */
    /* Zero struct: MOV r2, #0x160 / MOVS r3, #0 / BL memset-equivalent */
    /* ... this gets complex. Let's do simpler: just poke the struct directly via ptrace */
    /* BKPT #0 immediately — just test if we can reach here */
    0x70, 0xbe,              /* BKPT #0 (Thumb) */
};

/*
 * ACTUAL STRATEGY (simpler):
 * Instead of injecting shellcode, we:
 * 1. Find the gamma value in the DATA section by scanning for known PP param patterns
 * 2. Write the new value directly via PTRACE_POKEDATA
 *
 * The PP params are stored in a ParamBoard or similar structure in the daemon's
 * memory (heap or data). We'll scan the heap for the pattern.
 *
 * We know from RE: when gamma=Standard (default), the value stored is likely
 * index 0 or 1. We want to change it to index 10 (S-Log2).
 *
 * Let's read the ScalarDaemon data section and look for param ID 0x238
 * (PARAM_GAMMA) or surrounding context.
 */

static void scan_and_patch(pid_t pid) {
    printf("Scanning PID %d data section for PP gamma param...\n", pid);
    
    /* Scan ScalarDaemon .data section */
    uint32_t addr = SCALAR_DATA;
    int found = 0;
    
    while (addr < SCALAR_DATA_END - 16) {
        uint32_t v0 = peek32(pid, addr);
        uint32_t v1 = peek32(pid, addr+4);
        uint32_t v2 = peek32(pid, addr+8);
        uint32_t v3 = peek32(pid, addr+12);
        
        /* Look for pattern: param_id=0x238, value=small_int, flag byte */
        /* Or look for gamma value index (0-11) next to other small values */
        /* Heuristic: find address where val=0x238 (gamma param ID) */
        if (v0 == 0x238) {
            printf("Found 0x238 at 0x%08x: [%d, %d, %d, %d]\n",
                   addr, v0, v1, v2, v3);
            found++;
        }
        /* Also look for PP struct signature: first field=0x274 (PP slot param ID) */
        if (v0 == 0x274) {
            printf("Found PP_SLOT_PARAMID(0x274) at 0x%08x next=[%d,%d,%d]\n",
                   addr, v1, v2, v3);
        }
        addr += 4;
    }
    
    /* Also scan heap area */
    uint32_t heap_start = 0x00016000;
    uint32_t heap_end   = 0x0050c000;
    printf("Scanning heap (0x%08x - 0x%08x) for 0x238...\n", heap_start, heap_end);
    addr = heap_start;
    int heap_hits = 0;
    while (addr < heap_end && heap_hits < 20) {
        uint32_t v = peek32(pid, addr);
        if (v == 0x238) {
            uint32_t next = peek32(pid, addr+4);
            printf("HEAP 0x238 at 0x%08x next=0x%08x(%d)\n", addr, next, next);
            if (next <= 11) { /* plausible gamma index */
                printf("  >>> CANDIDATE gamma value=%d at 0x%08x (want 10=S-Log2)\n",
                       next, addr+4);
                /* Patch it! */
                poke32(pid, addr+4, 10);
                printf("  >>> PATCHED to 10!\n");
            }
            heap_hits++;
        }
        addr += 4;
    }
    printf("Scan done. found=%d heap_hits=%d\n", found, heap_hits);
}

void _start() {
    pid_t target = 149;
    
    puts("=== S-Log2 ptrace injector ===");
    printf("Target PID: %d\n", target);
    printf("ScalarDaemon base: 0x%08x\n", (uint32_t)SCALAR_BASE);
    printf("ScalarDaemon data: 0x%08x\n", (uint32_t)SCALAR_DATA);
    
    /* Attach */
    puts("ptrace ATTACH...");
    long r = my_ptrace(PTRACE_ATTACH, target, 0, 0);
    if (r < 0) {
        printf("ATTACH failed: %ld\n", r);
        exit(1);
    }
    puts("ATTACH OK");
    
    /* Wait for stop */
    int status = 0;
    my_waitpid(target, &status, 0);
    printf("Process stopped. status=0x%08x\n", status);
    
    /* Read registers */
    arm_regs_t regs;
    memset(&regs, 0, sizeof(regs));
    r = my_ptrace(PTRACE_GETREGS, target, 0, (long)&regs);
    if (r < 0) {
        printf("GETREGS failed: %ld\n", r);
    } else {
        printf("PC=0x%08x SP=0x%08x\n", regs.uregs[REG_PC], regs.uregs[REG_SP]);
    }
    
    /* Peek first few words of ScalarDaemon data */
    printf("ScalarDaemon .data[0..7]:\n");
    for (int i = 0; i < 8; i++) {
        uint32_t v = peek32(target, SCALAR_DATA + i*4);
        printf("  [%d] 0x%08x (%d)\n", i, v, v);
    }
    
    /* Scan for gamma param */
    scan_and_patch(target);
    
    /* Detach */
    puts("ptrace DETACH...");
    my_ptrace(PTRACE_DETACH, target, 0, 0);
    puts("DETACH OK");
    
    puts("=== Done. Check camera! ===");
    exit(0);
}

