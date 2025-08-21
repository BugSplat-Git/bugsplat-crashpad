cmake_minimum_required(VERSION 3.10)

# Project name
project(MyCMakeCrasher VERSION 1.0)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set output directories to mimic multi-config generators for single-config generators
# This ensures Linux and macOS builds create Debug/Release folders like Windows
get_property(IS_MULTICONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT IS_MULTICONFIG)
    # For single-config generators, set the output directories to include config name
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
    endif()
    
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE})
endif()

# Ensure debug symbols are generated
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  # Debug symbol flags for all platforms
  if(NOT MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
  endif()
  
  # Platform-specific debug symbol settings
  if(MSVC)
    # Visual Studio generates PDB files by default in Debug mode
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  endif()
endif()

# Extract configuration values from main.h for symbol uploads
file(STRINGS "main.h" MAIN_H_CONTENTS)
foreach(LINE ${MAIN_H_CONTENTS})
  if(LINE MATCHES "^#define BUGSPLAT_DATABASE \"(.*)\"")
    set(BUGSPLAT_DATABASE "${CMAKE_MATCH_1}")
  elseif(LINE MATCHES "^#define BUGSPLAT_APP_NAME \"(.*)\"")
    set(BUGSPLAT_APP_NAME "${CMAKE_MATCH_1}")
  elseif(LINE MATCHES "^#define BUGSPLAT_APP_VERSION \"(.*)\"")
    set(BUGSPLAT_APP_VERSION "${CMAKE_MATCH_1}")
  endif()
endforeach()

# For sensitive values, check environment first, then allow manual override
if(DEFINED ENV{BUGSPLAT_CLIENT_ID})
  set(BUGSPLAT_CLIENT_ID $ENV{BUGSPLAT_CLIENT_ID} CACHE STRING "BugSplat client ID")
else()
  set(BUGSPLAT_CLIENT_ID "" CACHE STRING "BugSplat client ID")
endif()

if(DEFINED ENV{BUGSPLAT_CLIENT_SECRET})
  set(BUGSPLAT_CLIENT_SECRET $ENV{BUGSPLAT_CLIENT_SECRET} CACHE STRING "BugSplat client secret")
else()
  set(BUGSPLAT_CLIENT_SECRET "" CACHE STRING "BugSplat client secret")
endif()

# Set Crashpad paths
set(CRASHPAD_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/third_party/crashpad)

# Set platform-specific build output directory for Crashpad
if(WIN32)
    set(CRASHPAD_OUT_DIR ${CRASHPAD_ROOT}/out/win-debug)
elseif(APPLE)
    set(CRASHPAD_OUT_DIR ${CRASHPAD_ROOT}/out/macos-debug)
elseif(UNIX)
    set(CRASHPAD_OUT_DIR ${CRASHPAD_ROOT}/out/linux-debug)
endif()

# Add Crashpad include directories
include_directories(
    ${CRASHPAD_ROOT}
    ${CRASHPAD_ROOT}/third_party/mini_chromium/mini_chromium
)

# Set up platform-specific libraries
if(WIN32)
    set(CRASHPAD_LIBRARIES
        ${CRASHPAD_OUT_DIR}/obj/client/client.lib
        ${CRASHPAD_OUT_DIR}/obj/client/common.lib
        ${CRASHPAD_OUT_DIR}/obj/util/util.lib
        ${CRASHPAD_OUT_DIR}/obj/third_party/mini_chromium/mini_chromium/base/base.lib
    )
    set(CRASHPAD_HANDLER ${CRASHPAD_OUT_DIR}/crashpad_handler.exe)
elseif(APPLE)
    set(CRASHPAD_LIBRARIES
        ${CRASHPAD_OUT_DIR}/obj/client/libclient.a
        ${CRASHPAD_OUT_DIR}/obj/client/libcommon.a
        ${CRASHPAD_OUT_DIR}/obj/util/libutil.a
        ${CRASHPAD_OUT_DIR}/obj/util/libmig_output.a
        ${CRASHPAD_OUT_DIR}/obj/third_party/mini_chromium/mini_chromium/base/libbase.a
    )
    set(CRASHPAD_HANDLER ${CRASHPAD_OUT_DIR}/crashpad_handler)
else() # Linux
    set(CRASHPAD_LIBRARIES
        ${CRASHPAD_OUT_DIR}/obj/client/libclient.a
        ${CRASHPAD_OUT_DIR}/obj/client/libcommon.a
        ${CRASHPAD_OUT_DIR}/obj/util/libutil.a
        ${CRASHPAD_OUT_DIR}/obj/third_party/mini_chromium/mini_chromium/base/libbase.a
    )
    set(CRASHPAD_HANDLER ${CRASHPAD_OUT_DIR}/crashpad_handler)
endif()

# Build the crash library
if(WIN32)
    add_library(crash SHARED crash.cpp crash.h)
    set_target_properties(crash PROPERTIES OUTPUT_NAME "crash")
elseif(APPLE)
    add_library(crash SHARED crash.cpp crash.h)
    set_target_properties(crash PROPERTIES OUTPUT_NAME "crash")
else() # Linux
    add_library(crash SHARED crash.cpp crash.h)
    set_target_properties(crash PROPERTIES 
        OUTPUT_NAME "crash" 
        VERSION 2.0.0 
        SOVERSION 2)
endif()



# Add executable with platform-specific source files
if(WIN32)
    add_executable(MyCMakeCrasher main.cpp paths.cpp windows.cpp)
else()
    add_executable(MyCMakeCrasher main.cpp paths.cpp)
endif()

# Platform-specific libraries and link order
if(APPLE)
    target_link_libraries(MyCMakeCrasher
        "-framework Foundation"
        "-framework CoreFoundation"
        "-framework CoreGraphics"
        "-framework CoreText"
        "-framework Security"
        "-framework IOKit"
        "-framework CoreServices"
        "-framework SystemConfiguration"
        bsm
        ${CRASHPAD_LIBRARIES}
    )
elseif(UNIX AND NOT APPLE)
    target_link_libraries(MyCMakeCrasher
        ${CRASHPAD_LIBRARIES}
        pthread
        dl
        stdc++fs
    )
else()
    target_link_libraries(MyCMakeCrasher ${CRASHPAD_LIBRARIES})
endif()

# Set the output config directory variable
if(IS_MULTICONFIG)
    set(OUTPUT_CONFIG_DIR $<CONFIG>)
else()
    set(OUTPUT_CONFIG_DIR ${CMAKE_BUILD_TYPE})
endif()

# Copy crashpad_handler to the build directory
add_custom_command(TARGET MyCMakeCrasher POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CRASHPAD_HANDLER}
    ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_CONFIG_DIR}/crashpad_handler${CMAKE_EXECUTABLE_SUFFIX}
)

