void* dlopen(const char* filename, int flag);
void* dlsym(void* handle, const char* symbol);
char* dlerror(void);
int puts(const char* s);
int printf(const char* format, ...);
void exit(int status);

#include "/tmp/slog2_stubs.h"

typedef int (*SetPPFunc)(const void*);

void _start() {
    puts("=== Sony A5100 S-Log2 v2 (Full PP Struct) ===");
    
    if (!dlopen("/lib/librt.so.1", 0x102)) puts("dlopen librt failed");
    if (!dlopen("/lib/libpthread.so.0", 0x102)) puts("dlopen libpthread failed");
    if (!dlopen("/usr/lib/libosal_uipc.so", 0x102)) puts("dlopen libosal_uipc failed");
    if (!dlopen("/usr/lib/libosal_utm.so", 0x102)) puts("dlopen libosal_utm failed");
    if (!dlopen("/usr/lib/libsdcom.so", 0x102)) puts("dlopen libsdcom failed");

    void* handle = dlopen("/usr/lib/libScalarDaemon.so", 0x102);
    if (!handle) {
        printf("FAILED dlopen libScalarDaemon: %s\n", dlerror());
        exit(1);
    }
    puts("dlopen SUCCESS!");

    SetPPFunc setPP = (SetPPFunc)dlsym(handle, "_ZN12SCALARDAEMON23SystemEventIssuerCamera17setPictureProfileEPKNS0_23PictureProfileParameterE");
    if (!setPP) {
        printf("FAILED dlsym: %s\n", dlerror());
        exit(2);
    }
    puts("dlsym RESOLVED!");

    // Struct layout (each field = 4-byte value at +0x0 + 1-byte flag at +0x4):
    // +0x00/0x04: Param 0x274 (PP profile number: 1=PP1 ... 6=PP6)
    // +0x08/0x0c: Param 0x1f8
    // +0x10/0x14: Param 0x1fc
    // +0x18/0x1c: Param 0x200
    // +0x20/0x24: Param 0x204
    // +0x28/0x2c: Param 0x208
    // +0x30/0x34: Param 0x20c
    // +0x38/0x3c: Param 0x210
    // +0x40/0x44: Param 0x214
    // +0x48/0x4c: Param 0x218
    // +0x50/0x54: Param 0x21c
    // +0x58/0x5c: Param 0x220
    // +0x60/0x64: Param 0x224
    // +0x68/0x6c: Param 0x228
    // +0x70/0x74: Param 0x22c
    // +0x78/0x7c: Param 0x230
    // +0x80/0x84: Param 0x234
    // +0x88/0x8c: Param 0x238 = GAMMA TYPE (10 = S-Log2)
    // +0x90/0x94: Param 0x23c
    // ... etc

    unsigned char buf[0x160];
    for (int i = 0; i < 0x160; i++) buf[i] = 0;

    // Test variant B: Try RAW MWF value 0x480 for gamma
    // AND set PP index = 1 (PP1)

    // PP index = 1 (select PP1 slot)
    *((unsigned int*)(buf + 0x00)) = 1;
    *((unsigned char*)(buf + 0x04)) = 1;

    // Gamma = 0x480 (raw MWF VALUEID_PP_GAMMA_SLOG2)
    *((unsigned int*)(buf + 0x88)) = 0x480;
    *((unsigned char*)(buf + 0x8c)) = 1;

    puts("Calling setPictureProfile(PP=1, gamma=0x480)...");
    int ret = setPP(buf);
    printf("VARIANT_480 RETURNED: %d\n", ret);

    // Also try with index 10
    for (int i = 0; i < 0x160; i++) buf[i] = 0;
    *((unsigned int*)(buf + 0x00)) = 1;
    *((unsigned char*)(buf + 0x04)) = 1;
    *((unsigned int*)(buf + 0x88)) = 10;
    *((unsigned char*)(buf + 0x8c)) = 1;

    puts("Calling setPictureProfile(PP=1, gamma=10)...");
    ret = setPP(buf);
    printf("VARIANT_10 RETURNED: %d\n", ret);

    puts("=== Done. Check camera LCD now! ===");
    exit(0);
}
