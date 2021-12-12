#pragma once

#include <AK/DistinctNumeric.h>
#include <AK/HashMap.h>
#include <AK/String.h>

namespace Java
{
#define ENUMERATE_JAVA_OPCODES(M)                                                                                      \
    M(nop, "nop", 0x00)                                                                                                \
    M(aconst_null, "aconst_null", 0x01)                                                                                \
    M(iconst_m1, "iconst_m1", 0x02)                                                                                    \
    M(iconst_0, "iconst_0", 0x03)                                                                                      \
    M(iconst_1, "iconst_1", 0x04)                                                                                      \
    M(iconst_2, "iconst_2", 0x05)                                                                                      \
    M(iconst_3, "iconst_3", 0x06)                                                                                      \
    M(iconst_4, "iconst_4", 0x07)                                                                                      \
    M(iconst_5, "iconst_5", 0x08)                                                                                      \
    M(lconst_0, "lconst_0", 0x09)                                                                                      \
    M(lconst_1, "lconst_1", 0x0a)                                                                                      \
    M(fconst_0, "fconst_0", 0x0b)                                                                                      \
    M(fconst_1, "fconst_1", 0x0c)                                                                                      \
    M(fconst_2, "fconst_2", 0x0d)                                                                                      \
    M(dconst_0, "dconst_0", 0x0e)                                                                                      \
    M(dconst_1, "dconst_1", 0x0f)                                                                                      \
    M(bipush, "bipush", 0x10)                                                                                          \
    M(sipush, "sipush", 0x11)                                                                                          \
    M(ldc, "ldc", 0x12)                                                                                                \
    M(ldc_w, "ldc_w", 0x13)                                                                                            \
    M(ldc2_w, "ldc2_w", 0x14)                                                                                          \
    M(iload, "iload", 0x15)                                                                                            \
    M(lload, "lload", 0x16)                                                                                            \
    M(fload, "fload", 0x17)                                                                                            \
    M(dload, "dload", 0x18)                                                                                            \
    M(aload, "aload", 0x19)                                                                                            \
    M(iload_0, "iload_0", 0x1a)                                                                                        \
    M(iload_1, "iload_1", 0x1b)                                                                                        \
    M(iload_2, "iload_2", 0x1c)                                                                                        \
    M(iload_3, "iload_3", 0x1d)                                                                                        \
    M(lload_0, "lload_0", 0x1e)                                                                                        \
    M(lload_1, "lload_1", 0x1f)                                                                                        \
    M(lload_2, "lload_2", 0x20)                                                                                        \
    M(lload_3, "lload_3", 0x21)                                                                                        \
    M(fload_0, "fload_0", 0x22)                                                                                        \
    M(fload_1, "fload_1", 0x23)                                                                                        \
    M(fload_2, "fload_2", 0x24)                                                                                        \
    M(fload_3, "fload_3", 0x25)                                                                                        \
    M(dload_0, "dload_0", 0x26)                                                                                        \
    M(dload_1, "dload_1", 0x27)                                                                                        \
    M(dload_2, "dload_2", 0x28)                                                                                        \
    M(dload_3, "dload_3", 0x29)                                                                                        \
    M(aload_0, "aload_0", 0x2a)                                                                                        \
    M(aload_1, "aload_1", 0x2b)                                                                                        \
    M(aload_2, "aload_2", 0x2c)                                                                                        \
    M(aload_3, "aload_3", 0x2d)                                                                                        \
    M(iaload, "iaload", 0x2e)                                                                                          \
    M(laload, "laload", 0x2f)                                                                                          \
    M(faload, "faload", 0x30)                                                                                          \
    M(daload, "daload", 0x31)                                                                                          \
    M(aaload, "aaload", 0x32)                                                                                          \
    M(baload, "baload", 0x33)                                                                                          \
    M(caload, "caload", 0x34)                                                                                          \
    M(saload, "saload", 0x35)                                                                                          \
    M(istore, "istore", 0x36)                                                                                          \
    M(lstore, "lstore", 0x37)                                                                                          \
    M(fstore, "fstore", 0x38)                                                                                          \
    M(dstore, "dstore", 0x39)                                                                                          \
    M(astore, "astore", 0x3a)                                                                                          \
    M(istore_0, "istore_0", 0x3b)                                                                                      \
    M(istore_1, "istore_1", 0x3c)                                                                                      \
    M(istore_2, "istore_2", 0x3d)                                                                                      \
    M(istore_3, "istore_3", 0x3e)                                                                                      \
    M(lstore_0, "lstore_0", 0x3f)                                                                                      \
    M(lstore_1, "lstore_1", 0x40)                                                                                      \
    M(lstore_2, "lstore_2", 0x41)                                                                                      \
    M(lstore_3, "lstore_3", 0x42)                                                                                      \
    M(fstore_0, "fstore_0", 0x43)                                                                                      \
    M(fstore_1, "fstore_1", 0x44)                                                                                      \
    M(fstore_2, "fstore_2", 0x45)                                                                                      \
    M(fstore_3, "fstore_3", 0x46)                                                                                      \
    M(dstore_0, "dstore_0", 0x47)                                                                                      \
    M(dstore_1, "dstore_1", 0x48)                                                                                      \
    M(dstore_2, "dstore_2", 0x49)                                                                                      \
    M(dstore_3, "dstore_3", 0x4a)                                                                                      \
    M(astore_0, "astore_0", 0x4b)                                                                                      \
    M(astore_1, "astore_1", 0x4c)                                                                                      \
    M(astore_2, "astore_2", 0x4d)                                                                                      \
    M(astore_3, "astore_3", 0x4e)                                                                                      \
    M(iastore, "iastore", 0x4f)                                                                                        \
    M(lastore, "lastore", 0x50)                                                                                        \
    M(fastore, "fastore", 0x51)                                                                                        \
    M(dastore, "dastore", 0x52)                                                                                        \
    M(aastore, "aastore", 0x53)                                                                                        \
    M(bastore, "bastore", 0x54)                                                                                        \
    M(castore, "castore", 0x55)                                                                                        \
    M(sastore, "sastore", 0x56)                                                                                        \
    M(pop, "pop", 0x57)                                                                                                \
    M(pop2, "pop2", 0x58)                                                                                              \
    M(dup, "dup", 0x59)                                                                                                \
    M(dup_x1, "dup_x1", 0x5a)                                                                                          \
    M(dup_x2, "dup_x2", 0x5b)                                                                                          \
    M(dup2, "dup2", 0x5c)                                                                                              \
    M(dup2_x1, "dup2_x1", 0x5d)                                                                                        \
    M(dup2_x2, "dup2_x2", 0x5e)                                                                                        \
    M(swap, "swap", 0x5f)                                                                                              \
    M(iadd, "iadd", 0x60)                                                                                              \
    M(ladd, "ladd", 0x61)                                                                                              \
    M(fadd, "fadd", 0x62)                                                                                              \
    M(dadd, "dadd", 0x63)                                                                                              \
    M(isub, "isub", 0x64)                                                                                              \
    M(lsub, "lsub", 0x65)                                                                                              \
    M(fsub, "fsub", 0x66)                                                                                              \
    M(dsub, "dsub", 0x67)                                                                                              \
    M(imul, "imul", 0x68)                                                                                              \
    M(lmul, "lmul", 0x69)                                                                                              \
    M(fmul, "fmul", 0x6a)                                                                                              \
    M(dmul, "dmul", 0x6b)                                                                                              \
    M(idiv, "idiv", 0x6c)                                                                                              \
    M(ldiv, "ldiv", 0x6d)                                                                                              \
    M(fdiv, "fdiv", 0x6e)                                                                                              \
    M(ddiv, "ddiv", 0x6f)                                                                                              \
    M(irem, "irem", 0x70)                                                                                              \
    M(lrem, "lrem", 0x71)                                                                                              \
    M(frem, "frem", 0x72)                                                                                              \
    M(drem, "drem", 0x73)                                                                                              \
    M(ineg, "ineg", 0x74)                                                                                              \
    M(lneg, "lneg", 0x75)                                                                                              \
    M(fneg, "fneg", 0x76)                                                                                              \
    M(dneg, "dneg", 0x77)                                                                                              \
    M(ishl, "ishl", 0x78)                                                                                              \
    M(lshl, "lshl", 0x79)                                                                                              \
    M(ishr, "ishr", 0x7a)                                                                                              \
    M(lshr, "lshr", 0x7b)                                                                                              \
    M(iushr, "iushr", 0x7c)                                                                                            \
    M(lushr, "lushr", 0x7d)                                                                                            \
    M(iand, "iand", 0x7e)                                                                                              \
    M(land, "land", 0x7f)                                                                                              \
    M(ior, "ior", 0x80)                                                                                                \
    M(lor, "lor", 0x81)                                                                                                \
    M(ixor, "ixor", 0x82)                                                                                              \
    M(lxor, "lxor", 0x83)                                                                                              \
    M(iinc, "iinc", 0x84)                                                                                              \
    M(i2l, "i2l", 0x85)                                                                                                \
    M(i2f, "i2f", 0x86)                                                                                                \
    M(i2d, "i2d", 0x87)                                                                                                \
    M(l2i, "l2i", 0x88)                                                                                                \
    M(l2f, "l2f", 0x89)                                                                                                \
    M(l2d, "l2d", 0x8a)                                                                                                \
    M(f2i, "f2i", 0x8b)                                                                                                \
    M(f2l, "f2l", 0x8c)                                                                                                \
    M(f2d, "f2d", 0x8d)                                                                                                \
    M(d2i, "d2i", 0x8e)                                                                                                \
    M(d2l, "d2l", 0x8f)                                                                                                \
    M(d2f, "d2f", 0x90)                                                                                                \
    M(i2b, "i2b", 0x91)                                                                                                \
    M(i2c, "i2c", 0x92)                                                                                                \
    M(i2s, "i2s", 0x93)                                                                                                \
    M(lcmp, "lcmp", 0x94)                                                                                              \
    M(fcmpl, "fcmpl", 0x95)                                                                                            \
    M(fcmpg, "fcmpg", 0x96)                                                                                            \
    M(dcmpl, "dcmpl", 0x97)                                                                                            \
    M(dcmpg, "dcmpg", 0x98)                                                                                            \
    M(ifeq, "ifeq", 0x99)                                                                                              \
    M(ifne, "ifne", 0x9a)                                                                                              \
    M(iflt, "iflt", 0x9b)                                                                                              \
    M(ifge, "ifge", 0x9c)                                                                                              \
    M(ifgt, "ifgt", 0x9d)                                                                                              \
    M(ifle, "ifle", 0x9e)                                                                                              \
    M(if_icmpeq, "if_icmpeq", 0x9f)                                                                                    \
    M(if_icmpne, "if_icmpne", 0xa0)                                                                                    \
    M(if_icmplt, "if_icmplt", 0xa1)                                                                                    \
    M(if_icmpge, "if_icmpge", 0xa2)                                                                                    \
    M(if_icmpgt, "if_icmpgt", 0xa3)                                                                                    \
    M(if_icmple, "if_icmple", 0xa4)                                                                                    \
    M(if_acmpeq, "if_acmpeq", 0xa5)                                                                                    \
    M(if_acmpne, "if_acmpne", 0xa6)                                                                                    \
    M(goto_, "goto", 0xa7)                                                                                             \
    M(jsr, "jsr", 0xa8)                                                                                                \
    M(ret, "ret", 0xa9)                                                                                                \
    M(tableswitch, "tableswitch", 0xaa)                                                                                \
    M(lookupswitch, "lookupswitch", 0xab)                                                                              \
    M(ireturn, "ireturn", 0xac)                                                                                        \
    M(lreturn, "lreturn", 0xad)                                                                                        \
    M(freturn, "freturn", 0xae)                                                                                        \
    M(dreturn, "dreturn", 0xaf)                                                                                        \
    M(areturn, "areturn", 0xb0)                                                                                        \
    M(return_, "return", 0xb1)                                                                                         \
    M(getstatic, "getstatic", 0xb2)                                                                                    \
    M(putstatic, "putstatic", 0xb3)                                                                                    \
    M(getfield, "getfield", 0xb4)                                                                                      \
    M(putfield, "putfield", 0xb5)                                                                                      \
    M(invokevirtual, "invokevirtual", 0xb6)                                                                            \
    M(invokespecial, "invokespecial", 0xb7)                                                                            \
    M(invokestatic, "invokestatic", 0xb8)                                                                              \
    M(invokeinterface, "invokeinterface", 0xb9)                                                                        \
    M(invokedynamic, "invokedynamic", 0xba)                                                                            \
    M(new_, "new", 0xbb)                                                                                               \
    M(newarray, "newarray", 0xbc)                                                                                      \
    M(anewarray, "anewarray", 0xbd)                                                                                    \
    M(arraylength, "arraylength", 0xbe)                                                                                \
    M(athrow, "athrow", 0xbf)                                                                                          \
    M(checkcast, "checkcast", 0xc0)                                                                                    \
    M(instanceof, "instanceof", 0xc1)                                                                                  \
    M(monitorenter, "monitorenter", 0xc2)                                                                              \
    M(monitorexit, "monitorexit", 0xc3)                                                                                \
    M(wide, "wide", 0xc4)                                                                                              \
    M(multianewarray, "multianewarray", 0xc5)                                                                          \
    M(ifnull, "ifnull", 0xc6)                                                                                          \
    M(ifnonnull, "ifnonnull", 0xc7)                                                                                    \
    M(goto_w, "goto_w", 0xc8)                                                                                          \
    M(jsr_w, "jsr_w", 0xc9)                                                                                            \
    M(breakpoint, "breakpoint", 0xca)                                                                                  \
    M(impdep1, "impdep1", 0xfe)                                                                                        \
    M(impdep2, "impdep2", 0xff)

#define M(name, name_string, value) name = value,
enum class Opcode : u8
{
    ENUMERATE_JAVA_OPCODES(M)
};
#undef M

#define M(name, name_string, value) {Opcode::name, name_string},
static HashMap<Opcode, String> opcode_names{ENUMERATE_JAVA_OPCODES(M)};
#undef M

}