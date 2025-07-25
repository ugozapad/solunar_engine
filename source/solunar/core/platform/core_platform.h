#ifndef CORE_PLATFORM_H
#define CORE_PLATFORM_H

#ifdef WIN32
#	include "core/platform/core_platform_win32.h"
#endif // WIN32

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#include <time.h>

#include "../containers/containers.h"

#endif // !CORE_PLATFORM_H
