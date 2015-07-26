#ifndef WINDOWS
#include <unistd.h>

inline void closesocket(int sock)
{
    close(sock);
}
#endif