#include "Utilities.h"

Utilities::Utilities()
{

}

QString Utilities::rstrip(const QString& str)
{
    int n = str.size() - 1;
    for (; n >= 0; --n)
    {
        if (!str.at(n).isSpace())
        {
            return str.left(n + 1);
        }
    }
    return "";
}

