void* dlopen(const char* filename, int flag);
void* dlsym(void* handle, const char* symbol);
char* dlerror(void);
int puts(const char* s);
int printf(const char* format, ...);
void exit(int status);

#include "/tmp/slog2_stubs.h"

typedef int (*SetPPFunc)(const void*);

void _start() {
    puts("=== Sony A5100 S-Log2 Native Host Injector (Clean Stub) ===");

    // Preload only OSAL and SDCOM (glibc IPC stack)
    if (!dlopen("/lib/librt.so.1", 0x102)) puts("dlopen librt failed");
    if (!dlopen("/lib/libpthread.so.0", 0x102)) puts("dlopen libpthread failed");
    if (!dlopen("/usr/lib/libosal_uipc.so", 0x102)) puts("dlopen libosal_uipc failed");
    if (!dlopen("/usr/lib/libosal_utm.so", 0x102)) puts("dlopen libosal_utm failed");
    if (!dlopen("/usr/lib/libsdcom.so", 0x102)) puts("dlopen libsdcom failed");
    puts("OSAL IPC stack loaded successfully!");

    void* handle = dlopen("/usr/lib/libScalarDaemon.so", 0x102);
    if (!handle) {
        printf("FAILED dlopen libScalarDaemon: %s\n", dlerror());
        exit(1);
    }
    puts("dlopen(/usr/lib/libScalarDaemon.so) SUCCESS!");

    SetPPFunc setPP = (SetPPFunc)dlsym(handle, "_ZN12SCALARDAEMON23SystemEventIssuerCamera17setPictureProfileEPKNS0_23PictureProfileParameterE");
    if (!setPP) {
        printf("FAILED dlsym setPictureProfile: %s\n", dlerror());
        exit(2);
    }
    puts("dlsym(setPictureProfile) RESOLVED!");

    unsigned char buf[0x160];
    for (int i = 0; i < 0x160; i++) buf[i] = 0;
    *(unsigned int*)(buf + 0x88) = 10; // S-Log2 wire index (10)
    *(unsigned char*)(buf + 0x8c) = 1; // activate flag (1)

    puts("Calling setPictureProfile(gamma=10)...");
    int ret = setPP(buf);
    printf("setPictureProfile RETURNED: %d\n", ret);
    puts("=== S-Log2 INJECTION FINISHED! Check camera viewfinder / LCD! ===");
    exit(0);
}
