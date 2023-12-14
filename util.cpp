#include "util.h"

extern const int dx[] = {1, 0, -1, 0}, dy[] = {0, -1, 0, 1};

extern const QPoint dp[] = { {1, 0}, {0, -1}, {-1, 0}, {0, 1} };

rotate_t rotateL(rotate_t d) {
  return rotate_t((d+1)%4);
}
rotate_t rotateR(rotate_t d) {
  return rotate_t((d+3)%4);
}
