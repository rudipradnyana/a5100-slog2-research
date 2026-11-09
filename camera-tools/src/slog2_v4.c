void *dlopen(const char *filename, int flag);
void *dlsym(void *handle, const char *symbol);
char *dlerror(void);
int puts(const char *s);
int printf(const char *format, ...);
void exit(int status);

#include "/tmp/slog2_stubs.h"

/* Provide memcpy - compiler may generate calls to it */
void *memcpy(void *dst, const void *src, unsigned long n) {
  char *d = (char *)dst;
  const char *s = (const char *)src;
  while (n--)
    *d++ = *s++;
  return dst;
}

void *memset(void *dst, int c, unsigned long n) {
  char *d = (char *)dst;
  while (n--)
    *d++ = (char)c;
  return dst;
}

typedef int (*SetPPFunc)(const void *);
typedef int (*ResetPPFunc)(void);

static void zero_buf(unsigned char *buf, int len) {
  int i;
  for (i = 0; i < len; i++)
    buf[i] = 0;
}

void _start() {
  puts("=== Sony A5100 S-Log2 v4 ===");

  /* --- Load dependencies --- */
  if (!dlopen("/lib/librt.so.1", 0x102))
    puts("[WARN] librt failed");
  if (!dlopen("/lib/libpthread.so.0", 0x102))
    puts("[WARN] libpthread failed");
  if (!dlopen("/usr/lib/libosal_uipc.so", 0x102))
    puts("[WARN] libosal_uipc failed");
  if (!dlopen("/usr/lib/libosal_utm.so", 0x102))
    puts("[WARN] libosal_utm failed");
  if (!dlopen("/usr/lib/libsdcom.so", 0x102))
    puts("[WARN] libsdcom failed");

  void *handle = dlopen("/usr/lib/libScalarDaemon.so", 0x102);
  if (!handle) {
    printf("FAILED dlopen: %s\n", dlerror());
    exit(1);
  }
  puts("dlopen libScalarDaemon OK");

  SetPPFunc setPP = (SetPPFunc)dlsym(
      handle, "_ZN12SCALARDAEMON23SystemEventIssuerCamera17setPictureProfileEPK"
              "NS0_23PictureProfileParameterE");
  if (!setPP) {
    printf("FAILED dlsym setPP: %s\n", dlerror());
    exit(2);
  }
  puts("dlsym setPP OK");

  ResetPPFunc resetPP = (ResetPPFunc)dlsym(
      handle,
      "_ZN12SCALARDAEMON23SystemEventIssuerCamera19resetPictureProfileEv");
  if (!resetPP)
    puts("[WARN] dlsym resetPP not found (continuing)");
  else
    puts("dlsym resetPP OK");

  int ret;
  unsigned char buf[0x160];

  /* ============================================================
   * TEST 1: NULL pointer
   * Expected: returns 1 immediately (null guard beq.w at 0x118820)
   * This baseline tells us: NULL path always returns 1.
   * ============================================================ */
  puts("[TEST1] calling setPP(NULL)...");
  ret = setPP(0);
  printf("[TEST1] setPP(NULL) = %d\n", ret);

  /* ============================================================
   * TEST 2: Only gamma field enabled (1 param)
   * If return = number-of-params-sent, should return 1.
   * ============================================================ */
  zero_buf(buf, 0x160);
  *((unsigned int *)(buf + 0x88)) = 10; /* gamma=10 (S-Log2 index) */
  *((unsigned char *)(buf + 0x8c)) = 1; /* flag=enabled */

  puts("[TEST2] calling setPP(gamma=10 only, 1 param)...");
  ret = setPP(buf);
  printf("[TEST2] setPP(gamma=10 only) = %d\n", ret);

  /* ============================================================
   * TEST 3: PP index + gamma (2 params)
   * If return = count, should return 2.
   * ============================================================ */
  zero_buf(buf, 0x160);
  *((unsigned int *)(buf + 0x00)) = 1; /* PP slot = 1 */
  *((unsigned char *)(buf + 0x04)) = 1;
  *((unsigned int *)(buf + 0x88)) = 10; /* gamma = S-Log2 */
  *((unsigned char *)(buf + 0x8c)) = 1;

  puts("[TEST3] calling setPP(PP=1, gamma=10, 2 params)...");
  ret = setPP(buf);
  printf("[TEST3] setPP(PP=1, gamma=10) = %d\n", ret);

  /* ============================================================
   * TEST 4: resetPictureProfile then setPP
   * Reset PP1 to defaults, then apply S-Log2
   * ============================================================ */
  if (resetPP) {
    puts("[TEST4] calling resetPP()...");
    ret = resetPP();
    printf("[TEST4] resetPP() = %d\n", ret);

    zero_buf(buf, 0x160);
    *((unsigned int *)(buf + 0x00)) = 1;
    *((unsigned char *)(buf + 0x04)) = 1;
    *((unsigned int *)(buf + 0x88)) = 10;
    *((unsigned char *)(buf + 0x8c)) = 1;

    puts("[TEST4] calling setPP(PP=1, gamma=10) after reset...");
    ret = setPP(buf);
    printf("[TEST4] setPP after reset = %d\n", ret);
  }

  /* ============================================================
   * TEST 5: ALL 44 params set, gamma=10 (S-Log2)
   * Set every field's flag=1 so daemon can't ignore them.
   * Values copied from factory defaults for most, gamma=10.
   * ============================================================ */
  zero_buf(buf, 0x160);

  /* Enable ALL 44 params (flag byte = 1 for each) */
  static const int offsets[] = {
      0x00,  0x08,  0x10,  0x18,  0x20,  0x28,  0x30,  0x38,  0x40,
      0x48,  0x50,  0x58,  0x60,  0x68,  0x70,  0x78,  0x80,  0x88,
      0x90,  0x98,  0xa0,  0xa8,  0xb0,  0xb8,  0xc0,  0xc8,  0xd0,
      0xd8,  0xe0,  0xe8,  0xf0,  0xf8,  0x100, 0x108, 0x110, 0x118,
      0x120, 0x128, 0x130, 0x138, 0x140, 0x148, 0x150, 0x158};
  int i;
  for (i = 0; i < 44; i++) {
    *((unsigned int *)(buf + offsets[i])) = 0;      /* value=0 (default) */
    *((unsigned char *)(buf + offsets[i] + 4)) = 1; /* flag=enabled */
  }

  /* Override specific fields */
  *((unsigned int *)(buf + 0x00)) = 1;  /* PP slot = 1 */
  *((unsigned int *)(buf + 0x88)) = 10; /* gamma = S-Log2 (index 10) */

  puts("[TEST5] calling setPP(ALL 44 params, PP=1, gamma=10)...");
  ret = setPP(buf);
  printf("[TEST5] setPP(ALL 44) = %d\n", ret);

  puts("=== v4 done. Check camera LCD! ===");
  exit(0);
}
