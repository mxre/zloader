project(tools C)
cmake_minimum_required(VERSION 3.21)

file(CREATE_LINK "../include/efi/pe.h" "${CMAKE_BINARY_DIR}/pe.h" SYMBOLIC)
file(CREATE_LINK "../include/efi/compiler.h" "${CMAKE_BINARY_DIR}/compiler.h" SYMBOLIC)

include_directories(${CMAKE_BINARY_DIR})

add_executable(pe_fixup pe_fixup.c)
target_compile_options(pe_fixup
  PRIVATE "-std=gnu2x"
)

add_executable(build_image build_image.c)
target_compile_options(build_image
  PRIVATE "-std=gnu2x"
)

add_custom_command(TARGET pe_fixup POST_BUILD
  BYPRODUCTS bundle_image.sh
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/bundle_image.sh" "bundle_image.sh"
)
