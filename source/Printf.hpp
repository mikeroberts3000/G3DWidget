#ifndef PRINTF_HPP
#define PRINTF_HPP

#include <string>

#include "ToString.hpp"

namespace mojo
{

template <typename T00>
inline void printf(T00 t00);

template <typename T00, typename T01>
inline void printf(T00 t00, T01 t01);

template <typename T00, typename T01, typename T02>
inline void printf(T00 t00, T01 t01, T02 t02);

template <typename T00, typename T01, typename T02, typename T03>
inline void printf(T00 t00, T01 t01, T02 t02, T03 t03);

template <typename T00, typename T01, typename T02, typename T03, typename T04>
inline void printf(T00 t00, T01 t01, T02 t02, T03 t03, T04 t04);

template <typename T00, typename T01, typename T02, typename T03, typename T04, typename T05>
inline void printf(T00 t00, T01 t01, T02 t02, T03 t03, T04 t04, T05 t05);

template <typename T00, typename T01, typename T02, typename T03, typename T04, typename T05, typename T06>
inline void printf(T00 t00, T01 t01, T02 t02, T03 t03, T04 t04, T05 t05, T06 t06);

template <typename T00, typename T01, typename T02, typename T03, typename T04, typename T05, typename T06, typename T07>
inline void printf(T00 t00, T01 t01, T02 t02, T03 t03, T04 t04, T05 t05, T06 t06, T07 t07);

template <typename T00, typename T01, typename T02, typename T03, typename T04, typename T05, typename T06, typename T07, typename T08>
inline void printf(T00 t00, T01 t01, T02 t02, T03 t03, T04 t04, T05 t05, T06 t06, T07 t07, T08 t08);

template <typename T00, typename T01, typename T02, typename T03, typename T04, typename T05, typename T06, typename T07, typename T08, typename T09>
inline void printf(T00 t00, T01 t01, T02 t02, T03 t03, T04 t04, T05 t05, T06 t06, T07 t07, T08 t08, T09 t09);

template <typename T00, typename T01, typename T02, typename T03, typename T04, typename T05, typename T06, typename T07, typename T08, typename T09, typename T10>
inline void printf(T00 t00, T01 t01, T02 t02, T03 t03, T04 t04, T05 t05, T06 t06, T07 t07, T08 t08, T09 t09, T10 t10);

template <typename T00, typename T01, typename T02, typename T03, typename T04, typename T05, typename T06, typename T07, typename T08, typename T09, typename T10, typename T11>
inline void printf(T00 t00, T01 t01, T02 t02, T03 t03, T04 t04, T05 t05, T06 t06, T07 t07, T08 t08, T09 t09, T10 t10, T11 t11);

void printfHelper(const std::string& string);

template <typename T00>
inline void printf(T00 t00)
{
    printfHelper(toString(t00));
}

template <typename T00, typename T01>
inline void printf(T00 t00, T01 t01)
{
    printfHelper(toString(t00, t01));
}

template <typename T00, typename T01, typename T02>
inline void printf(T00 t00, T01 t01, T02 t02)
{
    printfHelper(toString(t00, t01, t02));
}

template <typename T00, typename T01, typename T02, typename T03>
inline void printf(T00 t00, T01 t01, T02 t02, T03 t03)
{
    printfHelper(toString(t00, t01, t02, t03));
}

template <typename T00, typename T01, typename T02, typename T03, typename T04>
inline void printf(T00 t00, T01 t01, T02 t02, T03 t03, T04 t04)
{
    printfHelper(toString(t00, t01, t02, t03, t04));
}

template <typename T00, typename T01, typename T02, typename T03, typename T04, typename T05>
inline void printf(T00 t00, T01 t01, T02 t02, T03 t03, T04 t04, T05 t05)
{
    printfHelper(toString(t00, t01, t02, t03, t04, t05));
}

template <typename T00, typename T01, typename T02, typename T03, typename T04, typename T05, typename T06>
inline void printf(T00 t00, T01 t01, T02 t02, T03 t03, T04 t04, T05 t05, T06 t06)
{
    printfHelper(toString(t00, t01, t02, t03, t04, t05, t06));
}

template <typename T00, typename T01, typename T02, typename T03, typename T04, typename T05, typename T06, typename T07>
inline void printf(T00 t00, T01 t01, T02 t02, T03 t03, T04 t04, T05 t05, T06 t06, T07 t07)
{
    printfHelper(toString(t00, t01, t02, t03, t04, t05, t06, t07));
}

template <typename T00, typename T01, typename T02, typename T03, typename T04, typename T05, typename T06, typename T07, typename T08>
inline void printf(T00 t00, T01 t01, T02 t02, T03 t03, T04 t04, T05 t05, T06 t06, T07 t07, T08 t08)
{
    printfHelper(toString(t00, t01, t02, t03, t04, t05, t06, t07, t08));
}

template <typename T00, typename T01, typename T02, typename T03, typename T04, typename T05, typename T06, typename T07, typename T08, typename T09>
inline void printf(T00 t00, T01 t01, T02 t02, T03 t03, T04 t04, T05 t05, T06 t06, T07 t07, T08 t08, T09 t09)
{
    printfHelper(toString(t00, t01, t02, t03, t04, t05, t06, t07, t08, t09));
}

template <typename T00, typename T01, typename T02, typename T03, typename T04, typename T05, typename T06, typename T07, typename T08, typename T09, typename T10>
inline void printf(T00 t00, T01 t01, T02 t02, T03 t03, T04 t04, T05 t05, T06 t06, T07 t07, T08 t08, T09 t09, T10 t10)
{
    printfHelper(toString(t00, t01, t02, t03, t04, t05, t06, t07, t08, t09, t10));
}

template <typename T00, typename T01, typename T02, typename T03, typename T04, typename T05, typename T06, typename T07, typename T08, typename T09, typename T10, typename T11>
inline void printf(T00 t00, T01 t01, T02 t02, T03 t03, T04 t04, T05 t05, T06 t06, T07 t07, T08 t08, T09 t09, T10 t10, T11 t11)
{
    printfHelper(toString(t00, t01, t02, t03, t04, t05, t06, t07, t08, t09, t10, t11));
}

}

#endif
