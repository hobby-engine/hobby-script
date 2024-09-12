cmake_minimum_required(VERSION 3.29)
project(hobby)

if(LINUX)
  add_compile_definitions(hb_linux)
  add_compile_definitions(hb_posix)
elseif(APPLE)
  add_compile_definitions(hb_apple)
  add_compile_definitions(hb_posix)
elseif(WIN32)
  add_compile_definitions(hb_win)
else()
  message(FATAL_ERROR "Platform not supported.")
endif()

set(SRC
  # Hobby
  src/main.c

  # Hobbyscript
  src/hbs.c
  src/api.c
  src/arr.c
  src/chunk.c
  src/compiler.c
  src/debug.c
  src/lexer.c
  src/lib_arr.c
  src/lib_core.c
  src/lib_io.c
  src/lib_math.c
  src/lib_rand.c
  src/lib_sys.c
  src/map.c
  src/mem.c
  src/obj.c
  src/state.c
  src/tostr.c
  src/val.c
  src/vm.c
)

add_compile_options(-Wall -Wextra -Wno-unused-parameter)

if(CONFIG STREQUAL "release")
  add_compile_options(-O3)
  add_compile_definitions(hb_release)
else()
  add_compile_options(-O2 -g)
  # I don't know if asan is on Mac too, but I don't really wanna find out
  if(LINUX)
    add_compile_options(-fsanitize=address -fsanitize=undefined)
    add_link_options(-fsanitize=address -fsanitize=undefined)
  endif()
  add_compile_definitions(hb_debug)
endif()

add_compile_definitions(GLFW_INCLUDE_NONE)

add_executable(hobby ${SRC})
target_include_directories(hobby PRIVATE src)
target_link_libraries(hobby PRIVATE m)
