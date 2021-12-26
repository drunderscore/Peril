add_library(Java SHARED
        ClassFile.cpp
        Descriptor.cpp
        Disassembler.cpp
        VM.cpp
        )

target_include_directories(Java SYSTEM PRIVATE
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_BINARY_DIR}
        )

target_link_libraries(Java PRIVATE Lagom::Core)
