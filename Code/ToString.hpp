#ifndef TO_STRING_HPP
#define TO_STRING_HPP

#include <string>

#include <boost/lexical_cast.hpp>

namespace mojo
{

template <typename T00>
inline std::string toString(T00 t00);

template <typename T00, typename T01>
inline std::string toString(T00 t00, T01 t01);

template <typename T00, typename T01, typename T02>
inline std::string toString(T00 t00, T01 t01, T02 t02);

template <typename T00, typename T01, typename T02, typename T03>
inline std::string toString(T00 t00, T01 t01, T02 t02, T03 t03);

template <typename T00, typename T01, typename T02, typename T03, typename T04>
inline std::string toString(T00 t00, T01 t01, T02 t02, T03 t03, T04 t04);

template <typename T00, typename T01, typename T02, typename T03, typename T04, typename T05>
inline std::string toString(T00 t00, T01 t01, T02 t02, T03 t03, T04 t04, T05 t05);

template <typename T00, typename T01, typename T02, typename T03, typename T04, typename T05, typename T06>
inline std::string toString(T00 t00, T01 t01, T02 t02, T03 t03, T04 t04, T05 t05, T06 t06);

template <typename T00, typename T01, typename T02, typename T03, typename T04, typename T05, typename T06, typename T07>
inline std::string toString(T00 t00, T01 t01, T02 t02, T03 t03, T04 t04, T05 t05, T06 t06, T07 t07);

template <typename T00, typename T01, typename T02, typename T03, typename T04, typename T05, typename T06, typename T07, typename T08>
inline std::string toString(T00 t00, T01 t01, T02 t02, T03 t03, T04 t04, T05 t05, T06 t06, T07 t07, T08 t08);

template <typename T00, typename T01, typename T02, typename T03, typename T04, typename T05, typename T06, typename T07, typename T08, typename T09>
inline std::string toString(T00 t00, T01 t01, T02 t02, T03 t03, T04 t04, T05 t05, T06 t06, T07 t07, T08 t08, T09 t09);

template <typename T00, typename T01, typename T02, typename T03, typename T04, typename T05, typename T06, typename T07, typename T08, typename T09, typename T10>
inline std::string toString(T00 t00, T01 t01, T02 t02, T03 t03, T04 t04, T05 t05, T06 t06, T07 t07, T08 t08, T09 t09, T10 t10);

template <typename T00, typename T01, typename T02, typename T03, typename T04, typename T05, typename T06, typename T07, typename T08, typename T09, typename T10, typename T11>
inline std::string toString(T00 t00, T01 t01, T02 t02, T03 t03, T04 t04, T05 t05, T06 t06, T07 t07, T08 t08, T09 t09, T10 t10, T11 t11);

template <typename T>
inline std::string toStringHelper(T t);

template <typename T00>
inline std::string toString(T00 t00)
{
    return std::string(
        toStringHelper(t00));
}

template <typename T00, typename T01>
inline std::string toString(T00 t00, T01 t01)
{
    return std::string(
        toStringHelper(t00) +
        toStringHelper(t01));
}

template <typename T00, typename T01, typename T02>
inline std::string toString(T00 t00, T01 t01, T02 t02)
{
    return std::string(
        toStringHelper(t00) +
        toStringHelper(t01) +
        toStringHelper(t02));
}

template < typename T00, typename T01, typename T02, typename T03 >
inline std::string toString(T00 t00, T01 t01, T02 t02, T03 t03)
{
    return std::string(
        toStringHelper(t00) +
        toStringHelper(t01) +
        toStringHelper(t02) +
        toStringHelper(t03));
}

template <typename T00, typename T01, typename T02, typename T03, typename T04>
inline std::string toString(T00 t00, T01 t01, T02 t02, T03 t03, T04 t04)
{
    return std::string(
        toStringHelper(t00) +
        toStringHelper(t01) +
        toStringHelper(t02) +
        toStringHelper(t03) +
        toStringHelper(t04));
}

template <typename T00, typename T01, typename T02, typename T03, typename T04, typename T05>
inline std::string toString(T00 t00, T01 t01, T02 t02, T03 t03, T04 t04, T05 t05)
{
    return std::string(
        toStringHelper(t00) +
        toStringHelper(t01) +
        toStringHelper(t02) +
        toStringHelper(t03) +
        toStringHelper(t04) +
        toStringHelper(t05));
}

template < typename T00, typename T01, typename T02, typename T03, typename T04, typename T05, typename T06>
inline std::string toString(T00 t00, T01 t01, T02 t02, T03 t03, T04 t04, T05 t05, T06 t06)
{
    return std::string(
        toStringHelper(t00) +
        toStringHelper(t01) +
        toStringHelper(t02) +
        toStringHelper(t03) +
        toStringHelper(t04) +
        toStringHelper(t05) +
        toStringHelper(t06));
}

template <typename T00, typename T01, typename T02, typename T03, typename T04, typename T05, typename T06, typename T07>
inline std::string toString(T00 t00, T01 t01, T02 t02, T03 t03, T04 t04, T05 t05, T06 t06, T07 t07)
{
    return std::string(
        toStringHelper(t00) +
        toStringHelper(t01) +
        toStringHelper(t02) +
        toStringHelper(t03) +
        toStringHelper(t04) +
        toStringHelper(t05) +
        toStringHelper(t06) +
        toStringHelper(t07));
}

template <typename T00, typename T01, typename T02, typename T03, typename T04, typename T05, typename T06, typename T07, typename T08>
inline std::string toString(T00 t00, T01 t01, T02 t02, T03 t03, T04 t04, T05 t05, T06 t06, T07 t07, T08 t08)
{
    return std::string(
        toStringHelper(t00) +
        toStringHelper(t01) +
        toStringHelper(t02) +
        toStringHelper(t03) +
        toStringHelper(t04) +
        toStringHelper(t05) +
        toStringHelper(t06) +
        toStringHelper(t07) +
        toStringHelper(t08));
}

template <typename T00, typename T01, typename T02, typename T03, typename T04, typename T05, typename T06, typename T07, typename T08, typename T09>
inline std::string toString(T00 t00, T01 t01, T02 t02, T03 t03, T04 t04, T05 t05, T06 t06, T07 t07, T08 t08, T09 t09)
{
    return std::string(
        toStringHelper(t00) +
        toStringHelper(t01) +
        toStringHelper(t02) +
        toStringHelper(t03) +
        toStringHelper(t04) +
        toStringHelper(t05) +
        toStringHelper(t06) +
        toStringHelper(t07) +
        toStringHelper(t08) +
        toStringHelper(t09));
}

template <typename T00, typename T01, typename T02, typename T03, typename T04, typename T05, typename T06, typename T07, typename T08, typename T09, typename T10>
inline std::string toString(T00 t00, T01 t01, T02 t02, T03 t03, T04 t04, T05 t05, T06 t06, T07 t07, T08 t08, T09 t09, T10 t10)
{
    return std::string(
        toStringHelper(t00) +
        toStringHelper(t01) +
        toStringHelper(t02) +
        toStringHelper(t03) +
        toStringHelper(t04) +
        toStringHelper(t05) +
        toStringHelper(t06) +
        toStringHelper(t07) +
        toStringHelper(t08) +
        toStringHelper(t09) +
        toStringHelper(t10));
}

template <typename T00, typename T01, typename T02, typename T03, typename T04, typename T05, typename T06, typename T07, typename T08, typename T09, typename T10, typename T11>
inline std::string toString(T00 t00, T01 t01, T02 t02, T03 t03, T04 t04, T05 t05, T06 t06, T07 t07, T08 t08, T09 t09, T10 t10, T11 t11)
{
    return std::string(
        toStringHelper(t00) +
        toStringHelper(t01) +
        toStringHelper(t02) +
        toStringHelper(t03) +
        toStringHelper(t04) +
        toStringHelper(t05) +
        toStringHelper(t06) +
        toStringHelper(t07) +
        toStringHelper(t08) +
        toStringHelper(t09) +
        toStringHelper(t10) +
        toStringHelper(t11));
}

template <typename T>
inline std::string toStringHelper(T t)
{
    return boost::lexical_cast<std::string>(t);
}

}

#endif
