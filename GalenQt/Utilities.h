#ifndef UTILITIES_H
#define UTILITIES_H

#include <QString>

class Utilities
{
public:
    Utilities();

    static QString rstrip(const QString& str);
    static void RadixSort11(float *farray, float *sorted, uint32_t elements);
};

#endif // UTILITIES_H
