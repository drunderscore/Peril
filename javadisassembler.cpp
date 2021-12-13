#include <AK/MemoryStream.h>
#include <LibCore/ArgsParser.h>
#include <LibCore/File.h>
#include <LibJava/ClassFile.h>
#include <LibJava/Descriptor.h>
#include <LibJava/Disassembler.h>
#include <LibMain/Main.h>

ErrorOr<int> serenity_main(Main::Arguments arguments)
{
    Core::ArgsParser args_parser;
    String class_file_path;
    bool numbered_instructions;

    args_parser.add_positional_argument(class_file_path, "Path to a class file", "class-file");
    args_parser.add_option(numbered_instructions, "Prefix instructions with their code index", "numbered-instructions",
                           'n');
    args_parser.parse(arguments);

    auto file = TRY(Core::File::open(class_file_path, Core::OpenMode::ReadOnly));
    // Don't use Core::InputFileStream, it doesn't work that well
    auto file_contents = file->read_all();
    InputMemoryStream file_contents_stream(file_contents);

    auto class_file = TRY(Java::ClassFile::try_parse(file_contents_stream));
    Java::Disassembler disassembler(class_file);
    disassembler.set_numbered_instructions(numbered_instructions);

    for (auto& method : class_file.methods())
    {
        auto& name = class_file.constant_pool()[method.name_index - 1].get<Java::ClassFile::Utf8>();
        auto& descriptor = class_file.constant_pool()[method.descriptor_index - 1].get<Java::ClassFile::Utf8>();

        auto parsed_descriptor = TRY(Java::MethodDescriptor::try_parse(descriptor.value));

        auto instructions = disassembler.disassemble(method);
        if (instructions.is_error())
        {
            warnln("Could not disassemble {} ({}): {}", name.value, parsed_descriptor, instructions.error());
        }
        else
        {
            StringBuilder method_builder;

            // The static constructor
            if (name.value == "<clinit>"sv)
            {
                auto& this_class_name =
                    class_file.constant_pool()[class_file.this_class().name_index - 1].get<Java::ClassFile::Utf8>();

                method_builder.appendff("static"sv, name.value);
            }
            else
            {
                if (Java::has_flag(method.access_flags, Java::ClassFile::MethodInfo::AccessFlags::Private))
                    method_builder.append("private "sv);
                else if (Java::has_flag(method.access_flags, Java::ClassFile::MethodInfo::AccessFlags::Public))
                    method_builder.append("public "sv);
                else if (Java::has_flag(method.access_flags, Java::ClassFile::MethodInfo::AccessFlags::Protected))
                    method_builder.append("protected "sv);

                if (Java::has_flag(method.access_flags, Java::ClassFile::MethodInfo::AccessFlags::Static))
                    method_builder.append("static "sv);

                if (Java::has_flag(method.access_flags, Java::ClassFile::MethodInfo::AccessFlags::Abstract))
                    method_builder.append("abstract "sv);

                if (Java::has_flag(method.access_flags, Java::ClassFile::MethodInfo::AccessFlags::Abstract))
                    method_builder.append("synchronized "sv);

                if (Java::has_flag(method.access_flags, Java::ClassFile::MethodInfo::AccessFlags::Final))
                    method_builder.append("final "sv);

                if (Java::has_flag(method.access_flags, Java::ClassFile::MethodInfo::AccessFlags::Native))
                    method_builder.append("native "sv);

                if (Java::has_flag(method.access_flags, Java::ClassFile::MethodInfo::AccessFlags::Strict))
                    method_builder.append("strictfp "sv);

                if (name.value != "<init>"sv)
                {
                    method_builder.appendff("{} {}"sv, parsed_descriptor.return_type_to_string(), name.value);
                }
                else
                {
                    auto& this_class_name =
                        class_file.constant_pool()[class_file.this_class().name_index - 1].get<Java::ClassFile::Utf8>();
                    method_builder.appendff("{}"sv, this_class_name.value);
                }

                method_builder.appendff("({})"sv, parsed_descriptor.parameters_to_string());
            }

            outln("{} {{", method_builder.to_string());
            for (auto& instruction : instructions.value())
                outln("\t{}", instruction);
            outln("}}");
            outln();
        }
    }

    return 0;
}