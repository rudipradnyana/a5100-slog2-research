typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef unsigned long size_t;

void* dlopen(const char* filename, int flag);
void* dlsym(void* handle, const char* symbol);

typedef void* JNIEnv;
typedef void* jobject;
typedef int jint;

typedef int (*SetPPFunc)(const void*);

__attribute__((visibility("default")))
jint Java_com_rudi_gammatest2_GammaTestActivity_injectSLog2(JNIEnv* env, jobject thiz, jint gammaVal) {
    void* handle = dlopen("/usr/lib/libScalarDaemon.so", 2);
    if (!handle) handle = dlopen("/system/lib/libScalarDaemon.so", 2);
    if (!handle) handle = dlopen("libScalarDaemon.so", 2);
    if (!handle) return -101; // dlopen failed

    SetPPFunc setPP = (SetPPFunc)dlsym(handle, "_ZN12SCALARDAEMON23SystemEventIssuerCamera17setPictureProfileEPKNS0_23PictureProfileParameterE");
    if (!setPP) return -102; // dlsym failed

    uint8_t buf[0x160];
    for (int i = 0; i < 0x160; i++) buf[i] = 0;
    *(uint32_t*)(buf + 0x88) = (uint32_t)gammaVal;
    *(uint8_t*)(buf + 0x8c) = 1;

    int ret = setPP(buf);
    return ret;
}
