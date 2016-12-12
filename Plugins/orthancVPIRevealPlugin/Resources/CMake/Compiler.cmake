# This file sets all the compiler-related flags

if (CMAKE_CROSSCOMPILING)
  # Cross-compilation necessarily implies standalone and static build
  SET(STATIC_BUILD ON)
  SET(STANDALONE_BUILD ON)
endif()

if (CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-long-long -Wno-implicit-function-declaration")  
  # --std=c99 makes libcurl not to compile
  # -pedantic gives a lot of warnings on OpenSSL 
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -Wno-variadic-macros")

  if (CMAKE_CROSSCOMPILING)
    # http://stackoverflow.com/a/3543845/881731
    set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -O coff -I<CMAKE_CURRENT_SOURCE_DIR> <SOURCE> <OBJECT>")
  endif()

elseif (MSVC)
  # Use static runtime under Visual Studio
  # http://www.cmake.org/Wiki/CMake_FAQ#Dynamic_Replace
  # http://stackoverflow.com/a/6510446
  foreach(flag_var
    CMAKE_C_FLAGS_DEBUG
    CMAKE_CXX_FLAGS_DEBUG
    CMAKE_C_FLAGS_RELEASE 
    CMAKE_CXX_FLAGS_RELEASE
    CMAKE_C_FLAGS_MINSIZEREL 
    CMAKE_CXX_FLAGS_MINSIZEREL 
    CMAKE_C_FLAGS_RELWITHDEBINFO 
    CMAKE_CXX_FLAGS_RELWITHDEBINFO) 
    string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
    string(REGEX REPLACE "/MDd" "/MTd" ${flag_var} "${${flag_var}}")
  endforeach(flag_var)

  # Add /Zm256 compiler option to Visual Studio to fix PCH errors
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zm256")

  add_definitions(
    -D_CRT_SECURE_NO_WARNINGS=1
    -D_CRT_SECURE_NO_DEPRECATE=1
    )
  include_directories(${ORTHANC_ROOT}/Resources/ThirdParty/VisualStudio)
  link_libraries(netapi32)
endif()


if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR
    ${CMAKE_SYSTEM_NAME} STREQUAL "kFreeBSD" OR
    ${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--no-undefined")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")

  if (NOT DEFINED ENABLE_PLUGINS_VERSION_SCRIPT OR 
      ENABLE_PLUGINS_VERSION_SCRIPT)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--version-script=${ORTHANC_ROOT}/Plugins/Samples/Common/VersionScript.map")
  endif()

  # Remove the "-rdynamic" option
  # http://www.mail-archive.com/cmake@cmake.org/msg08837.html
  set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")
  link_libraries(uuid pthread rt)

  if (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--as-needed")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--as-needed")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--as-needed")
    add_definitions(
      -D_LARGEFILE64_SOURCE=1 
      -D_FILE_OFFSET_BITS=64
      )
    link_libraries(dl)
  endif()

  CHECK_INCLUDE_FILES(uuid/uuid.h HAVE_UUID_H)
  if (NOT HAVE_UUID_H)
    message(FATAL_ERROR "Please install the uuid-dev package")
  endif()

elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  if (MSVC)
    message("MSVC compiler version = " ${MSVC_VERSION} "\n")
    # Starting Visual Studio 2013 (version 1800), it is not possible
    # to target Windows XP anymore
    if (MSVC_VERSION LESS 1800)
      add_definitions(
        -DWINVER=0x0501
        -D_WIN32_WINNT=0x0501
        )
    endif()
  else()
    add_definitions(
      -DWINVER=0x0501
      -D_WIN32_WINNT=0x0501
      )
  endif()

  add_definitions(
    -D_CRT_SECURE_NO_WARNINGS=1
    )
  link_libraries(rpcrt4 ws2_32)

  if (CMAKE_COMPILER_IS_GNUCXX)
    # Some additional C/C++ compiler flags for MinGW
    SET(MINGW_NO_WARNINGS "-Wno-unused-function -Wno-unused-variable")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MINGW_NO_WARNINGS} -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MINGW_NO_WARNINGS}")

    # This is a patch for MinGW64
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--allow-multiple-definition -static-libgcc -static-libstdc++")
    SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--allow-multiple-definition -static-libgcc -static-libstdc++")

    CHECK_LIBRARY_EXISTS(winpthread pthread_create "" HAVE_WIN_PTHREAD)
    if (HAVE_WIN_PTHREAD)
      # This line is necessary to compile with recent versions of MinGW,
      # otherwise "libwinpthread-1.dll" is not statically linked.
      SET(CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_CXX_STANDARD_LIBRARIES} -Wl,-Bstatic -lstdc++ -lpthread -Wl,-Bdynamic")
      add_definitions(-DHAVE_WIN_PTHREAD=1)
    else()
      add_definitions(-DHAVE_WIN_PTHREAD=0)
    endif()
  endif()

elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
  SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -exported_symbols_list ${ORTHANC_ROOT}/Plugins/Samples/Common/ExportedSymbols.list")

  add_definitions(
    -D_XOPEN_SOURCE=1
    )
  link_libraries(iconv)

  CHECK_INCLUDE_FILES(uuid/uuid.h HAVE_UUID_H)
  if (NOT HAVE_UUID_H)
    message(FATAL_ERROR "Please install the uuid-dev package")
  endif()

endif()


if ("${CMAKE_SYSTEM_VERSION}" STREQUAL "LinuxStandardBase")
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --lsb-target-version=${LSB_TARGET_VERSION} -I${LSB_PATH}/include")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --lsb-target-version=${LSB_TARGET_VERSION} -nostdinc++ -I${LSB_PATH}/include -I${LSB_PATH}/include/c++ -I${LSB_PATH}/include/c++/backward -fpermissive")
  SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} --lsb-target-version=${LSB_TARGET_VERSION} -L${LSB_LIBPATH}")
endif()


if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
  # In FreeBSD, the "/usr/local/" folder contains the ports and need to be imported
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I/usr/local/include")
  SET(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -I/usr/local/include")
  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L/usr/local/lib")
  SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -L/usr/local/lib")
endif()


if (STATIC_BUILD)
  add_definitions(-DORTHANC_STATIC=1)
else()
  add_definitions(-DORTHANC_STATIC=0)
endif()
