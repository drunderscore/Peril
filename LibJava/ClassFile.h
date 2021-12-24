#pragma once

#include <AK/EnumBits.h>
#include <AK/Error.h>
#include <AK/Stream.h>
#include <AK/String.h>
#include <AK/Variant.h>
#include <AK/Vector.h>
#include <LibJava/Types.h>

namespace Java
{
// 4.1 The ClassFile Structure
class ClassFile
{
public:
    // Table 4.1-B. Class access and property modifiers
    // Table 4.5-A. Field access and property flags
    enum class AccessFlags : u16
    {
        Public = 0x0001,
        Final = 0x0010,
        Super = 0x0020,
        Interface = 0x0200,
        Abstract = 0x4000,
        Synthetic = 0x1000,
        Annotation = 0x2000,
        Enum = 0x4000,
        Module = 0x8000
    };

    // TODO: Move all these!
    struct Class
    {
        BigEndian<u16> name_index;
    };

    struct FieldRef
    {
        BigEndian<u16> class_index;
        BigEndian<u16> name_and_type_index;
    };

    struct MethodRef
    {
        BigEndian<u16> class_index;
        BigEndian<u16> name_and_type_index;
    };

    struct InterfaceMethodRef
    {
        BigEndian<u16> class_index;
        BigEndian<u16> name_and_type_index;
    };

    // TODO: Either rename this or move it -- it's way too conflicting.
    struct String
    {
        BigEndian<u16> string_index;
    };

    struct NameAndType
    {
        BigEndian<u16> name_index;
        BigEndian<u16> descriptor_index;
    };

    struct Utf8
    {
        // TODO: This is different than what the JVM Specification says. Is this okay?
        AK::String value;
    };

    struct MethodHandle
    {
        u8 reference_kind{};
        BigEndian<u16> reference_index;
    };

    struct MethodType
    {
        BigEndian<u16> descriptor_index;
    };

    struct Dynamic
    {
        BigEndian<u16> bootstrap_method_attr_index;
        BigEndian<u16> name_and_type_index;
    };

    struct InvokeDynamic
    {
        BigEndian<u16> bootstrap_method_attr_index;
        BigEndian<u16> name_and_type_index;
    };

    struct Module
    {
        BigEndian<u16> name_index;
    };

    struct Package
    {
        BigEndian<u16> name_index;
    };

    struct SourceFile
    {
        BigEndian<u16> source_file_index;
    };

    struct ConstantValue
    {
        BigEndian<u16> constant_value_index;
    };

    struct Code
    {
        struct ExceptionHandler
        {
            BigEndian<u16> start_pc;
            BigEndian<u16> end_pc;
            BigEndian<u16> handler_pc;
            BigEndian<u16> catch_type;
        };

        BigEndian<u16> max_stacks;
        BigEndian<u16> max_locals;
        Vector<u8> code;
        Vector<ExceptionHandler> exception_table;
        // TODO: Attributes
    };

    using Attribute = Variant<SourceFile, Code, ConstantValue>;

    struct FieldInfo
    {
        enum class AccessFlags : u16
        {
            Public = 0x0001,
            Private = 0x0002,
            Protected = 0x0004,
            Static = 0x0008,
            Final = 0x0010,
            Volatile = 0x0040,
            Transient = 0x0080,
            Synthetic = 0x1000,
            Enum = 0x4000
        };

        AccessFlags access_flags;
        BigEndian<u16> name_index;
        BigEndian<u16> descriptor_index;

        Optional<ConstantValue*> constant_value;
        Vector<Attribute> attributes;
    };

    struct MethodInfo
    {
        enum class AccessFlags : u16
        {
            Public = 0x0001,
            Private = 0x0002,
            Protected = 0x0004,
            Static = 0x0008,
            Final = 0x0010,
            Synchronized = 0x0020,
            Bridge = 0x0040,
            Varargs = 0x0080,
            Native = 0x0100,
            Abstract = 0x0400,
            Strict = 0x0800,
            Synthetic = 0x1000
        };

        AccessFlags access_flags;
        BigEndian<u16> name_index;
        BigEndian<u16> descriptor_index;
        Vector<Attribute> attributes;
    };

    // The Empty is specifically used for gaps in the constant pool table, which occur with Long and Double types
    // If this sounds dumb, that's because it is :^)
    // To quote the JVM Specification:
    // "In retrospect, making 8-byte constants take two constant pool entries was a poor choice."
    using ConstantType =
        Variant<Class, FieldRef, MethodRef, InterfaceMethodRef, String, Integer, Float, Long, Double, NameAndType, Utf8,
                MethodHandle, MethodType, Dynamic, InvokeDynamic, Module, Package, Empty>;

    static ErrorOr<ClassFile> try_parse(InputStream&);

    AccessFlags access_flags() const { return m_access_flags; }

    const Vector<ConstantType>& constant_pool() const { return m_constant_pool; }

    const ClassFile::Class& this_class() const { return *m_this_class; }

    const ClassFile::Class& super_class() const { return *m_super_class; }

    const Vector<FieldInfo>& fields() const { return m_fields; }

    const Vector<MethodInfo>& methods() const { return m_methods; }

    const Vector<Attribute>& attributes() const { return m_attributes; }

    bool operator==(const ClassFile& other) const
    {
        auto& this_name = constant_pool()[this_class().name_index - 1].get<Java::ClassFile::Utf8>();
        auto& other_name = other.constant_pool()[other.this_class().name_index - 1].get<Java::ClassFile::Utf8>();
        return this_name.value == other_name.value;
    }

private:
    ClassFile() = default;
    // Table 4.1-A. class file format major versions
    BigEndian<u16> m_minor_version;
    BigEndian<u16> m_major_version;
    Vector<ConstantType> m_constant_pool;
    AccessFlags m_access_flags{};
    Class* m_this_class{};
    Class* m_super_class{};
    Vector<Class*> m_interfaces;
    Vector<FieldInfo> m_fields;
    Vector<MethodInfo> m_methods;
    Vector<Attribute> m_attributes;

    ErrorOr<Attribute> try_parse_attribute(InputStream&, Utf8&);
};

AK_ENUM_BITWISE_OPERATORS(ClassFile::AccessFlags);
AK_ENUM_BITWISE_OPERATORS(ClassFile::FieldInfo::AccessFlags);
AK_ENUM_BITWISE_OPERATORS(ClassFile::MethodInfo::AccessFlags);
}

namespace AK
{
template<>
struct Traits<Java::ClassFile> : public GenericTraits<Java::ClassFile>
{
    static unsigned hash(const Java::ClassFile& value)
    {
        auto& name = value.constant_pool()[value.this_class().name_index - 1].get<Java::ClassFile::Utf8>();
        return string_hash(name.value.characters(), name.value.length());
    }
};
}