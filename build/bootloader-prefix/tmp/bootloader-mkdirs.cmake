# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Espressif/frameworks/esp-idf-v4.4.2/components/bootloader/subproject"
  "E:/workspace/Scanner/build/bootloader"
  "E:/workspace/Scanner/build/bootloader-prefix"
  "E:/workspace/Scanner/build/bootloader-prefix/tmp"
  "E:/workspace/Scanner/build/bootloader-prefix/src/bootloader-stamp"
  "E:/workspace/Scanner/build/bootloader-prefix/src"
  "E:/workspace/Scanner/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "E:/workspace/Scanner/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
