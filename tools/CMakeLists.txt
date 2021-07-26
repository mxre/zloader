project(tools C)
cmake_minimum_required(VERSION 3.21)

set(SOURCES vma_fixer.c)

file(CREATE_LINK "../include/efi/pe.h" "${CMAKE_BINARY_DIR}/pe.h" SYMBOLIC)
file(CREATE_LINK "../include/efi/compiler.h" "${CMAKE_BINARY_DIR}/compiler.h" SYMBOLIC)

include_directories(${CMAKE_BINARY_DIR})

add_compile_options(-std=c2x)
add_executable(vma_fixer ${SOURCES})

add_custom_command(TARGET vma_fixer POST_BUILD
  BYPRODUCTS bundle_image.sh
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/bundle_image.sh" "bundle_image.sh" )
