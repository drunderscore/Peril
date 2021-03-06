#include <AK/GenericLexer.h>
#include <LibJava/Descriptor.h>

namespace Java
{
ErrorOr<FieldDescriptor> FieldDescriptor::try_parse(StringView value, size_t* original_descriptor_length)
{
    u8 array_dimensions = 0;

    if (original_descriptor_length)
        *original_descriptor_length = 0;

    for (auto i = 0; i < value.length(); i++)
    {
        if (original_descriptor_length)
            (*original_descriptor_length)++;

        const auto& c = value[i];
        switch (c)
        {
            case '[':
                if (array_dimensions++ == NumericLimits<u8>::max())
                    return Error::from_string_literal("Field descriptor has too many array dimensions (over 255)");
                break;
            case 'B':
                return FieldDescriptor(PrimitiveType::Byte, array_dimensions);
            case 'C':
                return FieldDescriptor(PrimitiveType::Char, array_dimensions);
            case 'D':
                return FieldDescriptor(PrimitiveType::Double, array_dimensions);
            case 'F':
                return FieldDescriptor(PrimitiveType::Float, array_dimensions);
            case 'I':
                return FieldDescriptor(PrimitiveType::Int, array_dimensions);
            case 'J':
                return FieldDescriptor(PrimitiveType::Long, array_dimensions);
            case 'L':
            {
                // +1 to skip the 'L'
                GenericLexer lexer(value.substring_view(i + 1));
                auto name = lexer.consume_until(';');

                // +1 to account for the skipped 'L'
                if (original_descriptor_length)
                    (*original_descriptor_length) += (name.length() + 1);

                // We replace forward slashes with dots, because:
                // 4.2.1 "For historical reasons, the syntax of binary names that appear in class file structures
                // differs from the syntax of binary names documented in JLS §13.1."
                // The dots are more friendly and expected from an API like this.
                return FieldDescriptor(name.to_string().replace("/"sv, "."sv, true), array_dimensions);
            }

            case 'S':
                return FieldDescriptor(PrimitiveType::Short, array_dimensions);
            case 'Z':
                return FieldDescriptor(PrimitiveType::Boolean, array_dimensions);
            default:
                return Error::from_string_literal("Encountered unknown character in field descriptor");
        }
    }

    return Error::from_string_literal("Incomplete field descriptor");
}

ErrorOr<MethodDescriptor> MethodDescriptor::try_parse(StringView value)
{
    // Quick-fail if we can already tell this isn't a method descriptor
    if (value[0] != '(')
        return Error::from_string_literal("Method descriptors must begin with an open parenthesis");

    MethodDescriptor descriptor;

    bool expect_return_next = false;
    // Start at 1, as we already checked above for open parentheses, and has no use for us
    for (auto i = 1; i < value.length(); i++)
    {
        const auto& c = value[i];
        if (expect_return_next)
        {
            if (c != 'V')
                descriptor.m_return_type = TRY(FieldDescriptor::try_parse(value.substring_view(i)));

            break;
        }
        else
        {
            if (c == ')')
            {
                expect_return_next = true;
            }
            else
            {
                size_t original_descriptor_length;
                descriptor.m_parameters.append(
                    TRY(FieldDescriptor::try_parse(value.substring_view(i), &original_descriptor_length)));

                // -1, because next iteration we're going to +1
                i += original_descriptor_length - 1;
            }
        }
    }

    return descriptor;
}

String FieldDescriptor::to_string() const
{
    StringBuilder builder;

    m_type.visit([&builder](const PrimitiveType& value) { builder.appendff("{}"sv, value); },
                 [&builder](const String& value) { builder.append(value); });

    for (auto i = 0; i < m_array_dimensions; i++)
        builder.append("[]"sv);

    return builder.to_string();
}

String MethodDescriptor::parameters_to_string() const
{
    StringBuilder builder;

    for (auto i = 0; i < m_parameters.size(); i++)
    {
        auto& parameter = m_parameters[i];
        builder.appendff("{}"sv, parameter);
        if (i != m_parameters.size() - 1)
            builder.append(", "sv);
    }

    return builder.to_string();
}

String MethodDescriptor::return_type_to_string() const
{
    return m_return_type.visit([](Empty) { return String("void"); },
                               [](const FieldDescriptor& value) { return value.to_string(); });
}

String MethodDescriptor::to_string() const
{
    StringBuilder builder;
    builder.appendff("{} ({})"sv, return_type_to_string(), parameters_to_string());

    return builder.to_string();
}
}