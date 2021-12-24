#pragma once

#include <AK/Vector.h>
#include <LibJava/ClassFile.h>
#include <LibJava/Types.h>

namespace Java
{
class VM
{
public:
    VM(ClassFile& class_file);

    template<typename... Args>
    Value call(const ClassFile::MethodInfo& method, Args... args)
    {
        if constexpr (sizeof...(Args) == 0)
            return call(method);

        Vector<Value> arguments;
        arguments.ensure_capacity(sizeof...(Args));
        (arguments.append(move(args)), ...);

        return call(method, arguments.span());
    }

    Value call(const ClassFile::MethodInfo&, Span<Value> arguments = {});

private:
    struct Frame
    {
        // TODO: dont have empty
        Vector<Value> locals;
    };

    struct StaticData
    {
        HashMap<String, Value> fields;
    };

    // 2.5.1
    // If that method is not native, the pc register contains the address of the Java Virtual Machine instruction
    // currently being executed.
    // If the method currently being executed by the thread is native, the value of the Java
    // Virtual Machine's pc register is undefined.
    u16 m_program_counter{};
    ClassFile& m_class_file;
    Vector<Frame> m_stack;
    HashMap<ClassFile, StaticData> m_static_data;

    void initialize_class(ClassFile&);
    void initialize_class_if_needed(ClassFile&);

    template<typename T>
    ALWAYS_INLINE static constexpr T add(Value&& a, Value&& b)
    {
        return a.get<T>() + b.get<T>();
    }

    template<typename T>
    ALWAYS_INLINE static constexpr T sub(Value&& a, Value&& b)
    {
        return a.get<T>() - b.get<T>();
    }

    template<typename T>
    ALWAYS_INLINE static constexpr T mul(Value&& a, Value&& b)
    {
        return a.get<T>() * b.get<T>();
    }

    template<typename T>
    ALWAYS_INLINE static constexpr T div(Value&& a, Value&& b)
    {
        return a.get<T>() / b.get<T>();
    }

    template<typename T>
    ALWAYS_INLINE static constexpr T bitwise_inclusive_or(Value&& a, Value&& b)
    {
        return a.get<T>() | b.get<T>();
    }

    template<typename T>
    ALWAYS_INLINE static constexpr T bitwise_and(Value&& a, Value&& b)
    {
        return a.get<T>() & b.get<T>();
    }

    template<typename T>
    ALWAYS_INLINE static constexpr T bitwise_exclusive_or(Value&& a, Value&& b)
    {
        return a.get<T>() ^ b.get<T>();
    }

    template<typename T>
    ALWAYS_INLINE bool if_equal(int a, int b, u16 success_branch)
    {
        if (a == b)
        {
            m_program_counter += success_branch;
            return true;
        }

        return false;
    }

    template<typename T>
    ALWAYS_INLINE bool if_not_equal(int a, int b, u16 success_branch)
    {
        if (a != b)
        {
            m_program_counter += success_branch;
            return true;
        }

        return false;
    }

    template<typename T>
    ALWAYS_INLINE bool if_less_than(int a, int b, u16 success_branch)
    {
        if (a < b)
        {
            m_program_counter += success_branch;
            return true;
        }

        return false;
    }

    template<typename T>
    ALWAYS_INLINE bool if_greater_than_or_equal_to(int a, int b, u16 success_branch)
    {
        if (a >= b)
        {
            m_program_counter += success_branch;
            return true;
        }

        return false;
    }

    template<typename T>
    ALWAYS_INLINE bool if_greater_than(int a, int b, u16 success_branch)
    {
        if (a > b)
        {
            m_program_counter += success_branch;
            return true;
        }

        return false;
    }

    template<typename T>
    ALWAYS_INLINE bool if_less_than_or_equal_to(int a, int b, u16 success_branch)
    {
        if (a <= b)
        {
            m_program_counter += success_branch;
            return true;
        }

        return false;
    }

    template<typename T>
    ALWAYS_INLINE bool if_equal(Value&& a, Value&& b, u16 success_branch)
    {
        return if_equal<T>(a.get<Integer>().value(), b.get<Integer>().value(), success_branch);
    }

    template<typename T>
    ALWAYS_INLINE bool if_not_equal(Value&& a, Value&& b, u16 success_branch)
    {
        return if_not_equal<T>(a.get<Integer>().value(), b.get<Integer>().value(), success_branch);
    }

    template<typename T>
    ALWAYS_INLINE bool if_less_than(Value&& a, Value&& b, u16 success_branch)
    {
        return if_less_than<T>(a.get<Integer>().value(), b.get<Integer>().value(), success_branch);
    }

    template<typename T>
    ALWAYS_INLINE bool if_greater_than_or_equal_to(Value&& a, Value&& b, u16 success_branch)
    {
        return if_greater_than_or_equal_to<T>(a.get<Integer>().value(), b.get<Integer>().value(), success_branch);
    }

    template<typename T>
    ALWAYS_INLINE bool if_greater_than(Value&& a, Value&& b, u16 success_branch)
    {
        return if_greater_than<T>(a.get<Integer>().value(), b.get<Integer>().value(), success_branch);
    }

    template<typename T>
    ALWAYS_INLINE bool if_less_than_or_equal_to(Value&& a, Value&& b, u16 success_branch)
    {
        return if_less_than_or_equal_to<T>(a.get<Integer>().value(), b.get<Integer>().value(), success_branch);
    }

    template<typename T>
    ALWAYS_INLINE bool if_equal(Value&& a, int b, u16 success_branch)
    {
        return if_equal<T>(a.get<Integer>().value(), b, success_branch);
    }

    template<typename T>
    ALWAYS_INLINE bool if_not_equal(Value&& a, int b, u16 success_branch)
    {
        return if_not_equal<T>(a.get<Integer>().value(), b, success_branch);
    }

    template<typename T>
    ALWAYS_INLINE bool if_less_than(Value&& a, int b, u16 success_branch)
    {
        return if_less_than<T>(a.get<Integer>().value(), b, success_branch);
    }

    template<typename T>
    ALWAYS_INLINE bool if_greater_than_or_equal_to(Value&& a, int b, u16 success_branch)
    {
        return if_greater_than_or_equal_to<T>(a.get<Integer>().value(), b, success_branch);
    }

    template<typename T>
    ALWAYS_INLINE bool if_greater_than(Value&& a, int b, u16 success_branch)
    {
        return if_greater_than<T>(a.get<Integer>().value(), b, success_branch);
    }

    template<typename T>
    ALWAYS_INLINE bool if_less_than_or_equal_to(Value&& a, int b, u16 success_branch)
    {
        return if_less_than_or_equal_to<T>(a.get<Integer>().value(), b, success_branch);
    }
};
}