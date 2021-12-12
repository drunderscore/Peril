#pragma once

#include <AK/String.h>
#include <AK/Vector.h>
#include <LibJava/ClassFile.h>

namespace Java
{
class Disassembler
{
public:
    Disassembler(ClassFile&);

    ErrorOr<Vector<String>> disassemble(const ClassFile::MethodInfo&);

    bool numbered_instructions() const { return m_numbered_instructions; }

    void set_numbered_instructions(bool value) { m_numbered_instructions = value; }

private:
    ClassFile& m_class_file;
    bool m_numbered_instructions = false;
};
}