#include <AK/MemoryStream.h>
#include <LibCore/ArgsParser.h>
#include <LibCore/File.h>
#include <LibJava/ClassFile.h>
#include <LibJava/Descriptor.h>
#include <LibJava/VM.h>
#include <LibMain/Main.h>

ErrorOr<int> serenity_main(Main::Arguments arguments)
{
    Core::ArgsParser args_parser;

    String class_file_path;
    String method_to_call;
    args_parser.add_positional_argument(class_file_path, "Path to the class file to execute", "class-file");
    args_parser.add_positional_argument(method_to_call, "Name of the method to call", "method-name");

    args_parser.parse(arguments);

    auto class_file_file = TRY(Core::File::open(class_file_path, Core::OpenMode::ReadOnly));
    auto class_file_contents = class_file_file->read_all();
    InputMemoryStream class_file_stream(class_file_contents);

    auto class_file = TRY(Java::ClassFile::try_parse(class_file_stream));

    Java::VM vm;

    for (auto& method : class_file.methods())
    {
        auto& name = class_file.constant_pool()[method.name_index - 1].get<Java::ClassFile::Utf8>();

        if (name.value == method_to_call)
        {
            if (!Java::has_flag(method.access_flags, Java::ClassFile::MethodInfo::AccessFlags::Public |
                                                         Java::ClassFile::MethodInfo::AccessFlags::Static))
                return Error::from_string_literal("Method to execute must be public and static");

            auto descriptor_string =
                class_file.constant_pool()[method.descriptor_index - 1].get<Java::ClassFile::Utf8>();

            auto descriptor = TRY(Java::MethodDescriptor::try_parse(descriptor_string.value));

            if (descriptor.parameters().size() != 0)
                return Error::from_string_literal("Method to execute must not take any parameters");

            auto return_value = vm.call(class_file, method);

            // FIXME: is there no general integral type to string?
            outln("Return: {}",
                  return_value.visit([](Java::Byte& value) { return String::formatted("{}", value.value()); },
                                     [](Java::Short& value) { return String::formatted("{}", value.value()); },
                                     [](Java::Integer& value) { return String::formatted("{}", value.value()); },
                                     [](Java::Long& value) { return String::formatted("{}", value.value()); },
                                     [](Java::Char& value) { return String::formatted("{}", value.value()); },
                                     [](Java::Float& value) { return String::formatted("{}", value.value()); },
                                     [](Java::Double& value) { return String::formatted("{}", value.value()); }));
            return 0;
        }
    }

    return Error::from_string_literal("Could not find the method to execute");
}