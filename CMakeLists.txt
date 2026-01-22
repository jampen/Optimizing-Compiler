cmake_minimum_required (VERSION 3.10)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("cyrex-compiler")

# Add source to this project's executable.
add_executable (cyrex-compiler
	cyrex/token.cpp
	cyrex/parser.cpp
	cyrex/irgen.cpp
	cyrex/semantics.cpp
	cyrex/x64-allocator.cpp
	cyrex/x64-optimizer.cpp
	cyrex/x64.cpp
	cyrex/main.cpp
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET cyrex-compiler PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
