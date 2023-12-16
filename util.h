#ifndef UTIL_H
#define UTIL_H

#include <QRandomGenerator>
#include <QPoint>

enum rotate_t { R0 = 0, R90 = 1, R180 = 2, R270 = 3 };

rotate_t rotateL(rotate_t d);
rotate_t rotateR(rotate_t d);

extern const int dx[], dy[];
extern const QPoint dp[];

extern QRandomGenerator &rng;

#endif // UTIL_H
