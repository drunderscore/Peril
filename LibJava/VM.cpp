#include <LibJava/Descriptor.h>
#include <LibJava/Opcode.h>
#include <LibJava/VM.h>

namespace Java
{
ErrorOr<void> VM::initialize_class(const ClassFile& class_file)
{
    if (m_static_data.contains(class_file))
        return {};

    StaticData static_data;

    for (auto& field : class_file.fields())
    {
        if (has_flag(field.access_flags, ClassFile::FieldInfo::AccessFlags::Static))
        {
            auto& name = class_file.constant_pool()[field.name_index - 1].get<ClassFile::Utf8>();
            auto& descriptor_string = class_file.constant_pool()[field.descriptor_index - 1].get<ClassFile::Utf8>();
            auto descriptor = TRY(FieldDescriptor::try_parse(descriptor_string.value));

            Optional<Value> initial_value;

            if (field.constant_value.has_value())
            {
                auto& constant_pool_initial_value =
                    class_file.constant_pool()[field.constant_value.value()->constant_value_index - 1];

                constant_pool_initial_value.downcast<Integer, Float, Long, Double>().visit(
                    [&initial_value](Integer& value) { initial_value = value; },
                    [&initial_value](Float& value) { initial_value = value; },
                    [&initial_value](Long& value) { initial_value = value; },
                    [&initial_value](Double& value) { initial_value = value; });
            }

            // TODO: Support String ConstantValue
            if (!descriptor.type().has<PrimitiveType>())
                return Error::from_string_literal("No support for String ConstantValue");

            switch (descriptor.type().get<PrimitiveType>())
            {
                case PrimitiveType::Byte:
                    // Booleans are just Bytes in disguise!
                case PrimitiveType::Boolean:
                    static_data.fields.set(
                        name.value, initial_value.has_value() ? Byte(initial_value.value().get<Integer>().value()) : 0);
                    break;
                case PrimitiveType::Short:
                    static_data.fields.set(name.value, initial_value.has_value()
                                                           ? Short(initial_value.value().get<Integer>().value())
                                                           : 0);
                    break;
                case PrimitiveType::Int:
                    static_data.fields.set(name.value, initial_value.has_value()
                                                           ? Integer(initial_value.value().get<Integer>().value())
                                                           : 0);
                    break;
                case PrimitiveType::Long:
                    static_data.fields.set(
                        name.value, initial_value.has_value() ? Long(initial_value.value().get<Long>().value()) : 0);
                    break;
                case PrimitiveType::Char:
                    static_data.fields.set(
                        name.value, initial_value.has_value() ? Char(initial_value.value().get<Integer>().value()) : 0);
                    break;
                case PrimitiveType::Float:
                    static_data.fields.set(
                        name.value, initial_value.has_value() ? Float(initial_value.value().get<Float>().value()) : 0);
                    break;
                case PrimitiveType::Double:
                    static_data.fields.set(name.value, initial_value.has_value()
                                                           ? Double(initial_value.value().get<Double>().value())
                                                           : 0);
                    break;
                default:
                    return Error::from_string_literal("Invalid primitive type in ConstantValue");
            }
        }
    }

    m_static_data.set(class_file, move(static_data));

    for (auto& method : class_file.methods())
    {
        auto& method_name = class_file.constant_pool()[method.name_index - 1].get<ClassFile::Utf8>();

        if (method_name.value == "<clinit>"sv)
        {
            auto& descriptor_string = class_file.constant_pool()[method.descriptor_index - 1].get<ClassFile::Utf8>();
            auto descriptor = TRY(MethodDescriptor::try_parse(descriptor_string.value));

            if (!descriptor.return_type().has<Empty>())
                return Error::from_string_literal("Class initilization method must have no return value");

            // TODO: In a class file whose version number is 51.0 or above, the method has its
            //       ACC_STATIC flag set and takes no arguments (§4.6).

            TRY(call(class_file, method));
            break;
        }
    }

    return {};
}

ErrorOr<Value> VM::call(const ClassFile& class_file, const ClassFile::MethodInfo& method, Span<Value> arguments)
{
    TRY(initialize_class(class_file));

    const ClassFile::Code* code = nullptr;

    for (auto& kv : method.attributes)
    {
        if (kv.has<ClassFile::Code>())
        {
            code = kv.get_pointer<ClassFile::Code>();
            break;
        }
    }

    if (!code)
        return Error::from_string_literal("Method to execute has no Code attribute");

    Frame frame;

    // TODO: wtf is this!
    //       can't resize because no default construction, but this is worse!

    for (auto i = 0; i < code->max_locals; i++)
        frame.locals.append(Byte(0));

    auto argument_index = 0;
    for (auto& arg : arguments)
    {
        frame.locals[argument_index] = move(arg);
        argument_index++;
    }

    m_stack.append(frame);

    Vector<Value> operand_stack;

    m_program_counter = 0;

    while (m_program_counter < code->code.size())
    {
        auto opcode = static_cast<Opcode>(code->code[m_program_counter]);

        // TODO: type safety! (store ops should do type checking)
        switch (opcode)
        {
            case Opcode::nop:
                break;
            case Opcode::iconst_m1:
                operand_stack.append(Integer(-1));
                break;
            case Opcode::iconst_0:
                operand_stack.append(Integer(0));
                break;
            case Opcode::iconst_1:
                operand_stack.append(Integer(1));
                break;
            case Opcode::iconst_2:
                operand_stack.append(Integer(2));
                break;
            case Opcode::iconst_3:
                operand_stack.append(Integer(3));
                break;
            case Opcode::iconst_4:
                operand_stack.append(Integer(4));
                break;
            case Opcode::iconst_5:
                operand_stack.append(Integer(5));
                break;
            case Opcode::lconst_0:
                operand_stack.append(Long(0));
                break;
            case Opcode::lconst_1:
                operand_stack.append(Long(1));
                break;
            case Opcode::dconst_0:
                operand_stack.append(Double(0));
                break;
            case Opcode::dconst_1:
                operand_stack.append(Double(1));
                break;
            case Opcode::istore_0:
                frame.locals[0] = operand_stack.take_first();
                break;
            case Opcode::istore_1:
                frame.locals[1] = operand_stack.take_first();
                break;
            case Opcode::istore_2:
                frame.locals[2] = operand_stack.take_first();
                break;
            case Opcode::istore_3:
                frame.locals[3] = operand_stack.take_first();
                break;
            case Opcode::dstore_0:
            case Opcode::lstore_0:
            {
                auto value = operand_stack.take_first();
                frame.locals[0] = value;
                frame.locals[1] = value;
                break;
            }
            case Opcode::dstore_1:
            case Opcode::lstore_1:
            {
                auto value = operand_stack.take_first();
                frame.locals[1] = value;
                frame.locals[2] = value;
                break;
            }
            case Opcode::dstore_2:
            case Opcode::lstore_2:
            {
                auto value = operand_stack.take_first();
                frame.locals[2] = value;
                frame.locals[3] = value;
                break;
            }
            case Opcode::dstore_3:
            case Opcode::lstore_3:
            {
                auto value = operand_stack.take_first();
                frame.locals[3] = value;
                frame.locals[4] = value;
                break;
            }
            case Opcode::iload_0:
            case Opcode::lload_0:
            case Opcode::dload_0:
                operand_stack.append(frame.locals[0]);
                break;
            case Opcode::iload_1:
            case Opcode::lload_1:
            case Opcode::dload_1:
                operand_stack.append(frame.locals[1]);
                break;
            case Opcode::iload_2:
            case Opcode::lload_2:
            case Opcode::dload_2:
                operand_stack.append(frame.locals[2]);
                break;
            case Opcode::iload_3:
            case Opcode::lload_3:
            case Opcode::dload_3:
                operand_stack.append(frame.locals[3]);
                break;
            case Opcode::istore:
            case Opcode::lstore:
            case Opcode::dstore:
            case Opcode::fstore:
                frame.locals[code->code[m_program_counter + 1]] = operand_stack.take_first();
                m_program_counter++;
                break;
            case Opcode::iload:
            case Opcode::lload:
            case Opcode::dload:
            case Opcode::fload:
                operand_stack.append(frame.locals[code->code[m_program_counter + 1]]);
                m_program_counter++;
                break;
            case Opcode::bipush:
            {
                auto value = code->code[m_program_counter + 1];
                operand_stack.append(Integer(value));
                m_program_counter++;
                break;
            }
            case Opcode::sipush:
            {
                auto value = (code->code[m_program_counter + 1] << 8 | code->code[m_program_counter + 2]);
                operand_stack.append(Integer(value));
                m_program_counter += 2;
                break;
            }
            case Opcode::ldc:
            {
                auto value_index = code->code[m_program_counter + 1];
                auto& value = class_file.constant_pool()[value_index - 1];

                if (value.has<Integer>())
                    operand_stack.append(value.get<Integer>());
                else if (value.has<Float>())
                    operand_stack.append(value.get<Float>());
                else
                    return Error::from_string_literal(
                        "Missing implementation for types other than Integer and Float in ldc");

                m_program_counter++;
                break;
            }

            case Opcode::i2b:
                operand_stack.append(Byte(operand_stack.take_first().get<Integer>().value()));
                break;
            case Opcode::i2c:
                operand_stack.append(Char(operand_stack.take_first().get<Integer>().value()));
                break;
            case Opcode::i2d:
                operand_stack.append(Double(operand_stack.take_first().get<Integer>().value()));
                break;
            case Opcode::i2f:
                operand_stack.append(Float(operand_stack.take_first().get<Integer>().value()));
                break;
            case Opcode::i2s:
                operand_stack.append(Short(operand_stack.take_first().get<Integer>().value()));
                break;
            case Opcode::i2l:
                operand_stack.append(Long(operand_stack.take_first().get<Integer>().value()));
                break;
            case Opcode::d2f:
                operand_stack.append(Float(operand_stack.take_first().get<Double>().value()));
                break;
            case Opcode::d2i:
                operand_stack.append(Integer(operand_stack.take_first().get<Double>().value()));
                break;
            case Opcode::d2l:
                operand_stack.append(Long(operand_stack.take_first().get<Double>().value()));
                break;
            case Opcode::l2f:
                operand_stack.append(Float(operand_stack.take_first().get<Long>().value()));
                break;
            case Opcode::l2i:
                operand_stack.append(Integer(operand_stack.take_first().get<Long>().value()));
                break;
            case Opcode::l2d:
                operand_stack.append(Double(operand_stack.take_first().get<Long>().value()));
                break;
            case Opcode::f2d:
                operand_stack.append(Double(operand_stack.take_first().get<Float>().value()));
                break;
            case Opcode::f2i:
                operand_stack.append(Integer(operand_stack.take_first().get<Float>().value()));
                break;
            case Opcode::f2l:
                operand_stack.append(Long(operand_stack.take_first().get<Float>().value()));
                break;
            case Opcode::ldc2_w:
            {
                auto value_index = code->code[m_program_counter + 1] << 8 | code->code[m_program_counter + 2];
                auto& value = class_file.constant_pool()[value_index - 1];

                if (value.has<Long>())
                    operand_stack.append(value.get<Long>());
                else if (value.has<Double>())
                    operand_stack.append(value.get<Double>());
                else
                    return Error::from_string_literal("Cannot use ldc2_w on types other than Long and Double");

                m_program_counter += 2;
                break;
            }
            case Opcode::iinc:
            {
                auto index = code->code[m_program_counter + 1];
                auto increment_const = code->code[m_program_counter + 2];

                auto& value = frame.locals[index];
                value.get<Integer>() += increment_const;

                m_program_counter += 2;
                break;
            }
            case Opcode::iadd:
            {
                auto a = operand_stack.take_first();
                auto b = operand_stack.take_first();

                operand_stack.append(add<Integer>(move(a), move(b)));
                break;
            }
            case Opcode::isub:
            {
                auto a = operand_stack.take_first();
                auto b = operand_stack.take_first();

                operand_stack.append(sub<Integer>(move(a), move(b)));
                break;
            }
            case Opcode::imul:
            {
                auto a = operand_stack.take_first();
                auto b = operand_stack.take_first();

                operand_stack.append(mul<Integer>(move(a), move(b)));
                break;
            }
            case Opcode::idiv:
            {
                auto a = operand_stack.take_first();
                auto b = operand_stack.take_first();

                operand_stack.append(div<Integer>(move(a), move(b)));
                break;
            }
            case Opcode::dadd:
            {
                auto a = operand_stack.take_first();
                auto b = operand_stack.take_first();

                operand_stack.append(add<Double>(move(a), move(b)));
                break;
            }
            case Opcode::dsub:
            {
                auto a = operand_stack.take_first();
                auto b = operand_stack.take_first();

                operand_stack.append(sub<Double>(move(a), move(b)));
                break;
            }
            case Opcode::dmul:
            {
                auto a = operand_stack.take_first();
                auto b = operand_stack.take_first();

                operand_stack.append(mul<Double>(move(a), move(b)));
                break;
            }
            case Opcode::ddiv:
            {
                auto a = operand_stack.take_first();
                auto b = operand_stack.take_first();

                operand_stack.append(div<Double>(move(a), move(b)));
                break;
            }
            case Opcode::fadd:
            {
                auto a = operand_stack.take_first();
                auto b = operand_stack.take_first();

                operand_stack.append(add<Float>(move(a), move(b)));
                break;
            }
            case Opcode::fsub:
            {
                auto a = operand_stack.take_first();
                auto b = operand_stack.take_first();

                operand_stack.append(sub<Float>(move(a), move(b)));
                break;
            }
            case Opcode::fmul:
            {
                auto a = operand_stack.take_first();
                auto b = operand_stack.take_first();

                operand_stack.append(mul<Float>(move(a), move(b)));
                break;
            }
            case Opcode::fdiv:
            {
                auto a = operand_stack.take_first();
                auto b = operand_stack.take_first();

                operand_stack.append(div<Float>(move(a), move(b)));
                break;
            }
            case Opcode::ladd:
            {
                auto a = operand_stack.take_first();
                auto b = operand_stack.take_first();

                operand_stack.append(add<Long>(move(a), move(b)));
                break;
            }
            case Opcode::lsub:
            {
                auto a = operand_stack.take_first();
                auto b = operand_stack.take_first();

                operand_stack.append(sub<Long>(move(a), move(b)));
                break;
            }
            case Opcode::lmul:
            {
                auto a = operand_stack.take_first();
                auto b = operand_stack.take_first();

                operand_stack.append(mul<Long>(move(a), move(b)));
                break;
            }
            case Opcode::ldiv:
            {
                auto a = operand_stack.take_first();
                auto b = operand_stack.take_first();

                operand_stack.append(div<Long>(move(a), move(b)));
                break;
            }
            case Opcode::ior:
            {
                auto a = operand_stack.take_first();
                auto b = operand_stack.take_first();

                operand_stack.append(bitwise_inclusive_or<Integer>(move(a), move(b)));
                break;
            }
            case Opcode::ixor:
            {
                auto a = operand_stack.take_first();
                auto b = operand_stack.take_first();

                operand_stack.append(bitwise_exclusive_or<Integer>(move(a), move(b)));
                break;
            }
            case Opcode::iand:
            {
                auto a = operand_stack.take_first();
                auto b = operand_stack.take_first();

                operand_stack.append(bitwise_and<Integer>(move(a), move(b)));
                break;
            }
            case Opcode::lor:
            {
                auto a = operand_stack.take_first();
                auto b = operand_stack.take_first();

                operand_stack.append(bitwise_inclusive_or<Long>(move(a), move(b)));
                break;
            }
            case Opcode::lxor:
            {
                auto a = operand_stack.take_first();
                auto b = operand_stack.take_first();

                operand_stack.append(bitwise_exclusive_or<Long>(move(a), move(b)));
                break;
            }
            case Opcode::land:
            {
                auto a = operand_stack.take_first();
                auto b = operand_stack.take_first();

                operand_stack.append(bitwise_and<Long>(move(a), move(b)));
                break;
            }
            case Opcode::invokestatic:
            {
                auto value_index = code->code[m_program_counter + 1] << 8 | code->code[m_program_counter + 2];
                auto& value = class_file.constant_pool()[value_index - 1].get<ClassFile::MethodRef>();

                // FIXME: don't assume the method is in this class file!
                auto& static_method_to_invoke_name_and_type =
                    class_file.constant_pool()[value.name_and_type_index - 1].get<ClassFile::NameAndType>();
                auto& static_method_to_invoke_name =
                    class_file.constant_pool()[static_method_to_invoke_name_and_type.name_index - 1]
                        .get<ClassFile::Utf8>();
                auto& static_method_to_invoke_descriptor =
                    class_file.constant_pool()[static_method_to_invoke_name_and_type.descriptor_index - 1]
                        .get<ClassFile::Utf8>();

                auto static_method_to_invoke_resolved_descriptor =
                    MethodDescriptor::try_parse(static_method_to_invoke_descriptor.value)
                        .release_value_but_fixme_should_propagate_errors();

                const ClassFile::MethodInfo* static_method_to_invoke = nullptr;

                for (auto& method : class_file.methods())
                {
                    auto& name = class_file.constant_pool()[method.name_index - 1].get<ClassFile::Utf8>();
                    if (name.value == static_method_to_invoke_name.value)
                    {
                        static_method_to_invoke = &method;
                        break;
                    }
                }

                if (!static_method_to_invoke)
                    return Error::from_string_literal("Unable to find method to invoke with invokestatic");

                auto program_counter_to_return_to = m_program_counter;

                auto return_value =
                    TRY(call(class_file, *static_method_to_invoke,
                             {operand_stack.data(), static_method_to_invoke_resolved_descriptor.parameters().size()}));

                m_program_counter = program_counter_to_return_to;

                for (auto i = 0; i < static_method_to_invoke_resolved_descriptor.parameters().size(); i++)
                    operand_stack.remove(0);

                if (!static_method_to_invoke_resolved_descriptor.return_type().has<Empty>())
                    operand_stack.append(move(return_value));

                m_program_counter += 2;
                break;
            }
            case Opcode::goto_:
            {
                auto offset = code->code[m_program_counter + 1] << 8 | code->code[m_program_counter + 2];
                m_program_counter += offset;
                continue;
            }
            case Opcode::goto_w:
            {
                auto offset = (((code->code[m_program_counter + 1] << 24) | code->code[m_program_counter + 2] << 16) |
                               code->code[m_program_counter + 3] << 8) |
                              code->code[m_program_counter + 4];
                m_program_counter += offset;
                continue;
            }
            case Opcode::return_:
                // TODO: return null?
                return Integer(0);

            case Opcode::if_icmpeq:
            {
                auto success_branch_offset = code->code[m_program_counter + 1] << 8 | code->code[m_program_counter + 2];
                auto a = operand_stack.take_first();
                auto b = operand_stack.take_first();

                if (if_equal<Integer>(move(a), move(b), success_branch_offset))
                    continue;

                m_program_counter += 2;
                break;
            }
            case Opcode::if_icmpne:
            {
                auto success_branch_offset = code->code[m_program_counter + 1] << 8 | code->code[m_program_counter + 2];
                auto a = operand_stack.take_first();
                auto b = operand_stack.take_first();

                if (if_not_equal<Integer>(move(a), move(b), success_branch_offset))
                    continue;

                m_program_counter += 2;
                break;
            }
            case Opcode::if_icmplt:
            {
                auto success_branch_offset = code->code[m_program_counter + 1] << 8 | code->code[m_program_counter + 2];
                auto a = operand_stack.take_first();
                auto b = operand_stack.take_first();

                if (if_less_than<Integer>(move(a), move(b), success_branch_offset))
                    continue;

                m_program_counter += 2;
                break;
            }
            case Opcode::if_icmpge:
            {
                auto success_branch_offset = code->code[m_program_counter + 1] << 8 | code->code[m_program_counter + 2];
                auto a = operand_stack.take_first();
                auto b = operand_stack.take_first();

                if (if_greater_than_or_equal_to<Integer>(move(a), move(b), success_branch_offset))
                    continue;

                m_program_counter += 2;
                break;
            }
            case Opcode::if_icmpgt:
            {
                auto success_branch_offset = code->code[m_program_counter + 1] << 8 | code->code[m_program_counter + 2];
                auto a = operand_stack.take_first();
                auto b = operand_stack.take_first();

                if (if_greater_than<Integer>(move(a), move(b), success_branch_offset))
                    continue;

                m_program_counter += 2;
                break;
            }
            case Opcode::if_icmple:
            {
                auto success_branch_offset = code->code[m_program_counter + 1] << 8 | code->code[m_program_counter + 2];
                auto a = operand_stack.take_first();
                auto b = operand_stack.take_first();

                if (if_less_than_or_equal_to<Integer>(move(a), move(b), success_branch_offset))
                    continue;

                m_program_counter += 2;
                break;
            }
            case Opcode::ifeq:
            {
                auto success_branch_offset = code->code[m_program_counter + 1] << 8 | code->code[m_program_counter + 2];

                if (if_equal<Integer>(operand_stack.take_first(), 0, success_branch_offset))
                    continue;

                m_program_counter += 2;
                break;
            }
            case Opcode::ifne:
            {
                auto success_branch_offset = code->code[m_program_counter + 1] << 8 | code->code[m_program_counter + 2];

                if (if_not_equal<Integer>(operand_stack.take_first(), 0, success_branch_offset))
                    continue;

                m_program_counter += 2;
                break;
            }
            case Opcode::iflt:
            {
                auto success_branch_offset = code->code[m_program_counter + 1] << 8 | code->code[m_program_counter + 2];

                if (if_less_than<Integer>(operand_stack.take_first(), 0, success_branch_offset))
                    continue;

                m_program_counter += 2;
                break;
            }
            case Opcode::ifge:
            {
                auto success_branch_offset = code->code[m_program_counter + 1] << 8 | code->code[m_program_counter + 2];

                if (if_greater_than_or_equal_to<Integer>(operand_stack.take_first(), 0, success_branch_offset))
                    continue;

                m_program_counter += 2;
                break;
            }
            case Opcode::ifgt:
            {
                auto success_branch_offset = code->code[m_program_counter + 1] << 8 | code->code[m_program_counter + 2];

                if (if_greater_than<Integer>(operand_stack.take_first(), 0, success_branch_offset))
                    continue;

                m_program_counter += 2;
                break;
            }
            case Opcode::ifle:
            {
                auto success_branch_offset = code->code[m_program_counter + 1] << 8 | code->code[m_program_counter + 2];

                if (if_less_than_or_equal_to<Integer>(operand_stack.take_first(), 0, success_branch_offset))
                    continue;

                m_program_counter += 2;
                break;
            }

            case Opcode::ireturn:
            case Opcode::dreturn:
            case Opcode::freturn:
            case Opcode::lreturn:
                return operand_stack.take_first();

            case Opcode::ineg:
                operand_stack.append(Integer(-operand_stack.take_first().get<Integer>()));
                break;
            case Opcode::fneg:
                operand_stack.append(Float(-operand_stack.take_first().get<Float>()));
                break;
            case Opcode::dneg:
                operand_stack.append(Double(-operand_stack.take_first().get<Double>()));
                break;
            case Opcode::lneg:
                operand_stack.append(Long(-operand_stack.take_first().get<Long>()));
                break;

            case Opcode::dup:
                operand_stack.append(operand_stack.first());
                break;

            case Opcode::pop:
                operand_stack.remove(0);
                break;

            case Opcode::getstatic:
            {
                auto value_index = code->code[m_program_counter + 1] << 8 | code->code[m_program_counter + 2];
                auto& value = class_file.constant_pool()[value_index - 1].get<ClassFile::FieldRef>();
                [[maybe_unused]] auto& class_of_field =
                    class_file.constant_pool()[value.class_index - 1].get<ClassFile::Class>();

                auto& field_name_and_type =
                    class_file.constant_pool()[value.name_and_type_index - 1].get<ClassFile::NameAndType>();
                auto& field_name =
                    class_file.constant_pool()[field_name_and_type.name_index - 1].get<ClassFile::Utf8>();

                // TODO: resolve and initialize class_of_field (don't assume it's in our class!)
                TRY(initialize_class(class_file));

                operand_stack.append(*m_static_data.find(class_file)->value.fields.get(field_name.value));

                m_program_counter += 2;
                break;
            }

            case Opcode::putstatic:
            {
                auto value_index = code->code[m_program_counter + 1] << 8 | code->code[m_program_counter + 2];
                auto& value = class_file.constant_pool()[value_index - 1].get<ClassFile::FieldRef>();
                [[maybe_unused]] auto& class_of_field =
                    class_file.constant_pool()[value.class_index - 1].get<ClassFile::Class>();

                auto& field_name_and_type =
                    class_file.constant_pool()[value.name_and_type_index - 1].get<ClassFile::NameAndType>();
                auto& field_name =
                    class_file.constant_pool()[field_name_and_type.name_index - 1].get<ClassFile::Utf8>();

                // TODO: resolve and initialize class_of_field (don't assume it's in our class!)
                TRY(initialize_class(class_file));

                m_static_data.find(class_file)->value.fields.set(field_name.value, operand_stack.take_first());

                m_program_counter += 2;
                break;
            }

            default:
                return Error::from_string_literal(String::formatted("Unhandled opcode {}", *opcode_names.get(opcode)));
        }

        m_program_counter++;
    }

    return Error::from_string_literal("Method code execution reached the end without returning");
}
}