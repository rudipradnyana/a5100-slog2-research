void* dlopen(const char* filename, int flag);
void* dlsym(void* handle, const char* symbol);
char* dlerror(void);
int puts(const char* s);
int printf(const char* format, ...);
void exit(int status);

#include "/tmp/slog2_stubs.h"

typedef int (*SetPPFunc)(const void*);

void _start() {
    puts("=== Sony A5100 S-Log2 v3 (Return code test) ===");
    
    dlopen("/lib/librt.so.1", 0x102);
    dlopen("/lib/libpthread.so.0", 0x102);
    dlopen("/usr/lib/libosal_uipc.so", 0x102);
    dlopen("/usr/lib/libosal_utm.so", 0x102);
    dlopen("/usr/lib/libsdcom.so", 0x102);

    void* handle = dlopen("/usr/lib/libScalarDaemon.so", 0x102);
    if (!handle) { printf("FAILED: %s\n", dlerror()); exit(1); }

    SetPPFunc setPP = (SetPPFunc)dlsym(handle, "_ZN12SCALARDAEMON23SystemEventIssuerCamera17setPictureProfileEPKNS0_23PictureProfileParameterE");
    if (!setPP) { printf("FAILED dlsym: %s\n", dlerror()); exit(2); }

    // Test 1: NULL pointer -> should trigger null-guard -> return 1
    int ret_null = setPP((void*)0);
    printf("NULL ptr RETURNED: %d\n", ret_null);

    // Test 2: valid buf, gamma=10, PP=1
    unsigned char buf[0x160];
    for (int i = 0; i < 0x160; i++) buf[i] = 0;
    *((unsigned int*)(buf + 0x00)) = 1;   // PP=1
    *((unsigned char*)(buf + 0x04)) = 1;  // flag
    *((unsigned int*)(buf + 0x88)) = 10;  // gamma=10 (S-Log2)
    *((unsigned char*)(buf + 0x8c)) = 1;  // flag
    int ret_valid = setPP(buf);
    printf("VALID ptr RETURNED: %d\n", ret_valid);

    // Test 3: valid buf, all 44 flags=1, gamma=10
    for (int i = 0; i < 0x160; i++) buf[i] = 0;
    // Enable ALL flags, all values = 0 (defaults), except gamma=10
    int offsets[] = {0x04,0x0c,0x14,0x1c,0x24,0x2c,0x34,0x3c,0x44,0x4c,
                     0x54,0x5c,0x64,0x6c,0x74,0x7c,0x84,0x8c,0x94,0x9c,
                     0xa4,0xac,0xb4,0xbc,0xc4,0xcc,0xd4,0xdc,0xe4,0xec,
                     0xf4,0xfc,0x104,0x10c,0x114,0x11c,0x124,0x12c,0x134,
                     0x13c,0x144,0x14c,0x154,0x15c};
    for (int i = 0; i < 44; i++) {
        *((unsigned char*)(buf + offsets[i])) = 1;
    }
    *((unsigned int*)(buf + 0x00)) = 1;   // PP=1
    *((unsigned int*)(buf + 0x88)) = 10;  // gamma=10

    int ret_all = setPP(buf);
    printf("ALL FLAGS RETURNED: %d\n", ret_all);

    puts("=== Check camera now! ===");
    exit(0);
}
