void* dlopen(const char* filename, int flag) { return 0; }
void* dlsym(void* handle, const char* symbol) { return 0; }
char* dlerror(void) { return 0; }
int puts(const char* s) { return 0; }
int printf(const char* format, ...) { return 0; }
void exit(int status) {}
int open(const char* path, int flags, ...) { return 0; }
int close(int fd) { return 0; }
void* mmap(void* addr, unsigned long length, int prot, int flags, int fd, unsigned long offset) { return 0; }
int munmap(void* addr, unsigned long length) { return 0; }
void perror(const char* s) {}
int memcmp(const void* s1, const void* s2, unsigned long n) { return 0; }
int msync(void* addr, unsigned long length, int flags) { return 0; }
