project(tools C)
cmake_minimum_required(VERSION 3.21)

set(SOURCES vma_fixer.c)

file(CREATE_LINK "../include/efi/pe.h" "pe.h" SYMBOLIC)
file(CREATE_LINK "../include/efi/compiler.h" "compiler.h" SYMBOLIC)

include_directories(${CMAKE_BINARY_DIR})

add_executable(vma_fixer ${SOURCES})

add_custom_command(TARGET vma_fixer POST_BUILD
  BYPRODUCTS bundle_image.sh
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/bundle_image.sh" "bundle_image.sh" )
