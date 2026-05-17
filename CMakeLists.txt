cmake_minimum_required(VERSION 3.16)
project(bang-lish C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---- Compiler flags ----
add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wformat=2
    -Wno-unused-parameter
    -Wno-unused-function
)
add_compile_definitions(_POSIX_C_SOURCE=200809L)

# ---- Sources: all modules under src/ ----
file(GLOB_RECURSE LIB_SOURCES
    "src/utils/*.c"
    "src/core/*.c"
    "src/lexer/*.c"
    "src/exec/*.c"
    "src/builtin/*.c"
    "src/info/*.c"
    "src/scripting/*.c"
)

# ---- Static library (all modules except main) ----
add_library(banglish_lib STATIC ${LIB_SOURCES})
target_include_directories(banglish_lib PUBLIC src)

# ---- readline (required) ----
find_library(READLINE_LIB
    NAMES readline
    REQUIRED)

find_path(READLINE_INC
    NAMES readline/readline.h
    REQUIRED)

find_library(NCURSES_LIB
    NAMES ncurses tinfo
    REQUIRED)

message(STATUS "readline: ${READLINE_LIB}")
message(STATUS "readline headers: ${READLINE_INC}")
message(STATUS "ncurses: ${NCURSES_LIB}")

target_compile_definitions(banglish_lib PUBLIC HAVE_READLINE=1)
target_include_directories(banglish_lib PUBLIC ${READLINE_INC})
target_link_libraries(banglish_lib PUBLIC ${READLINE_LIB} ${NCURSES_LIB})

# ---- Main executable ----
add_executable(banglish src/main.c)
target_include_directories(banglish PRIVATE src)
target_link_libraries(banglish PRIVATE banglish_lib)

# ---- Install ----
install(TARGETS banglish DESTINATION bin)
