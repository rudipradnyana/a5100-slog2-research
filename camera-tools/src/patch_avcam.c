typedef unsigned int uint32_t;
typedef unsigned char uint8_t;
typedef unsigned long size_t;

#define O_RDONLY 0
#define O_RDWR   2
#define O_SYNC   0x101000

#define PROT_READ  1
#define PROT_WRITE 2
#define MAP_SHARED 1
#define MAP_FAILED ((void*)-1)

int open(const char* path, int flags, ...);
int close(int fd);
void* mmap(void* addr, size_t length, int prot, int flags, int fd, long offset);
int munmap(void* addr, size_t length);
int msync(void* addr, size_t length, int flags);
int printf(const char* format, ...);
void exit(int status);

static int my_memcmp(const void* s1, const void* s2, size_t n) {
    const uint8_t* p1 = (const uint8_t*)s1;
    const uint8_t* p2 = (const uint8_t*)s2;
    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) return p1[i] - p2[i];
    }
    return 0;
}

void _start() {
    printf("[*] === Sony A5100 S-Log2 / PictureProfile Capability Gate Patcher ===\n");

    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
        printf("[-] Failed to open /dev/mem with O_RDWR!\n");
        exit(1);
    }
    printf("[+] /dev/mem opened successfully (O_RDWR).\n");

    // Targets:
    // 1. ExGammaTableOn: 0x83813dc4
    // 2. ul_ExGammaTablePointer: 0x83813dfc
    // 3. PictureProfile: 0x838119d0

    // Patch: movs r0, #0; bx lr -> 00 20 70 47
    const uint8_t patch_code[4] = {0x00, 0x20, 0x70, 0x47};

    // --- Page 1: 0x83813000 (covers 0x83813dc4 and 0x83813dfc) ---
    void* p1 = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x83813000);
    if (p1 == MAP_FAILED) {
        printf("[-] Failed to mmap page 0x83813000\n");
        exit(2);
    }
    uint8_t* page1 = (uint8_t*)p1;

    // Check & Patch ExGammaTableOn
    uint8_t* target1 = page1 + 0xdc4;
    printf("[*] Target 1 (ExGammaTableOn @ 0x83813dc4): %02x %02x %02x %02x\n",
           target1[0], target1[1], target1[2], target1[3]);
    target1[0] = patch_code[0];
    target1[1] = patch_code[1];
    target1[2] = patch_code[2];
    target1[3] = patch_code[3];
    printf("[+] Patched Target 1 -> %02x %02x %02x %02x (movs r0, #0; bx lr)\n",
           target1[0], target1[1], target1[2], target1[3]);

    // Check & Patch ul_ExGammaTablePointer
    uint8_t* target2 = page1 + 0xdfc;
    printf("[*] Target 2 (ul_ExGammaTablePointer @ 0x83813dfc): %02x %02x %02x %02x\n",
           target2[0], target2[1], target2[2], target2[3]);
    target2[0] = patch_code[0];
    target2[1] = patch_code[1];
    target2[2] = patch_code[2];
    target2[3] = patch_code[3];
    printf("[+] Patched Target 2 -> %02x %02x %02x %02x (movs r0, #0; bx lr)\n",
           target2[0], target2[1], target2[2], target2[3]);

    msync(p1, 4096, 4); // MS_SYNC = 4
    munmap(p1, 4096);

    // --- Page 2: 0x83811000 (covers 0x838119d0) ---
    void* p2 = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x83811000);
    if (p2 == MAP_FAILED) {
        printf("[-] Failed to mmap page 0x83811000\n");
        exit(3);
    }
    uint8_t* page2 = (uint8_t*)p2;

    uint8_t* target3 = page2 + 0x9d0;
    printf("[*] Target 3 (PictureProfile @ 0x838119d0): %02x %02x %02x %02x\n",
           target3[0], target3[1], target3[2], target3[3]);
    target3[0] = patch_code[0];
    target3[1] = patch_code[1];
    target3[2] = patch_code[2];
    target3[3] = patch_code[3];
    printf("[+] Patched Target 3 -> %02x %02x %02x %02x (movs r0, #0; bx lr)\n",
           target3[0], target3[1], target3[2], target3[3]);

    msync(p2, 4096, 4);
    munmap(p2, 4096);

    close(fd);
    printf("[+++] ALL SONY A5100 S-LOG2 / PICTURE PROFILE CAPABILITY GATES SUCCESSFULLY PATCHED IN RAM!\n");
    exit(0);
}
