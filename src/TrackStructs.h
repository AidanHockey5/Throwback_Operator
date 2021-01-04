#ifndef TRACKSTRUCTS_H_
#define TRACKSTRUCTS_H_
#include <stdint.h>

enum FileStrategy {FIRST_START, NEXT, PREV, RND, REQUEST};
enum PlayMode {LOOP, PAUSE, SHUFFLE, IN_ORDER};
static PlayMode playMode;
#endif