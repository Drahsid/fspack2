#ifndef FSPACK_COMMON_H
#define FSPACK_COMMON_H

#define ABS(X) ((X) < 0 ? -(X) : (X))
#ifdef __cplusplus
#define CONSTEXPR constexpr
#else
#define CONSTEXPR
#endif

#ifdef _WIN32
#include "dirent.h"
#else
#include <dirent.h>
#define MAX_PATH 260
#endif

typedef struct dirent DirentFile;

#endif

