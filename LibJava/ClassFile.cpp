#include <LibJava/ClassFile.h>

namespace Java
{
constexpr u32 class_file_magic = 0xCAFEBABE;

ErrorOr<ClassFile> ClassFile::try_parse(InputStream& stream)
{
    BigEndian<u32> magic;
    stream >> magic;

    if (magic != class_file_magic)
        return Error::from_string_literal("Invalid file magic");

    ClassFile class_file;
    stream >> class_file.m_minor_version;
    stream >> class_file.m_major_version;

    BigEndian<u16> constant_pool_count;
    stream >> constant_pool_count;
    for (auto i = 0; i < constant_pool_count - 1; i++)
    {
        u8 tag;
        stream >> tag;

        // For the constant primitive types, we can't say, for example, BigEndian<Integer>, because to take that integer
        // back out we'd have to tell it the entire template of the DistinctNumber... so let's not do that.
        switch (tag)
        {
            case 1:
            {
                BigEndian<u16> length;
                stream >> length;

                auto buffer = *ByteBuffer::create_uninitialized(length);
                stream >> buffer;

                Utf8 constant;
                constant.value = move(buffer);

                class_file.m_constant_pool.append(move(constant));
            }
            break;
            case 3:
            {
                BigEndian<i32> constant;
                stream >> constant;
                class_file.m_constant_pool.append(Integer(constant));
            }
            break;
            case 4:
            {
                // FIXME: BigEndian fails with floating-point types
                BigEndian<u32> constant;
                stream >> constant;
                auto constant_value = constant.operator unsigned int();
                auto* constant_value_as_float = reinterpret_cast<float*>(&constant_value);
                class_file.m_constant_pool.append(Float(*constant_value_as_float));
            }
            break;
            case 5:
            {
                BigEndian<i64> constant;
                stream >> constant;
                class_file.m_constant_pool.append(Long(constant));
                // The constant_pool index n+1 must be valid but is considered unusable.
                class_file.m_constant_pool.append({});
                // ... the next usable entry in the table is located at index n+2
                i++;
            }
            break;
            case 6:
            {
                // FIXME: BigEndian fails with floating-point types
                BigEndian<u64> constant;
                stream >> constant;
                auto constant_value = constant.operator unsigned long();
                auto* constant_value_as_double = reinterpret_cast<double*>(&constant_value);
                class_file.m_constant_pool.append(Double(*constant_value_as_double));
                // The constant_pool index n+1 must be valid but is considered unusable.
                class_file.m_constant_pool.append({});
                // ... the next usable entry in the table is located at index n+2
                i++;
            }
            break;
            case 7:
            {
                Class constant;
                stream >> constant.name_index;
                class_file.m_constant_pool.append(move(constant));
            }
            break;
            case 8:
            {
                String constant;
                stream >> constant.string_index;
                class_file.m_constant_pool.append(move(constant));
            }
            break;
            case 9:
            {
                FieldRef constant;
                stream >> constant.class_index;
                stream >> constant.name_and_type_index;
                class_file.m_constant_pool.append(move(constant));
            }
            break;
            case 10:
            {
                MethodRef constant;
                stream >> constant.class_index;
                stream >> constant.name_and_type_index;
                class_file.m_constant_pool.append(move(constant));
            }
            break;
            case 11:
            {
                InterfaceMethodRef constant;
                stream >> constant.class_index;
                stream >> constant.name_and_type_index;
                class_file.m_constant_pool.append(move(constant));
            }
            break;
            case 12:
            {
                NameAndType constant;
                stream >> constant.name_index;
                stream >> constant.descriptor_index;
                class_file.m_constant_pool.append(move(constant));
            }
            break;
            case 15:
            {
                MethodHandle constant;
                stream >> constant.reference_kind;
                stream >> constant.reference_index;
                class_file.m_constant_pool.append(move(constant));
            }
            break;
            case 16:
            {
                MethodType constant;
                stream >> constant.descriptor_index;
                class_file.m_constant_pool.append(move(constant));
            }
            break;
            case 17:
            {
                Dynamic constant;
                stream >> constant.bootstrap_method_attr_index;
                stream >> constant.name_and_type_index;
                class_file.m_constant_pool.append(move(constant));
            }
            break;
            case 18:
            {
                InvokeDynamic constant;
                stream >> constant.bootstrap_method_attr_index;
                stream >> constant.name_and_type_index;
                class_file.m_constant_pool.append(move(constant));
            }
            break;
            case 19:
            {
                Module constant;
                stream >> constant.name_index;
                class_file.m_constant_pool.append(move(constant));
            }
            break;
            case 20:
            {
                Package constant;
                stream >> constant.name_index;
                class_file.m_constant_pool.append(move(constant));
            }
            break;
            default:
                return Error::from_string_literal("Constant pool has unknown constant tag");
        }
    }

    BigEndian<u16> access_flags;
    BigEndian<u16> this_class;
    BigEndian<u16> super_class;

    stream >> access_flags;
    stream >> this_class;
    stream >> super_class;
    class_file.m_access_flags = static_cast<AccessFlags>(access_flags.operator unsigned short());
    class_file.m_this_class = this_class;
    class_file.m_super_class = super_class;

    BigEndian<u16> interfaces_count;
    stream >> interfaces_count;

    for (auto i = 0; i < interfaces_count; i++)
    {
        BigEndian<u16> interface_index;
        stream >> interface_index;
        auto& interface = class_file.m_constant_pool.at(interface_index - 1);
        if (!interface.has<Class>())
            return Error::from_string_literal("Interface index into constant pool is not a Class");

        auto& interface_name_index = class_file.m_constant_pool.at(interface.get<Class>().name_index - 1);
        if (!interface_name_index.has<Utf8>())
            return Error::from_string_literal("Interface name index into constant pool is not a Utf8");

        class_file.m_interfaces.append(interface.get_pointer<Class>());
    }

    BigEndian<u16> fields_count;
    stream >> fields_count;

    for (auto i = 0; i < fields_count; i++)
    {
        FieldInfo info;
        BigEndian<u16> field_access_flags;
        stream >> field_access_flags;
        stream >> info.name_index;
        stream >> info.descriptor_index;
        info.access_flags = static_cast<FieldInfo::AccessFlags>(field_access_flags.operator unsigned short());

        if (!class_file.m_constant_pool.at(info.name_index - 1).has<Utf8>())
            return Error::from_string_literal("Field name index into constant pool is not a Utf8");

        if (!class_file.m_constant_pool.at(info.descriptor_index - 1).has<Utf8>())
            return Error::from_string_literal("Field descriptor index into constant pool is not a Utf8");

        BigEndian<u16> attribute_count;
        stream >> attribute_count;
        for (auto j = 0; j < attribute_count; j++)
        {
            BigEndian<u16> attribute_name_index;
            stream >> attribute_name_index;

            auto& name = class_file.m_constant_pool.at(attribute_name_index - 1).get<Utf8>();
            // TODO: Verify this attribute is applicable to fields
            auto attribute_or_error = class_file.try_parse_attribute(stream, name);

            // FIXME: This should be fatal
            if (!attribute_or_error.is_error())
            {
                info.attributes.append(attribute_or_error.release_value());

                auto& attribute_in_vector = info.attributes.last();
                if (attribute_in_vector.has<ConstantValue>())
                    info.constant_value = info.attributes.last().get_pointer<ConstantValue>();
            }
        }

        class_file.m_fields.append(move(info));
    }

    BigEndian<u16> method_count;
    stream >> method_count;

    for (auto i = 0; i < method_count; i++)
    {
        MethodInfo info;
        BigEndian<u16> method_access_flags;
        stream >> method_access_flags;
        stream >> info.name_index;
        stream >> info.descriptor_index;
        info.access_flags = static_cast<MethodInfo::AccessFlags>(method_access_flags.operator unsigned short());

        if (!class_file.m_constant_pool.at(info.name_index - 1).has<Utf8>())
            return Error::from_string_literal("Method name index into constant pool is not a Utf8");

        if (!class_file.m_constant_pool.at(info.descriptor_index - 1).has<Utf8>())
            return Error::from_string_literal("Method descriptor index into constant pool is not a Utf8");

        BigEndian<u16> attribute_count;
        stream >> attribute_count;
        for (auto j = 0; j < attribute_count; j++)
        {
            BigEndian<u16> attribute_name_index;
            stream >> attribute_name_index;

            auto& name = class_file.m_constant_pool.at(attribute_name_index - 1).get<Utf8>();
            // TODO: Verify this attribute is applicable to methods
            auto attribute_or_error = class_file.try_parse_attribute(stream, name);

            // FIXME: This should be fatal
            if (!attribute_or_error.is_error())
            {
                info.attributes.append(attribute_or_error.release_value());

                auto& attribute_in_vector = info.attributes.last();
                if (attribute_in_vector.has<Code>())
                    info.code = info.attributes.last().get_pointer<Code>();
            }
        }

        class_file.m_methods.append(move(info));
    }

    BigEndian<u16> attributes_count;
    stream >> attributes_count;

    for (auto i = 0; i < attributes_count; i++)
    {
        BigEndian<u16> attribute_name_index;
        stream >> attribute_name_index;

        auto& name = class_file.m_constant_pool.at(attribute_name_index - 1).get<Utf8>();
        // TODO: Verify this attribute is applicable to class files
        auto attribute_or_error = class_file.try_parse_attribute(stream, name);

        // FIXME: This should be fatal
        if (!attribute_or_error.is_error())
            class_file.m_attributes.append(attribute_or_error.release_value());
    }

    // The class file must not be truncated or have extra bytes at the end.
    if (!stream.unreliable_eof())
        return Error::from_string_literal("Class file has extra bytes at the end");

    return class_file;
}

ErrorOr<ClassFile::Attribute> ClassFile::try_parse_attribute(InputStream& stream, Utf8& name)
{
    BigEndian<u32> attribute_length;
    stream >> attribute_length;

    if (name.value == "SourceFile"sv)
    {
        SourceFile attribute;
        stream >> attribute.source_file_index;

        return attribute;
    }
    else if (name.value == "Code"sv)
    {
        Code code;
        BigEndian<u32> code_length;

        stream >> code.max_stacks;
        stream >> code.max_locals;
        stream >> code_length;

        code.code.resize(code_length);
        stream >> code.code.span();

        BigEndian<u16> exception_table_length;
        stream >> exception_table_length;
        code.exception_table.resize(exception_table_length);

        for (auto i = 0; i < exception_table_length; i++)
        {
            auto& exception_handler = code.exception_table[i];
            stream >> exception_handler.start_pc;
            stream >> exception_handler.end_pc;
            stream >> exception_handler.handler_pc;
            stream >> exception_handler.catch_type;
        }

        BigEndian<u16> code_attributes_count;
        stream >> code_attributes_count;

        for (auto i = 0; i < code_attributes_count; i++)
        {
            BigEndian<u16> code_attribute_name_index;
            stream >> code_attribute_name_index;

            auto& code_attribute_name = m_constant_pool.at(code_attribute_name_index - 1).get<Utf8>();
            // TODO: Don't discard these attributes
            (void)try_parse_attribute(stream, code_attribute_name);
        }

        return code;
    }
    else if (name.value == "ConstantValue"sv)
    {
        ConstantValue attribute;
        stream >> attribute.constant_value_index;

        return attribute;
    }
    else
    {
        stream.discard_or_error(attribute_length);
        return Error::from_string_literal(AK::String::formatted("Don't know how to parse attribute {}", name.value));
    }
}
}