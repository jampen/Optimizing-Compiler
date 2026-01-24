cmake_minimum_required (VERSION 3.10)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("cyrexc")

# Add source to this project's executable.
add_executable (cyrexc
	"cyrex/frontend/token.cpp"
	"cyrex/frontend/parser.cpp"
	"cyrex/backend/irgen.cpp"
	"cyrex/frontend/semantics.cpp"

	"cyrex/backend/x64-allocator.cpp"
	"cyrex/backend/x64-optimizer.cpp"
	"cyrex/backend/x64.cpp"

	"cyrex/main.cpp")

include_directories(cyrexc PRIVATE cyrex/)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET cyrexc PROPERTY CXX_STANDARD 23)
endif()


# Argparse
if (EXISTS "${CMAKE_SOURCE_DIR}/argparse/CMakeLists.txt")
    add_subdirectory(argparse)
    target_link_libraries(cyrexc PRIVATE argparse)  
else()
    message(FATAL_ERROR "argparse submodule not found. Did you run git submodule update --init?")
endif()

# TODO: Add tests and install targets if needed.
