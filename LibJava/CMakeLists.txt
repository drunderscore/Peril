add_library(LibJava SHARED
        ClassFile.cpp
        Descriptor.cpp
        Disassembler.cpp
        VM.cpp
        )

target_include_directories(LibJava SYSTEM PRIVATE
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_BINARY_DIR}
        )

target_link_libraries(LibJava PRIVATE Lagom::Core)
