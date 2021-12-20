#include <LibJava/Disassembler.h>
#include <LibJava/Opcode.h>

namespace Java
{
Disassembler::Disassembler(ClassFile& class_file) : m_class_file(class_file) {}

// FIXME: Should we just expect well-formed code?
//        The parser is supposed to ensure this, but what if we have manually created class files in code?
//        Failing miserably isn't great but the error checking is in some places unnecessary
ErrorOr<Vector<String>> Disassembler::disassemble(const ClassFile::MethodInfo& method)
{
    const ClassFile::Code* code = nullptr;

    for (auto& attribute : method.attributes)
    {
        if (attribute.has<ClassFile::Code>())
        {
            code = attribute.get_pointer<ClassFile::Code>();
            break;
        }
    }

    if (!code)
        return Error::from_string_literal("Method does not have code");

    Vector<String> disassembled_instructions;

    for (auto i = 0; i < code->code.size(); i++)
    {
        StringBuilder instruction;

        auto op = static_cast<Java::Opcode>(code->code[i]);
        auto maybe_op_name = opcode_names.get(op);
        if (!maybe_op_name.has_value())
            return Error::from_string_literal("Encountered invalid opcode");

        auto op_name = maybe_op_name.release_value();

        if (m_numbered_instructions)
            instruction.appendff("{} "sv, i);

        instruction.append(op_name);
        instruction.append(' ');

        switch (op)
        {
            case Opcode::aload:
            case Opcode::bipush:
            case Opcode::iload:
            case Opcode::astore:
            case Opcode::ret:
            case Opcode::fload:
            case Opcode::fstore:
            case Opcode::istore:
            case Opcode::lload:
            case Opcode::lstore:
                instruction.appendff("{}"sv, code->code[i + 1]);
                i++;
                break;

            case Opcode::dload:
            case Opcode::dstore:
                instruction.appendff("{}"sv, code->code[i + 1] << 8 | code->code[i + 2]);
                i += 2;
                break;

            case Opcode::anewarray:
            case Opcode::checkcast:
                // FIXME: CLion/clang-format hurt itself in confusion! (there is an odd space after the ::)
            case Opcode:: instanceof:
            {
                auto index = code->code[i + 1] << 8 | code->code[i + 2];
                auto& type = m_class_file.constant_pool()[index - 1];
                if (!type.has<ClassFile::Class>())
                    return Error::from_string_literal("Expected index into constant pool to be a Class");

                auto& name =
                    m_class_file.constant_pool()[type.get<ClassFile::Class>().name_index - 1].get<ClassFile::Utf8>();

                instruction.appendff(" #{} ("sv, index);

                instruction.append(name.value);
                instruction.append(')');

                i += 2;
            }
            break;
            case Opcode::getfield:
            case Opcode::getstatic:
            case Opcode::putfield:
            case Opcode::putstatic:
            {
                auto index = code->code[i + 1] << 8 | code->code[i + 2];

                auto& type = m_class_file.constant_pool()[index - 1].get<ClassFile::FieldRef>();

                auto& field_name_and_type =
                    m_class_file.constant_pool()[type.name_and_type_index - 1].get<ClassFile::NameAndType>();
                auto& field_name =
                    m_class_file.constant_pool()[field_name_and_type.name_index - 1].get<ClassFile::Utf8>();
                auto& field_class = m_class_file.constant_pool()[type.class_index - 1].get<ClassFile::Class>();
                auto& class_name = m_class_file.constant_pool()[field_class.name_index - 1].get<ClassFile::Utf8>();

                // FIXME: does it make sense to include the class name like this for non-statics?
                instruction.appendff("#{} ({}.{})"sv, index, class_name.value, field_name.value);

                i += 2;
            }
            break;

            case Opcode::goto_w:
            case Opcode::jsr_w:
            {
                auto index = (((code->code[i + 1] << 24) | code->code[i + 2] << 16) | code->code[i + 3] << 8) |
                             code->code[i + 4];
                auto absolute_index = i + index;
                instruction.appendff("{} ({})"sv, index, absolute_index);
                i += 4;
            }
            break;

            case Opcode::goto_:
            case Opcode::if_acmpeq:
            case Opcode::if_acmpne:
            case Opcode::if_icmpeq:
            case Opcode::if_icmpne:
            case Opcode::if_icmplt:
            case Opcode::if_icmpge:
            case Opcode::if_icmpgt:
            case Opcode::if_icmple:
            case Opcode::ifnonnull:
            case Opcode::ifnull:
            case Opcode::jsr:
            {
                auto index = code->code[i + 1] << 8 | code->code[i + 2];
                auto absolute_index = i + index;
                instruction.appendff("{} ({})"sv, index, absolute_index);
                i += 2;
            }
            break;

            case Opcode::ldc:
            case Opcode::ldc_w:
            {
                u16 index = code->code[i + 1];
                // TODO: Verify this works
                if (op == Opcode::ldc_w)
                {
                    (index <<= 8) | code->code[i + 2];
                    i++;
                }

                auto& type = m_class_file.constant_pool()[index - 1];
                instruction.appendff("#{} "sv, index);

                // Long and Double cannot appear in ldc
                // Table 4.4-C. Loadable constant pool tags
                type.downcast<Integer, Float, ClassFile::String, ClassFile::MethodHandle, ClassFile::MethodType,
                              ClassFile::Dynamic, ClassFile::Class>()
                    .visit([&instruction](Integer& value) { instruction.appendff("({})"sv, value); },
                           [&instruction](Float& value) { instruction.appendff("({})"sv, value); },
                           [&instruction, this](ClassFile::String& value) {
                               auto& utf8_string =
                                   m_class_file.constant_pool()[value.string_index - 1].get<ClassFile::Utf8>();
                               instruction.appendff("(\"{}\")"sv, utf8_string.value);
                           },
                           [&instruction](ClassFile::MethodHandle& value) {
                               // TODO: support method handles
                               instruction.append("(method ref)"sv);
                           },
                           [&instruction, this](ClassFile::MethodType& value) {
                               auto& descriptor =
                                   m_class_file.constant_pool()[value.descriptor_index - 1].get<ClassFile::Utf8>();
                               instruction.appendff("({})"sv, descriptor.value);
                           },
                           [&instruction, this](ClassFile::Dynamic& value) {
                               auto& name_and_type = m_class_file.constant_pool()[value.name_and_type_index - 1]
                                                         .get<ClassFile::NameAndType>();
                               auto& name =
                                   m_class_file.constant_pool()[name_and_type.name_index - 1].get<ClassFile::Utf8>();
                               instruction.appendff("({})"sv, name.value);
                           },
                           [&instruction, this](ClassFile::Class& value) {
                               auto& name = m_class_file.constant_pool()[value.name_index - 1].get<ClassFile::Utf8>();
                               instruction.appendff("({})"sv, name.value);
                           });

                i++;
            }
            break;

            case Opcode::ldc2_w:
            {
                auto index = code->code[i + 1] << 8 | code->code[i + 2];

                auto& type = m_class_file.constant_pool()[index - 1];
                instruction.appendff("#{} "sv, index);

                // Table 4.4-C. Loadable constant pool tags
                type.downcast<Long, Double>().visit(
                    [&instruction](Long& value) { instruction.appendff("({})"sv, value); },
                    [&instruction](Double& value) { instruction.appendff("({})"sv, value); });

                i += 2;
            }
            break;

            case Opcode::lookupswitch:
                return Error::from_string_literal("Encountered lookupswitch (TODO implementation)");

            case Opcode::tableswitch:
                return Error::from_string_literal("Encountered tableswitch (TODO implementation)");

            case Opcode::multianewarray:
            {
                auto index = code->code[i + 1] << 8 | code->code[i + 2];
                auto dimensions = code->code[i + 3];

                auto& type = m_class_file.constant_pool()[index - 1];
                if (!type.has<ClassFile::Class>())
                    return Error::from_string_literal("Expected index into constant pool to be a Class");

                auto& name =
                    m_class_file.constant_pool()[type.get<ClassFile::Class>().name_index - 1].get<ClassFile::Utf8>();

                instruction.appendff("#{} ({}, {} dimension"sv, index, name.value, dimensions);

                if (dimensions > 1)
                    instruction.append('s');

                instruction.append(')');

                i += 2;
            }
            break;

            case Opcode::new_:
            {
                auto index = code->code[i + 1] << 8 | code->code[i + 2];
                auto& type = m_class_file.constant_pool()[index - 1];
                if (!type.has<ClassFile::Class>())
                    return Error::from_string_literal("Expected index into constant pool to be a Class");

                auto& name =
                    m_class_file.constant_pool()[type.get<ClassFile::Class>().name_index - 1].get<ClassFile::Utf8>();

                instruction.appendff("#{} ({})"sv, index, name.value);

                i += 2;
            }
            break;

            case Opcode::newarray:
            {
                auto type = code->code[i + 1];

                switch (type)
                {
                    case 4:
                        instruction.append("boolean[]"sv);
                        break;
                    case 5:
                        instruction.append("char[]"sv);
                        break;
                    case 6:
                        instruction.append("float[]"sv);
                        break;
                    case 7:
                        instruction.append("double[]"sv);
                        break;
                    case 8:
                        instruction.append("byte[]"sv);
                        break;
                    case 9:
                        instruction.append("short[]"sv);
                        break;
                    case 10:
                        instruction.append("int[]"sv);
                        break;
                    case 11:
                        instruction.append("long[]"sv);
                        break;
                    default:
                        return Error::from_string_literal("Encountered invalid primitive array type");
                }

                i++;
            }
            break;

            case Opcode::sipush:
                instruction.appendff("{}"sv, code->code[i + 1] << 8 | code->code[i + 2]);
                i += 2;
                break;

            case Opcode::iinc:
            {
                instruction.appendff("{}, "sv, code->code[i + 1]);
                AK::FormatBuilder format_builder_for_sign(instruction);
                TRY(format_builder_for_sign.put_i64(static_cast<i8>(code->code[i + 2]), 10, false, false, false,
                                                    AK::FormatBuilder::Align::Right, 0, ' ',
                                                    AK::FormatBuilder::SignMode::Always));
                i += 2;
                break;
            }

                // TODO: Implement invokedynamic and invokeinterface
            case Opcode::invokedynamic:
            case Opcode::invokeinterface:
                instruction.append("TODO"sv);
                i += 4;
                break;

            case Opcode::invokespecial:
            case Opcode::invokestatic:
            case Opcode::invokevirtual:
            {
                auto index = code->code[i + 1] << 8 | code->code[i + 2];
                auto& type = m_class_file.constant_pool()[index - 1].get<ClassFile::MethodRef>();

                auto& method_name_and_type =
                    m_class_file.constant_pool()[type.name_and_type_index - 1].get<ClassFile::NameAndType>();
                auto& method_name =
                    m_class_file.constant_pool()[method_name_and_type.name_index - 1].get<ClassFile::Utf8>();
                auto& method_class = m_class_file.constant_pool()[type.class_index - 1].get<ClassFile::Class>();
                auto& class_name = m_class_file.constant_pool()[method_class.name_index - 1].get<ClassFile::Utf8>();
                // FIXME: does it make sense to include the class name like this for non-statics?
                instruction.appendff("#{} ({}.{})"sv, index, class_name.value, method_name.value);

                i += 2;
            }
            break;

            case Opcode::wide:
                return Error::from_string_literal("Encountered wide (TODO implementation)");

            default:
                break;
        }

        disassembled_instructions.append(instruction.to_string());
    }

    return disassembled_instructions;
}
}