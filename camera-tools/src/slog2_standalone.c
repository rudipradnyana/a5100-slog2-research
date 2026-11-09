typedef unsigned char uint8_t;
typedef unsigned int uint32_t;

void* dlopen(const char* filename, int flag);
void* dlsym(void* handle, const char* symbol);
typedef int (*SetPPFunc)(const void*);

static long write(int fd, const void* buf, unsigned long count) {
    register long r0 __asm__("r0") = fd;
    register long r1 __asm__("r1") = (long)buf;
    register long r2 __asm__("r2") = count;
    register long r7 __asm__("r7") = 4;
    __asm__ __volatile__("swi 0" : "=r"(r0) : "r"(r0), "r"(r1), "r"(r2), "r"(r7) : "memory");
    return r0;
}

static void exit(int status) {
    register long r0 __asm__("r0") = status;
    register long r7 __asm__("r7") = 1;
    __asm__ __volatile__("swi 0" : : "r"(r0), "r"(r7) : "memory");
}

void _start() {
    write(1, "=== S-Log2 Standalone Injector ===\n", 35);
    void* handle = dlopen("/usr/lib/libScalarDaemon.so", 2);
    if (!handle) handle = dlopen("/system/lib/libScalarDaemon.so", 2);
    if (!handle) {
        write(1, "ERR: dlopen failed\n", 19);
        exit(1);
    }
    SetPPFunc setPP = (SetPPFunc)dlsym(handle, "_ZN12SCALARDAEMON23SystemEventIssuerCamera17setPictureProfileEPKNS0_23PictureProfileParameterE");
    if (!setPP) {
        write(1, "ERR: dlsym failed\n", 18);
        exit(2);
    }
    uint8_t buf[0x160];
    for (int i = 0; i < 0x160; i++) buf[i] = 0;
    *(uint32_t*)(buf + 0x88) = 10;
    *(uint8_t*)(buf + 0x8c) = 1;

    write(1, "Calling setPictureProfile(gamma=10)...\n", 39);
    int ret = setPP(buf);
    write(1, "Success! Check LCD for S-Log2 gamma shift.\n", 43);
    exit(0);
}
