
#include <unistd.h>
#include <pthread.h>
int main()
{
    #ifndef _POSIX_THREADS
    # error POSIX Threads support not available
    #endif
    return 0;
}
