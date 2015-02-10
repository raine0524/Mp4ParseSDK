
#include "mp4utils.h"

#ifdef _WIN32
  #include <Winsock2.h>
  #pragma comment(lib,"Ws2_32.lib")
#else 
  #include <stdio.h>
  #include <unistd.h>
  #include <errno.h>
  #include <time.h>
  #include <sys/times.h>
#endif



void mpSleep(unsigned int ms)
{

#ifdef WIN32
	Sleep(ms); 
#else
	usleep(ms*1000);
#endif

}

unsigned long mpClkRateGet()
{
#ifdef _MSC_VER
    return 1000;
#endif

#ifdef _LINUX_
    long ret = sysconf(_SC_CLK_TCK);
    return (unsigned long)ret;
#endif
}

unsigned long mpTickGet()
{
#ifdef _MSC_VER
    return GetTickCount();
#endif

#ifdef _LINUX_
    clock_t ret = times(NULL);
    //times always valid!
    if (-1==ret)
    {
        ret = 0xffffffff - errno;
    }

    return (unsigned long)ret;
#endif
}

unsigned long mpTickToMs(unsigned long  dwTick)
{
    return dwTick * ( 1000 / mpClkRateGet());
}

