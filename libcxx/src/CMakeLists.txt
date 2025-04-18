set(LIBCXX_LIB_CMAKEFILES_DIR "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}"  PARENT_SCOPE)

# Get sources
set(LIBCXX_SOURCES
  algorithm.cpp
  any.cpp
  atomic.cpp
  barrier.cpp
  bind.cpp
  charconv.cpp
  chrono.cpp
  condition_variable.cpp
  condition_variable_destructor.cpp
  debug.cpp
  exception.cpp
  functional.cpp
  future.cpp
  hash.cpp
  include/apple_availability.h
  include/atomic_support.h
  include/config_elast.h
  include/refstring.h
  ios.cpp
  iostream.cpp
  locale.cpp
  memory.cpp
  mutex.cpp
  mutex_destructor.cpp
  new.cpp
  optional.cpp
  random.cpp
  regex.cpp
  shared_mutex.cpp
  stdexcept.cpp
  string.cpp
  strstream.cpp
  support/runtime/exception_fallback.ipp
  support/runtime/exception_glibcxx.ipp
  support/runtime/exception_libcxxabi.ipp
  support/runtime/exception_libcxxrt.ipp
  support/runtime/exception_msvc.ipp
  support/runtime/exception_pointer_cxxabi.ipp
  support/runtime/exception_pointer_glibcxx.ipp
  support/runtime/exception_pointer_msvc.ipp
  support/runtime/exception_pointer_unimplemented.ipp
  support/runtime/new_handler_fallback.ipp
  support/runtime/stdexcept_default.ipp
  support/runtime/stdexcept_vcruntime.ipp
  system_error.cpp
  thread.cpp
  typeinfo.cpp
  utility.cpp
  valarray.cpp
  variant.cpp
  vector.cpp
  )

if(WIN32)
  list(APPEND LIBCXX_SOURCES
    support/win32/locale_win32.cpp
    support/win32/support.cpp
    support/win32/thread_win32.cpp
    )
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "SunOS")
  list(APPEND LIBCXX_SOURCES
    support/solaris/mbsnrtowcs.inc
    support/solaris/wcsnrtombs.inc
    support/solaris/xlocale.cpp
    )
endif()

if (LIBCXX_ENABLE_FILESYSTEM)
  list(APPEND LIBCXX_SOURCES
    filesystem/filesystem_common.h
    filesystem/operations.cpp
    filesystem/directory_iterator.cpp
    )
  # Filesystem uses __int128_t, which requires a definition of __muloi4 when
  # compiled with UBSAN. This definition is not provided by libgcc_s, but is
  # provided by compiler-rt. So we need to disable it to avoid having multiple
  # definitions. See filesystem/int128_builtins.cpp.
  if (NOT LIBCXX_USE_COMPILER_RT)
    list(APPEND LIBCXX_SOURCES
      filesystem/int128_builtins.cpp
      )
  endif()
endif()

