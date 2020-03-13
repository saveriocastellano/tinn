#ifndef DIRENT_INCLUDED
#define DIRENT_INCLUDED



#ifdef __cplusplus
extern "C"
{
#endif

#include <direct.h>
#include <winsock.h> 
#include <time.h>

#define mkdir _mkdir
#define popen _popen
#define pclose _pclose
#define getcwd _getcwd
#define unlink _unlink
#define strdup _strdup
#pragma comment(lib, "Ws2_32.lib")
int setenv(const char *name, const char *value, int overwrite);

typedef struct DIR DIR;

DIR           *opendir(const char *);
int           closedir(DIR *);
struct dirent *readdir(DIR *);
void          rewinddir(DIR *);



#ifdef __cplusplus
}
#endif

#endif
