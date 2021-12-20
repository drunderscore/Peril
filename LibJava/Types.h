#pragma once

#include <AK/DistinctNumeric.h>
#include <AK/Format.h>
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
TYPEDEF_PRIMITIVE(i8, Byte)
TYPEDEF_PRIMITIVE(i16, Short)
TYPEDEF_PRIMITIVE(i32, Integer)
TYPEDEF_PRIMITIVE(i64, Long)
TYPEDEF_PRIMITIVE(u16, Char)
TYPEDEF_PRIMITIVE(float, Float)
TYPEDEF_PRIMITIVE(double, Double)

enum class PrimitiveType : u8
{
    Byte,
    Short,
    Int,
    Long,
    Char,
    Float,
    Double,
    Boolean,
    // This primitive is not directly tied to a JLS type -- it is strictly for JVM use.
    ReturnAddress
};

// TODO: returnAddress type?
// TODO: reference type?

#undef TYPEDEF_PRIMITIVE
}

namespace AK
{
template<>
struct Formatter<Java::PrimitiveType> : Formatter<String>
{
    ErrorOr<void> format(FormatBuilder& builder, const Java::PrimitiveType& value)
    {
        switch (value)
        {
            case Java::PrimitiveType::Byte:
                return builder.put_literal("byte"sv);
            case Java::PrimitiveType::Char:
                return builder.put_literal("char"sv);
            case Java::PrimitiveType::Double:
                return builder.put_literal("double"sv);
            case Java::PrimitiveType::Float:
                return builder.put_literal("float"sv);
            case Java::PrimitiveType::Int:
                return builder.put_literal("int"sv);
            case Java::PrimitiveType::Long:
                return builder.put_literal("long"sv);
            case Java::PrimitiveType::Short:
                return builder.put_literal("short"sv);
            case Java::PrimitiveType::Boolean:
                return builder.put_literal("boolean"sv);
            default:
                VERIFY_NOT_REACHED();
        }
    }
};
}