# Copy Crashpad's WER callback DLL to build directory (Windows only)
if(WIN32)
    add_custom_command(TARGET MyCMakeCrasher POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CRASHPAD_OUT_DIR}/crashpad_wer.dll
        ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_CONFIG_DIR}/crashpad_wer.dll
    )
endif()

# Create attachment.txt in the build directory
add_custom_command(TARGET MyCMakeCrasher POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "BugSplat rocks!" > ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_CONFIG_DIR}/attachment.txt
)

# Copy the crash library to the build directory
if(WIN32)
    add_custom_command(TARGET crash POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:crash>
        ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_CONFIG_DIR}/$<TARGET_FILE_NAME:crash>
    )
elseif(APPLE)
    add_custom_command(TARGET crash POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:crash>
        ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_CONFIG_DIR}/libcrash.dylib
    )
else() # Linux
    add_custom_command(TARGET crash POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:crash>
        ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_CONFIG_DIR}/libcrash.so.2
    )
endif()

# Add platform-specific debug symbol extraction for Debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  if(APPLE)
    # Add dsymutil post-build step for macOS
    add_custom_command(TARGET MyCMakeCrasher POST_BUILD
      COMMAND dsymutil $<TARGET_FILE:MyCMakeCrasher>
      COMMENT "Generating dSYM bundle for MyCMakeCrasher"
    )
    
    add_custom_command(TARGET crash POST_BUILD
      COMMAND dsymutil $<TARGET_FILE:crash>
      COMMENT "Generating dSYM bundle for crash library"
    )
  elseif(UNIX AND NOT APPLE)
    # Add objcopy debug symbol extraction for Linux
    add_custom_command(TARGET MyCMakeCrasher POST_BUILD
      COMMAND objcopy --only-keep-debug $<TARGET_FILE:MyCMakeCrasher> $<TARGET_FILE:MyCMakeCrasher>.debug
      COMMAND objcopy --strip-debug $<TARGET_FILE:MyCMakeCrasher>
      COMMAND objcopy --add-gnu-debuglink=$<TARGET_FILE:MyCMakeCrasher>.debug $<TARGET_FILE:MyCMakeCrasher>
      COMMENT "Extracting debug symbols for MyCMakeCrasher"
    )
    
    add_custom_command(TARGET crash POST_BUILD
      COMMAND objcopy --only-keep-debug $<TARGET_FILE:crash> $<TARGET_FILE:crash>.debug
      COMMAND objcopy --strip-debug $<TARGET_FILE:crash>
      COMMAND objcopy --add-gnu-debuglink=$<TARGET_FILE:crash>.debug $<TARGET_FILE:crash>
      COMMENT "Extracting debug symbols for crash library"
    )
  endif()
endif()

# Install targets
install(TARGETS MyCMakeCrasher DESTINATION bin)
install(TARGETS crash DESTINATION bin)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_CONFIG_DIR}/crashpad_handler${CMAKE_EXECUTABLE_SUFFIX} DESTINATION bin)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_CONFIG_DIR}/attachment.txt DESTINATION bin)
if(WIN32)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_CONFIG_DIR}/crashpad_wer.dll DESTINATION bin)
endif()

# Symbol upload target - updated to use platform-specific scripts
if(WIN32)
  add_custom_target(upload_symbols ALL
    COMMAND powershell -ExecutionPolicy Bypass -File 
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/upload_symbols.ps1
    DEPENDS MyCMakeCrasher crash
    COMMENT "Uploading symbols to BugSplat"
  )
elseif(APPLE)
  add_custom_target(upload_symbols ALL
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/upload_symbols.sh
    DEPENDS MyCMakeCrasher crash
    COMMENT "Uploading symbols to BugSplat"
  )
else() # Linux
  add_custom_target(upload_symbols ALL
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/upload_symbols.sh
    DEPENDS MyCMakeCrasher crash
    COMMENT "Uploading symbols to BugSplat"
  )
endif() 