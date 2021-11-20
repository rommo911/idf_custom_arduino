#ifndef __WMATH_H__
#define __WMATH_H__

extern "C" {
#include <stdlib.h>
#include "esp_system.h"
}
long random(long, long);
void randomSeed(unsigned long);
long map(long, long, long, long, long);
long random(long howsmall, long howbig);
unsigned int makeWord(unsigned char h, unsigned char l);

#endif // __WMATH_H__