# Add all the headers to the project for IDEs.
if (LIBCXX_CONFIGURE_IDE)
  file(GLOB_RECURSE LIBCXX_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/../include/*)
  if(WIN32)
    file( GLOB LIBCXX_WIN32_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/../include/support/win32/*.h)
    list(APPEND LIBCXX_HEADERS ${LIBCXX_WIN32_HEADERS})
  endif()
  # Force them all into the headers dir on MSVC, otherwise they end up at
  # project scope because they don't have extensions.
  if (MSVC_IDE)
    source_group("Header Files" FILES ${LIBCXX_HEADERS})
  endif()
endif()

if(NOT LIBCXX_INSTALL_LIBRARY)
  set(exclude_from_all EXCLUDE_FROM_ALL)
endif()

# If LIBCXX_CXX_ABI_LIBRARY_PATH is defined we want to add it to the search path.
add_link_flags_if(LIBCXX_CXX_ABI_LIBRARY_PATH
                  "${CMAKE_LIBRARY_PATH_FLAG}${LIBCXX_CXX_ABI_LIBRARY_PATH}")


if (LIBCXX_GENERATE_COVERAGE AND NOT LIBCXX_COVERAGE_LIBRARY)
  find_compiler_rt_library(profile LIBCXX_COVERAGE_LIBRARY)
endif()
add_library_flags_if(LIBCXX_COVERAGE_LIBRARY "${LIBCXX_COVERAGE_LIBRARY}")

if (APPLE AND LLVM_USE_SANITIZER)
  if (("${LLVM_USE_SANITIZER}" STREQUAL "Address") OR
      ("${LLVM_USE_SANITIZER}" STREQUAL "Address;Undefined") OR
      ("${LLVM_USE_SANITIZER}" STREQUAL "Undefined;Address"))
    set(LIBFILE "libclang_rt.asan_osx_dynamic.dylib")
  elseif("${LLVM_USE_SANITIZER}" STREQUAL "Undefined")
    set(LIBFILE "libclang_rt.ubsan_osx_dynamic.dylib")
  elseif("${LLVM_USE_SANITIZER}" STREQUAL "Thread")
    set(LIBFILE "libclang_rt.tsan_osx_dynamic.dylib")
  else()
    message(WARNING "LLVM_USE_SANITIZER=${LLVM_USE_SANITIZER} is not supported on OS X")
  endif()
  if (LIBFILE)
    find_compiler_rt_dir(LIBDIR)
    if (NOT IS_DIRECTORY "${LIBDIR}")
      message(FATAL_ERROR "Cannot find compiler-rt directory on OS X required for LLVM_USE_SANITIZER")
    endif()
    set(LIBCXX_SANITIZER_LIBRARY "${LIBDIR}/${LIBFILE}")
    set(LIBCXX_SANITIZER_LIBRARY "${LIBCXX_SANITIZER_LIBRARY}" PARENT_SCOPE)
    message(STATUS "Manually linking compiler-rt library: ${LIBCXX_SANITIZER_LIBRARY}")
    add_library_flags("${LIBCXX_SANITIZER_LIBRARY}")
    add_link_flags("-Wl,-rpath,${LIBDIR}")
  endif()
endif()

function(cxx_link_system_libraries target)
  target_add_link_flags_if_supported(${target} PRIVATE "-nodefaultlibs")
  target_add_compile_flags_if_supported(${target} PRIVATE "/Zl")
  target_add_compile_flags_if_supported(${target} PRIVATE "-Wno-deprecated-declarations")
  target_add_compile_flags_if_supported(${target} PRIVATE "-Wno-attributes")
  if (NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "NVHPC")
    target_add_link_flags_if_supported(${target} PRIVATE "/nodefaultlib")
  endif()

  if (LIBCXX_HAS_SYSTEM_LIB)
    target_link_libraries(${target} PUBLIC System)
  endif()

  if (LIBCXX_HAS_PTHREAD_LIB)
    target_link_libraries(${target} PUBLIC pthread)
  endif()

  if (LIBCXX_HAS_C_LIB)
    target_link_libraries(${target} PUBLIC c)
  endif()

  if (LIBCXX_HAS_M_LIB)
    target_link_libraries(${target} PUBLIC m)
  endif()

  if (LIBCXX_HAS_RT_LIB)
    target_link_libraries(${target} PUBLIC rt)
  endif()

  if (LIBCXX_USE_COMPILER_RT)
    find_compiler_rt_library(builtins LIBCXX_BUILTINS_LIBRARY)
    if (LIBCXX_BUILTINS_LIBRARY)
      target_link_libraries(${target} PUBLIC "${LIBCXX_BUILTINS_LIBRARY}")
    endif()
  elseif (LIBCXX_HAS_GCC_S_LIB)
    target_link_libraries(${target} PUBLIC gcc_s)
  endif()

  if (LIBCXX_HAVE_CXX_ATOMICS_WITH_LIB)
    target_link_libraries(${target} PUBLIC atomic)
  endif()

  if (MINGW)
    target_link_libraries(${target} PUBLIC "${MINGW_LIBRARIES}")
  endif()

  if (LIBCXX_TARGETING_MSVC)
    if (LIBCXX_DEBUG_BUILD)
      set(LIB_SUFFIX "d")
    else()
      set(LIB_SUFFIX "")
    endif()

    target_link_libraries(${target} PUBLIC ucrt${LIB_SUFFIX}) # Universal C runtime
    target_link_libraries(${target} PUBLIC vcruntime${LIB_SUFFIX}) # C++ runtime
    target_link_libraries(${target} PUBLIC msvcrt${LIB_SUFFIX}) # C runtime startup files
    target_link_libraries(${target} PUBLIC msvcprt${LIB_SUFFIX}) # C++ standard library. Required for exception_ptr internals.
    # Required for standards-complaint wide character formatting functions
    # (e.g. `printfw`/`scanfw`)
    target_link_libraries(${target} PUBLIC iso_stdio_wide_specifiers)
  endif()
endfunction()

if (LIBCXX_ENABLE_PARALLEL_ALGORITHMS AND NOT TARGET pstl::ParallelSTL)
  message(FATAL_ERROR "Could not find ParallelSTL")
endif()

function(cxx_set_common_defines name)
  if(LIBCXX_CXX_ABI_HEADER_TARGET)
    add_dependencies(${name} ${LIBCXX_CXX_ABI_HEADER_TARGET})
  endif()

  if (LIBCXX_ENABLE_PARALLEL_ALGORITHMS)
    target_link_libraries(${name} PUBLIC pstl::ParallelSTL)
  endif()
endfunction()

split_list(LIBCXX_COMPILE_FLAGS)
split_list(LIBCXX_LINK_FLAGS)

# Build the shared library.
if (LIBCXX_ENABLE_SHARED)
  add_library(cxx_shared SHARED ${exclude_from_all} ${LIBCXX_SOURCES} ${LIBCXX_HEADERS})
  if(COMMAND llvm_setup_rpath)
    llvm_setup_rpath(cxx_shared)
  endif()
  cxx_link_system_libraries(cxx_shared)
  target_link_libraries(cxx_shared PRIVATE ${LIBCXX_LIBRARIES})
  set_target_properties(cxx_shared
    PROPERTIES
      COMPILE_FLAGS "${LIBCXX_COMPILE_FLAGS}"
      LINK_FLAGS    "${LIBCXX_LINK_FLAGS}"
      OUTPUT_NAME   "c++"
      VERSION       "${LIBCXX_ABI_VERSION}.0"
      SOVERSION     "${LIBCXX_ABI_VERSION}"
      DEFINE_SYMBOL ""
  )
  cxx_add_basic_build_flags(cxx_shared)
  cxx_set_common_defines(cxx_shared)
  cxx_add_warning_flags(cxx_shared)
  cxx_add_windows_flags(cxx_shared)

  # Link against LLVM libunwind
  if (LIBCXXABI_USE_LLVM_UNWINDER)
    if (NOT LIBCXXABI_STATICALLY_LINK_UNWINDER_IN_SHARED_LIBRARY AND (TARGET unwind_shared OR HAVE_LIBUNWIND))
      target_link_libraries(cxx_shared PUBLIC unwind_shared)
      list(APPEND LIBCXX_INTERFACE_LIBRARIES unwind_shared) # For the linker script
    elseif (LIBCXXABI_STATICALLY_LINK_UNWINDER_IN_SHARED_LIBRARY AND (TARGET unwind_static OR HAVE_LIBUNWIND))
      # libunwind is already included in libc++abi
    else()
      target_link_libraries(cxx_shared PRIVATE unwind)
      list(APPEND LIBCXX_INTERFACE_LIBRARIES unwind) # For the linker script
    endif()
  endif()

  # Link against libc++abi
  if (LIBCXX_STATICALLY_LINK_ABI_IN_SHARED_LIBRARY)
    if (APPLE)
      target_link_libraries(cxx_shared PRIVATE "-Wl,-force_load" "${LIBCXX_CXX_STATIC_ABI_LIBRARY}")
    else()
      target_link_libraries(cxx_shared PRIVATE "-Wl,--whole-archive,-Bstatic" "${LIBCXX_CXX_STATIC_ABI_LIBRARY}" "-Wl,-Bdynamic,--no-whole-archive")
    endif()
  else()
    target_link_libraries(cxx_shared PUBLIC "${LIBCXX_CXX_SHARED_ABI_LIBRARY}")
    list(APPEND LIBCXX_INTERFACE_LIBRARIES "${LIBCXX_CXX_SHARED_ABI_LIBRARY}") # For the linker script
  endif()

  # Maybe re-export symbols from libc++abi
  if (APPLE AND (LIBCXX_CXX_ABI_LIBNAME STREQUAL "libcxxabi" OR
                 LIBCXX_CXX_ABI_LIBNAME STREQUAL "default")
            AND NOT DEFINED LIBCXX_OSX_REEXPORT_LIBCXXABI_SYMBOLS)
    set(LIBCXX_OSX_REEXPORT_LIBCXXABI_SYMBOLS ON)
  endif()

  if (LIBCXX_OSX_REEXPORT_LIBCXXABI_SYMBOLS)
    if ("${CMAKE_OSX_ARCHITECTURES}" MATCHES "^(armv6|armv7|armv7s)$")
      set(RE_EXPORT_LIST "${CMAKE_CURRENT_SOURCE_DIR}/../lib/libc++sjlj-abi.v${LIBCXX_LIBCUDACXXABI_VERSION}.exp")
    else()
      set(RE_EXPORT_LIST "${CMAKE_CURRENT_SOURCE_DIR}/../lib/libc++abi.v${LIBCXX_LIBCUDACXXABI_VERSION}.exp")
    endif()
    target_link_libraries(cxx_shared PRIVATE
      "-Wl,-unexported_symbols_list,${CMAKE_CURRENT_SOURCE_DIR}/../lib/libc++unexp.exp"
      "-Wl,-reexported_symbols_list,${RE_EXPORT_LIST}"
      "-Wl,-force_symbols_not_weak_list,${CMAKE_CURRENT_SOURCE_DIR}/../lib/notweak.exp"
      "-Wl,-force_symbols_weak_list,${CMAKE_CURRENT_SOURCE_DIR}/../lib/weak.exp")

    if (NOT LIBCXX_ENABLE_NEW_DELETE_DEFINITIONS)
      target_link_libraries(cxx_shared PRIVATE "-Wl,-reexported_symbols_list,${CMAKE_CURRENT_SOURCE_DIR}/../lib/libc++abi-new-delete.exp")
    endif()
  endif()

  # Generate a linker script in place of a libc++.so symlink. Rerun this command
  # after cxx builds.
  if (LIBCXX_ENABLE_ABI_LINKER_SCRIPT)
    # Get the name of the ABI library and handle the case where CXXABI_LIBNAME
    # is a target name and not a library. Ex cxxabi_shared.
    set(LIBCXX_INTERFACE_LIBRARY_NAMES)
    foreach(lib ${LIBCXX_INTERFACE_LIBRARIES})
      # FIXME: Handle cxxabi_static and unwind_static.
      if (TARGET ${lib} OR
          (${lib} MATCHES "cxxabi(_static|_shared)?" AND HAVE_LIBCXXABI) OR
          (${lib} MATCHES "unwind(_static|_shared)?" AND HAVE_LIBUNWIND))
        list(APPEND LIBCXX_INTERFACE_LIBRARY_NAMES "$<TARGET_PROPERTY:${lib},OUTPUT_NAME>")
      else()
        list(APPEND LIBCXX_INTERFACE_LIBRARY_NAMES "${lib}")
      endif()
    endforeach()
    add_custom_command(TARGET cxx_shared POST_BUILD
      COMMAND
        ${PYTHON_EXECUTABLE} ${LIBCXX_SOURCE_DIR}/utils/gen_link_script.py
      ARGS
        --input "$<TARGET_SONAME_FILE:cxx_shared>"
        --output "$<TARGET_LINKER_FILE:cxx_shared>"
        ${LIBCXX_INTERFACE_LIBRARY_NAMES}
      WORKING_DIRECTORY ${LIBCXX_BUILD_DIR}
    )
  endif()

  list(APPEND LIBCXX_BUILD_TARGETS "cxx_shared")
  if(WIN32 AND NOT MINGW AND NOT "${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Windows")
    # Since we most likely do not have a mt.exe replacement, disable the
    # manifest bundling.  This allows a normal cmake invocation to pass which
    # will attempt to use the manifest tool to generate the bundled manifest
    set_target_properties(cxx_shared PROPERTIES
                          APPEND_STRING PROPERTY LINK_FLAGS " /MANIFEST:NO")
  endif()
endif()

# Build the static library.
if (LIBCXX_ENABLE_STATIC)
  add_library(cxx_static STATIC ${exclude_from_all} ${LIBCXX_SOURCES} ${LIBCXX_HEADERS})
  cxx_link_system_libraries(cxx_static)
  target_link_libraries(cxx_static PRIVATE ${LIBCXX_LIBRARIES})
  set(CMAKE_STATIC_LIBRARY_PREFIX "lib")
  set_target_properties(cxx_static
    PROPERTIES
      COMPILE_FLAGS "${LIBCXX_COMPILE_FLAGS}"
      LINK_FLAGS    "${LIBCXX_LINK_FLAGS}"
      OUTPUT_NAME   "c++"
  )
  cxx_add_basic_build_flags(cxx_static)
  cxx_set_common_defines(cxx_static)
  cxx_add_warning_flags(cxx_static)
  cxx_add_windows_flags(cxx_static)

  if (LIBCXX_HERMETIC_STATIC_LIBRARY)
    # If the hermetic library doesn't define the operator new/delete functions
    # then its code shouldn't declare them with hidden visibility.  They might
    # actually be provided by a shared library at link time.
    if (LIBCXX_ENABLE_NEW_DELETE_DEFINITIONS)
      append_flags_if_supported(CXX_STATIC_LIBRARY_FLAGS -fvisibility-global-new-delete-hidden)
    endif()
    target_compile_options(cxx_static PRIVATE ${CXX_STATIC_LIBRARY_FLAGS})
    target_compile_definitions(cxx_static PRIVATE _LIBCUDACXX_DISABLE_VISIBILITY_ANNOTATIONS)
  endif()

  list(APPEND LIBCXX_BUILD_TARGETS "cxx_static")
  # Attempt to merge the libc++.a archive and the ABI library archive into one.
  if (LIBCXX_STATICALLY_LINK_ABI_IN_STATIC_LIBRARY)
    set(MERGE_ARCHIVES_SEARCH_PATHS "")
    if (LIBCXX_CXX_ABI_LIBRARY_PATH)
      set(MERGE_ARCHIVES_SEARCH_PATHS "-L${LIBCXX_CXX_ABI_LIBRARY_PATH}")
    endif()
    if (TARGET "${LIBCXX_CXX_STATIC_ABI_LIBRARY}" OR HAVE_LIBCXXABI)
      set(MERGE_ARCHIVES_ABI_TARGET "$<TARGET_LINKER_FILE:${LIBCXX_CXX_STATIC_ABI_LIBRARY}>")
    else()
      set(MERGE_ARCHIVES_ABI_TARGET
        "${CMAKE_STATIC_LIBRARY_PREFIX}${LIBCXX_CXX_STATIC_ABI_LIBRARY}${CMAKE_STATIC_LIBRARY_SUFFIX}")
    endif()
    if (APPLE)
      set(MERGE_ARCHIVES_LIBTOOL "--use-libtool" "--libtool" "${CMAKE_LIBTOOL}")
    endif()
    add_custom_command(TARGET cxx_static POST_BUILD
    COMMAND
      ${PYTHON_EXECUTABLE} ${LIBCXX_SOURCE_DIR}/utils/merge_archives.py
    ARGS
      -o $<TARGET_LINKER_FILE:cxx_static>
      --ar "${CMAKE_AR}" -v
      ${MERGE_ARCHIVES_LIBTOOL}
      "$<TARGET_LINKER_FILE:cxx_static>"
      "${MERGE_ARCHIVES_ABI_TARGET}"
      "${MERGE_ARCHIVES_SEARCH_PATHS}"
    WORKING_DIRECTORY ${LIBCXX_BUILD_DIR}
    )
  endif()
endif()

# Add a meta-target for both libraries.
add_custom_target(cxx DEPENDS cxx-headers ${LIBCXX_BUILD_TARGETS})

if (LIBCXX_ENABLE_EXPERIMENTAL_LIBRARY)
  set(LIBCXX_EXPERIMENTAL_SOURCES
    experimental/memory_resource.cpp
    )
  add_library(cxx_experimental STATIC ${LIBCXX_EXPERIMENTAL_SOURCES})
  if (LIBCXX_ENABLE_SHARED)
    target_link_libraries(cxx_experimental cxx_shared)
  else()
    target_link_libraries(cxx_experimental cxx_static)
  endif()

  cxx_add_basic_build_flags(cxx_experimental)
  cxx_set_common_defines(cxx_experimental)
  cxx_add_warning_flags(cxx_experimental)
  cxx_add_windows_flags(cxx_experimental)
  set_target_properties(cxx_experimental
    PROPERTIES
      OUTPUT_NAME   "c++experimental"
  )
endif()

if (LIBCXX_BUILD_EXTERNAL_THREAD_LIBRARY)
  file(GLOB LIBCXX_EXTERNAL_THREADING_SUPPORT_SOURCES ../test/support/external_threads.cpp)

  if (LIBCXX_ENABLE_SHARED)
    add_library(cxx_external_threads SHARED ${LIBCXX_EXTERNAL_THREADING_SUPPORT_SOURCES})
  else()
    add_library(cxx_external_threads STATIC ${LIBCXX_EXTERNAL_THREADING_SUPPORT_SOURCES})
  endif()

  set_target_properties(cxx_external_threads
    PROPERTIES
      LINK_FLAGS    "${LIBCXX_LINK_FLAGS}"
      COMPILE_FLAGS "${LIBCXX_COMPILE_FLAGS}"
      OUTPUT_NAME   "c++external_threads"
  )
endif()

if (LIBCXX_INSTALL_LIBRARY)
  if (LIBCXX_INSTALL_SHARED_LIBRARY)
    install(TARGETS cxx_shared
      ARCHIVE DESTINATION ${LIBCXX_INSTALL_PREFIX}${LIBCXX_INSTALL_LIBRARY_DIR} COMPONENT cxx
      LIBRARY DESTINATION ${LIBCXX_INSTALL_PREFIX}${LIBCXX_INSTALL_LIBRARY_DIR} COMPONENT cxx
      RUNTIME DESTINATION ${LIBCXX_INSTALL_PREFIX}bin COMPONENT cxx)
  endif()

  if (LIBCXX_INSTALL_STATIC_LIBRARY)
    install(TARGETS cxx_static
      ARCHIVE DESTINATION ${LIBCXX_INSTALL_PREFIX}${LIBCXX_INSTALL_LIBRARY_DIR} COMPONENT cxx
      LIBRARY DESTINATION ${LIBCXX_INSTALL_PREFIX}${LIBCXX_INSTALL_LIBRARY_DIR} COMPONENT cxx
      RUNTIME DESTINATION ${LIBCXX_INSTALL_PREFIX}bin COMPONENT cxx)
  endif()

  if(LIBCXX_INSTALL_EXPERIMENTAL_LIBRARY)
    install(TARGETS ${LIBCXX_INSTALL_TARGETS} ${experimental_lib}
      LIBRARY DESTINATION ${LIBCXX_INSTALL_PREFIX}${LIBCXX_INSTALL_LIBRARY_DIR} COMPONENT cxx
      ARCHIVE DESTINATION ${LIBCXX_INSTALL_PREFIX}${LIBCXX_INSTALL_LIBRARY_DIR} COMPONENT cxx
      RUNTIME DESTINATION ${LIBCXX_INSTALL_PREFIX}bin COMPONENT cxx)
  endif()

  # NOTE: This install command must go after the cxx install command otherwise
  # it will not be executed after the library symlinks are installed.
  if (LIBCXX_ENABLE_SHARED AND LIBCXX_ENABLE_ABI_LINKER_SCRIPT)
    # Replace the libc++ filename with $<TARGET_LINKER_FILE:cxx>
    # after we required CMake 3.0.
    install(FILES "${LIBCXX_LIBRARY_DIR}/libc++${CMAKE_SHARED_LIBRARY_SUFFIX}"
      DESTINATION ${LIBCXX_INSTALL_PREFIX}${LIBCXX_INSTALL_LIBRARY_DIR}
      COMPONENT libcxx)
  endif()
endif()

if (NOT CMAKE_CONFIGURATION_TYPES AND (LIBCXX_INSTALL_LIBRARY OR
                                       LIBCXX_INSTALL_HEADERS))
    if(LIBCXX_INSTALL_LIBRARY)
      set(lib_install_target cxx)
    endif()
    if (LIBCXX_INSTALL_EXPERIMENTAL_LIBRARY)
      set(experimental_lib_install_target cxx_experimental)
    endif()
    if(LIBCXX_INSTALL_HEADERS)
      set(header_install_target install-cxx-headers)
    endif()
    add_custom_target(install-cxx
                      DEPENDS ${lib_install_target}
                              ${experimental_lib_install_target}
                              ${header_install_target}
                      COMMAND "${CMAKE_COMMAND}"
                      -DCMAKE_INSTALL_COMPONENT=cxx
                      -P "${LIBCXX_BINARY_DIR}/cmake_install.cmake")
    add_custom_target(install-cxx-stripped
                      DEPENDS ${lib_install_target}
                              ${experimental_lib_install_target}
                              ${header_install_target}
                      COMMAND "${CMAKE_COMMAND}"
                      -DCMAKE_INSTALL_COMPONENT=cxx
                      -DCMAKE_INSTALL_DO_STRIP=1
                      -P "${LIBCXX_BINARY_DIR}/cmake_install.cmake")
    add_custom_target(install-libcxx DEPENDS install-cxx)
endif()
