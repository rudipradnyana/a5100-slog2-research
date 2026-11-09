
#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>

int main() {
    int pid = 149;
    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0) {
        perror("attach"); return 1;
    }
    int status;
    waitpid(pid, &status, 0);
    
    // Read s_postEventFunc at 0x1301ed58
    long val = ptrace(PTRACE_PEEKDATA, pid, (void*)0x1301ed58, NULL);
    printf("s_postEventFunc in PID 149 = 0x%lx\n", val);
    
    ptrace(PTRACE_DETACH, pid, NULL, NULL);
    return 0;
}
