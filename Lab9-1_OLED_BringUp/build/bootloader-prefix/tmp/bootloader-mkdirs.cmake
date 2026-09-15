# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/Users/tanwat/.espressif/v6.0.2/esp-idf/components/bootloader/subproject")
  file(MAKE_DIRECTORY "/Users/tanwat/.espressif/v6.0.2/esp-idf/components/bootloader/subproject")
endif()
file(MAKE_DIRECTORY
  "/Users/tanwat/ IoT-App-2569/091-Week-09-SPI-OLED-and-Kestrel-UI/Lab9-1_OLED_BringUp/build/bootloader"
  "/Users/tanwat/ IoT-App-2569/091-Week-09-SPI-OLED-and-Kestrel-UI/Lab9-1_OLED_BringUp/build/bootloader-prefix"
  "/Users/tanwat/ IoT-App-2569/091-Week-09-SPI-OLED-and-Kestrel-UI/Lab9-1_OLED_BringUp/build/bootloader-prefix/tmp"
  "/Users/tanwat/ IoT-App-2569/091-Week-09-SPI-OLED-and-Kestrel-UI/Lab9-1_OLED_BringUp/build/bootloader-prefix/src/bootloader-stamp"
  "/Users/tanwat/ IoT-App-2569/091-Week-09-SPI-OLED-and-Kestrel-UI/Lab9-1_OLED_BringUp/build/bootloader-prefix/src"
  "/Users/tanwat/ IoT-App-2569/091-Week-09-SPI-OLED-and-Kestrel-UI/Lab9-1_OLED_BringUp/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/tanwat/ IoT-App-2569/091-Week-09-SPI-OLED-and-Kestrel-UI/Lab9-1_OLED_BringUp/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/tanwat/ IoT-App-2569/091-Week-09-SPI-OLED-and-Kestrel-UI/Lab9-1_OLED_BringUp/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
