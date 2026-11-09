typedef unsigned int uint32_t;
typedef unsigned char uint8_t;
typedef unsigned long size_t;

#define O_RDONLY 0
#define O_RDWR   2
#define PROT_READ  1
#define PROT_WRITE 2
#define MAP_SHARED 1
#define MAP_FAILED ((void*)-1)

int open(const char* path, int flags, ...);
int close(int fd);
void* mmap(void* addr, size_t length, int prot, int flags, int fd, long offset);
int munmap(void* addr, size_t length);
int printf(const char* format, ...);
void exit(int status);

static int my_memcmp(const void* s1, const void* s2, size_t n) {
    const uint8_t* p1 = (const uint8_t*)s1;
    const uint8_t* p2 = (const uint8_t*)s2;
    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) return p1[i] - p2[i];
    }
    exit(0);
}

static size_t my_strlen(const char* s) {
    size_t len = 0;
    while (*s++) len++;
    return len;
}

void _start() {
    printf("[*] Starting av-cam physical memory scanner...\n");
    int fd = open("/dev/mem", O_RDONLY);
    if (fd < 0) {
        printf("[-] Failed to open /dev/mem\n");
        exit(1);
    }

    // Signatures
    const uint8_t magic[] = {0x0a, 0x00, 0x00, 0xea, 0x4f, 0x52, 0x49, 0x4c, 0x04, 0x00, 0x00, 0x00, 0x44, 0x10, 0x54, 0x43};
    const uint8_t check_func[] = {0x70, 0xb5, 0x0d, 0x46, 0x04, 0x46}; // CheckSetExGammaTableControl
    const char str_exgamma[] = "ExGammaTableOn";
    const char str_pp[] = "PictureProfile";

    // 1. First test 0x43541000 directly (from liro adr)
    printf("[*] Checking liro adr region (0x43540000)...\n");
    void* p_liro = mmap(0, 0x20000, PROT_READ, MAP_SHARED, fd, 0x43540000);
    if (p_liro != MAP_FAILED) {
        uint8_t* p = (uint8_t*)p_liro;
        printf("[+] Mapped 0x43540000 successfully!\n");
        for (uint32_t i = 0; i < 0x20000 - 16; i += 4) {
            if (my_memcmp(p + i, magic, 8) == 0) {
                printf("[+++] FOUND av-cam MAGIC at 0x%08x!\n", 0x43540000 + i);
            }
        }
        munmap(p_liro, 0x20000);
    } else {
        printf("[-] Could not mmap 0x43540000\n");
    }

    // 2. Scan System RAM 0x80000000 to 0xC0000000 in 16MB chunks
    printf("[*] Scanning System RAM 0x80000000 - 0xC0000000...\n");
    uint32_t chunk_size = 16 * 1024 * 1024;
    for (uint32_t base = 0x80000000; base < 0xc0000000; base += chunk_size) {
        void* ptr = mmap(0, chunk_size, PROT_READ, MAP_SHARED, fd, base);
        if (ptr == MAP_FAILED) {
            continue;
        }

        uint8_t* p = (uint8_t*)ptr;
        for (uint32_t i = 0; i < chunk_size - 64; i += 4) {
            if (my_memcmp(p + i, magic, sizeof(magic)) == 0) {
                printf("[+++] FOUND av-cam MAGIC at Physical Address: 0x%08x\n", base + i);
            }
            if (my_memcmp(p + i, check_func, sizeof(check_func)) == 0) {
                // Verify second byte instruction
                if (p[i+6] == 0x2d || (p[i+7] == 0xf0 || p[i+7] == 0xf7)) {
                    printf("[+++] CANDIDATE CheckSetExGamma at Phys: 0x%08x (base=0x%08x, off=0x%08x)\n", base + i, (base + i) - 0x2d2dc4, i);
                }
            }
        }
        munmap(ptr, chunk_size);
    }

    // 3. Scan ARM7 SRAM (0x00000000)
    void* sram = mmap(0, 0x4000, PROT_READ, MAP_SHARED, fd, 0x00000000);
    if (sram != MAP_FAILED) {
        uint8_t* p = (uint8_t*)sram;
        for (uint32_t i = 0; i < 0x4000 - 16; i += 4) {
            if (my_memcmp(p + i, magic, 8) == 0) {
                printf("[+++] FOUND magic in SRAM: 0x%08x\n", i);
            }
        }
        munmap(sram, 0x4000);
    }

    printf("[*] Scan complete.\n");
    close(fd);
    exit(0);
}
