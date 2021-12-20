#pragma once

#include <AK/Error.h>
#include <AK/String.h>
#include <AK/Variant.h>
#include <AK/Vector.h>
#include <LibJava/Types.h>

namespace Java
{
// TODO: Support 4.3.4 Signatures (types that have generics)

class FieldDescriptor
{
public:
    explicit FieldDescriptor(Variant<PrimitiveType, String> type, u8 array_dimensions = 0)
        : m_type(move(type)), m_array_dimensions(array_dimensions)
    {
    }

    // We need to do this bit of weirdness with "original_descriptor_length" for help with MethodDescriptor
    // It has multiple FieldDescriptors in the same StringView all next to each other,
    // but we need to know where one ends to know where the next one starts.
    // FIXME: Remove this pointer and find a more elegant solution. Pointers are the devil!
    static ErrorOr<FieldDescriptor> try_parse(StringView, size_t* original_descriptor_length = nullptr);

    u8 array_dimensions() const { return m_array_dimensions; }

    void set_array_dimensions(u8 value) { m_array_dimensions = value; }

    String to_string() const;

private:
    Variant<PrimitiveType, String> m_type;
    u8 m_array_dimensions{};
};

class MethodDescriptor
{
public:
    explicit MethodDescriptor() {}

    static ErrorOr<MethodDescriptor> try_parse(StringView);

    const Vector<FieldDescriptor> parameters() const { return m_parameters; }
    const Variant<FieldDescriptor, Empty> return_type() const { return m_return_type; }

    String return_type_to_string() const;
    String parameters_to_string() const;
    String to_string() const;

private:
    Vector<FieldDescriptor> m_parameters;
    Variant<FieldDescriptor, Empty> m_return_type;
};
}

namespace AK
{
template<>
struct Formatter<Java::FieldDescriptor> : Formatter<String>
{
    ErrorOr<void> format(FormatBuilder& builder, const Java::FieldDescriptor& value)
    {
        return Formatter<String>::format(builder, value.to_string());
    }
};

template<>
struct Formatter<Java::MethodDescriptor> : Formatter<String>
{
    ErrorOr<void> format(FormatBuilder& builder, const Java::MethodDescriptor& value)
    {
        return Formatter<String>::format(builder, value.to_string());
    }
};
}
