#pragma once

#include <AK/DistinctNumeric.h>
#include <AK/Stream.h>
#include <AK/Types.h>

namespace Java
{
#define TYPEDEF_PRIMITIVE(type, name)                                                                                  \
    TYPEDEF_DISTINCT_NUMERIC_GENERAL(type, true, true, true, true, true, true, name)                                   \
    static OutputStream& operator<<(OutputStream& stream, const name& value)                                           \
    {                                                                                                                  \
        stream << value.value();                                                                                       \
        return stream;                                                                                                 \
    }                                                                                                                  \
                                                                                                                       \
    static InputStream& operator>>(InputStream& stream, name& value)                                                   \
    {                                                                                                                  \
        stream >> RemoveConst<type&>(value.value());                                                                   \
        return stream;                                                                                                 \
    }

// 2.2 Data Types
// Although it is a primitive, the JVM specification does not explicitly define a boolean type.
TYPEDEF_PRIMITIVE(BigEndian<i8>, Byte)
TYPEDEF_PRIMITIVE(BigEndian<i16>, Short)
TYPEDEF_PRIMITIVE(BigEndian<i32>, Integer)
TYPEDEF_PRIMITIVE(BigEndian<i64>, Long)
TYPEDEF_PRIMITIVE(BigEndian<u16>, Char)
TYPEDEF_PRIMITIVE(BigEndian<float>, Float)
TYPEDEF_PRIMITIVE(BigEndian<double>, Double)

// TODO: returnAddress type?
// TODO: reference type?

#undef TYPEDEF_PRIMITIVE

}