# declaration of function
include(FindPackageHandleStandardArgs)

if(NOT VPVL2_ROOT_DIR)
  set(VPVL2_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")
endif()

function(__get_source_path output dir)
  set(${output} "${VPVL2_ROOT_DIR}/${dir}" PARENT_SCOPE)
endfunction()

function(__get_build_directory output)
  if(ANDROID)
    set(BUILD_DIRECTORY "build-android")
  else()
    string(TOLOWER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_TOLOWER)
    set(BUILD_DIRECTORY "build-${CMAKE_BUILD_TYPE_TOLOWER}")
  endif()
  set(${output} ${BUILD_DIRECTORY} PARENT_SCOPE)
endfunction()

function(__get_local_library_path_named output source_dir lib_dir)
  __get_build_directory(build_dir)
  if(MSVC)
    string(TOLOWER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_TOLOWER)
    set(${output} "${source_dir}/${build_dir}/${lib_dir}/${CMAKE_BUILD_TYPE_TOLOWER}" PARENT_SCOPE)
  else()
    set(${output} "${source_dir}/${build_dir}/${lib_dir}" PARENT_SCOPE)
  endif()
endfunction()

function(__get_local_library_path output source_dir)
  __get_local_library_path_named(output_to_reassign ${source_dir} "lib")
  set(${output} ${output_to_reassign} PARENT_SCOPE)
endfunction()

function(__get_install_path output dir)
  __get_source_path(source_dir ${dir})
  __get_build_directory(build_dir)
  set(${output} "${source_dir}/${build_dir}/install-root" PARENT_SCOPE)
endfunction()

function(vpvl2_detect_os)
  if(MSVC)
    set(VPVL2_OS_WINDOWS ON CACHE INTERNAL "OS is on windows" FORCE)
  elseif(APPLE)
    set(VPVL2_OS_OSX ON CACHE INTERNAL "OS is on OSX" FORCE)
  elseif(ANDROID)
    set(VPVL2_OS_ANDROID ON CACHE INTERNAL "OS is on Android" FORCE)
  elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(VPVL2_OS_LINUX ON CACHE INTERNAL "OS is on Linux" FORCE)
  else()
    # skip unknown
  endif()
endfunction()

function(vpvl2_detect_compiler_features)
  if(VPVL2_ENABLE_CXX11)
    include(CheckCXXSourceCompiles)
    set(CMAKE_REQUIRED_DEFINITIONS "-std=c++11")
    check_cxx_source_compiles("constexpr static inline int f() { return 0; } \n int main() { return f(); }" VPVL2_HAS_CXX11_DECL_CONSTEXPR)
    check_cxx_source_compiles("class C final {} c; \n int main() { return 0; }" VPVL2_HAS_CXX11_DECL_FINAL)
    check_cxx_source_compiles("class C { int m() noexcept { return 0; } } c; \n int main() { return 0; }" VPVL2_HAS_CXX11_DECL_NOEXCEPT)
    check_cxx_source_compiles("thread_local static bool b = true;  \n int main() { return 0; }" VPVL2_HAS_CXX11_DECL_THREAD_LOCAL)
  elseif(VPVL2_ENABLE_COMPILER_TLS)
    # check compiler static tls
    include(CheckCXXSourceCompiles)
    check_cxx_source_compiles("__thread int i; int main() { return 0; }" VPVL2_HAS_STATIC_TLS_GNU)
    if(NOT VPVL2_HAS_STATIC_TLS_GNU)
      check_cxx_source_compiles("__declspec(thread) int i; int main() { return 0; }" VPVL2_HAS_STATIC_TLS_MSVC)
    endif()
  endif()
endfunction()

function(vpvl2_set_library_properties target public_headers)
  if(WIN32 AND BUILD_SHARED_LIBS)
    set_target_properties(${target} PROPERTIES PREFIX "" SUFFIX .dll IMPORT_SUFFIX ${CMAKE_IMPORT_LIBRARY_SUFFIX})
  elseif(APPLE)
    # create as a framework if build on darwin environment
    if(BUILD_SHARED_LIBS AND FRAMEWORK)
      set_target_properties(${target} PROPERTIES FRAMEWORK true PROPERTIES PUBLIC_HEADER "${public_headers}")
    endif()
    set_target_properties(${target}
                          PROPERTIES
                          VERSION "${VPVL2_VERSION_MAJOR}.${VPVL2_VERSION_COMPAT}.${VPVL2_VERSION_MINOR}"
                          SOVERSION "${VPVL2_VERSION_MAJOR}.${VPVL2_VERSION_COMPAT}.0"
                          INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib")
  endif()
endfunction()

function(vpvl2_set_warnings)
  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR CMAKE_COMPILER_IS_GNUC)
  # set more warnings when clang or gcc is selected
    add_definitions(-W -Wall -Wextra -Wformat=2 -Wstrict-aliasing=2 -Wwrite-strings)
  elseif(MSVC)
    # disable _CRT_SECURE_NO_WARNINGS for surpressing warnings from vpvl2/Common.h
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE -D_SCL_SECURE_NO_WARNINGS /EHsc /wd4068 /wd4355 /wd4819)
  endif()
endfunction()

function(vpvl2_link_bullet target)
  if(NOT VPVL2_ENABLE_LAZY_LINK)
    target_link_libraries(${target} ${BULLET_DYNAMICS_LIB}
                                    ${BULLET_COLLISION_LIB}
                                    ${BULLET_SOFTBODY_LIB}
                                    ${BULLET_LINEARMATH_LIB})
  endif()
endfunction()

function(vpvl2_find_bullet)
  __get_install_path(BULLET_INSTALL_DIR "bullet-src")
  find_path(BULLET_INCLUDE_DIR NAMES btBulletCollisionCommon.h PATH_SUFFIXES include/bullet PATHS ${BULLET_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
  find_library(BULLET_LINEARMATH_LIB NAMES LinearMath PATH_SUFFIXES lib64 lib32 lib PATHS ${BULLET_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
  find_library(BULLET_COLLISION_LIB NAMES BulletCollision PATH_SUFFIXES lib64 lib32 lib PATHS ${BULLET_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
  find_library(BULLET_DYNAMICS_LIB NAMES BulletDynamics PATH_SUFFIXES lib64 lib32 lib PATHS ${BULLET_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
  find_library(BULLET_SOFTBODY_LIB NAMES BulletSoftBody PATH_SUFFIXES lib64 lib32 lib PATHS ${BULLET_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
  include_directories(${BULLET_INCLUDE_DIR})
  find_package_handle_standard_args(BULLET DEFAULT_MSG BULLET_INCLUDE_DIR BULLET_LINEARMATH_LIB BULLET_COLLISION_LIB BULLET_DYNAMICS_LIB BULLET_SOFTBODY_LIB)
endfunction()

function(vpvl2_link_assimp target)
  if(NOT VPVL2_ENABLE_LAZY_LINK AND (VPVL2_LINK_ASSIMP3 OR VPVL2_LINK_ASSIMP))
    target_link_libraries(${target} ${ASSIMP_LIBRARY})
  endif()
endfunction()

function(vpvl2_find_assimp)
  if(VPVL2_LINK_ASSIMP3 OR VPVL2_LINK_ASSIMP)
    __get_install_path(ASSIMP_INSTALL_DIR "assimp-src")
    if(VPVL2_LINK_ASSIMP3)
      find_path(ASSIMP_INCLUDE_DIR NAMES assimp/Importer.hpp PATH_SUFFIXES include PATHS ${ASSIMP_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
      find_library(ASSIMP_LIBRARY NAMES assimp assimpD PATH_SUFFIXES lib64 lib32 lib PATHS ${ASSIMP_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
      find_package_handle_standard_args(ASSIMP DEFAULT_MSG ASSIMP_INCLUDE_DIR ASSIMP_LIBRARY)
    elseif(VPVL2_LINK_ASSIMP)
      find_path(ASSIMP_INCLUDE_DIR NAMES assimp/assimp.h PATH_SUFFIXES include PATHS ${ASSIMP_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
      find_library(ASSIMP_LIBRARY NAMES assimp PATH_SUFFIXES lib64 lib32 lib PATHS ${ASSIMP_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
      find_package_handle_standard_args(ASSIMP DEFAULT_MSG ASSIMP_INCLUDE_DIR ASSIMP_LIBRARY)
    endif()
    include_directories(${ASSIMP_INCLUDE_DIR})
  endif()
endfunction()

function(vpvl2_link_vpvl target)
  if(NOT VPVL2_ENABLE_LAZY_LINK AND VPVL2_LINK_VPVL)
    target_link_libraries(${target} ${VPVL_LIBRARY})
  endif()
endfunction()

function(vpvl2_find_vpvl)
  if(VPVL2_LINK_VPVL)
    __get_install_path(VPVL_INSTALL_DIR "libvpvl")
    find_path(VPVL_INCLUDE_DIR NAMES vpvl/vpvl.h PATH_SUFFIXES include PATHS ${VPVL_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    find_library(VPVL_LIBRARY NAMES vpvl PATH_SUFFIXES lib64 lib32 lib PATHS ${VPVL_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    include_directories(${VPVL_INCLUDE_DIR} ${VPVL_CONFIG_DIR})
    find_package_handle_standard_args(VPVL DEFAULT_MSG VPVL_INCLUDE_DIR VPVL_LIBRARY)
  endif()
endfunction()

function(vpvl2_find_openmp)
  if(VPVL2_ENABLE_OPENMP)
    find_package(OpenMP)
    if(OPENMP_FOUND)
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
      message(STATUS "Activated OpenMP")
    endif()
  endif()
endfunction()

function(vpvl2_link_icu target)
  if(NOT VPVL2_ENABLE_LAZY_LINK AND VPVL2_ENABLE_EXTENSIONS_STRING)
    target_link_libraries(${target} ${ICU_LIBRARY_I18N} ${ICU_LIBRARY_UC} ${ICU_LIBRARY_DATA})
  endif()
endfunction()

function(vpvl2_find_icu)
  if(VPVL2_ENABLE_EXTENSIONS_STRING)
    if(MSVC)
      __get_source_path(ICU_INSTALL_DIR "icu4c-src")
    else()
      __get_install_path(ICU_INSTALL_DIR "icu4c-src")
    endif()
    find_path(ICU_INCLUDE_DIR NAMES unicode/unistr.h PATH_SUFFIXES include PATHS ${ICU_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    find_library(ICU_LIBRARY_DATA NAMES icudata icudt PATH_SUFFIXES lib64 lib32 lib PATHS ${ICU_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    find_library(ICU_LIBRARY_I18N NAMES icui18n icuin PATH_SUFFIXES lib64 lib32 lib PATHS ${ICU_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    find_library(ICU_LIBRARY_UC NAMES icuuc PATH_SUFFIXES lib64 lib32 lib PATHS ${ICU_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    include_directories(${ICU_INCLUDE_DIR})
    find_package_handle_standard_args(ICU DEFAULT_MSG ICU_INCLUDE_DIR ICU_LIBRARY_DATA ICU_LIBRARY_I18N ICU_LIBRARY_UC)
  endif()
endfunction()

function(vpvl2_link_tbb target)
  if(NOT VPVL2_ENABLE_LAZY_LINK AND VPVL2_LINK_INTEL_TBB)
    target_link_libraries(${target} ${TBB_LIBRARY} ${TBB_PROXY_LIBRARY} ${TBB_MALLOC_LIBRARY} ${TBB_MALLOC_PROXY_LIBRARY})
  endif()
endfunction()

function(vpvl2_find_tbb)
  if(VPVL2_LINK_INTEL_TBB)
    __get_source_path(TBB_SOURCE_DIRECTORY "tbb-src")
    if(WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Debug")
      find_library(TBB_LIBRARY NAMES tbb_debug PATH_SUFFIXES lib PATHS ${TBB_SOURCE_DIRECTORY} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
      find_library(TBB_PROXY_LIBRARY NAMES tbbproxy_debug PATH_SUFFIXES lib PATHS ${TBB_SOURCE_DIRECTORY} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
      find_library(TBB_MALLOC_LIBRARY NAMES tbbmalloc_debug PATH_SUFFIXES lib PATHS ${TBB_SOURCE_DIRECTORY} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
      find_library(TBB_MALLOC_PROXY_LIBRARY NAMES tbbmalloc_proxy_debug PATH_SUFFIXES lib PATHS ${TBB_SOURCE_DIRECTORY} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    else()
      execute_process(COMMAND make info WORKING_DIRECTORY ${TBB_SOURCE_DIRECTORY} OUTPUT_VARIABLE tbb_info_output)
      string(REGEX MATCHALL "tbb_build_prefix=[^\n]+" tbb_build_prefix_tmp ${tbb_info_output})
      string(REPLACE "tbb_build_prefix=" "" tbb_build_prefix ${tbb_build_prefix_tmp})
      string(TOLOWER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_TOLOWER)
      find_library(TBB_LIBRARY NAMES tbb tbb_debug PATHS "${TBB_SOURCE_DIRECTORY}/build/${tbb_build_prefix}_${CMAKE_BUILD_TYPE_TOLOWER}" NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    endif()
    find_path(TBB_INCLUDE_DIR tbb/tbb.h PATH_SUFFIXES include PATHS ${TBB_SOURCE_DIRECTORY} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    include_directories(${TBB_INCLUDE_DIR})
    find_package_handle_standard_args(TBB DEFAULT_MSG TBB_INCLUDE_DIR TBB_LIBRARY)
  endif()
endfunction()

function(vpvl2_link_zlib target)
  if(NOT VPVL2_ENABLE_LAZY_LINK AND VPVL2_ENABLE_EXTENSIONS_ARCHIVE)
    target_link_libraries(${target} ${ZLIB_LIBRARY})
  endif()
endfunction()

function(vpvl2_find_zlib)
  if(VPVL2_ENABLE_EXTENSIONS_ARCHIVE)
    __get_install_path(ZLIB_INSTALL_DIR "zlib-src")
    find_path(ZLIB_INCLUDE_DIR NAMES zlib.h PATH_SUFFIXES include PATHS ${ZLIB_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    find_library(ZLIB_LIBRARY NAMES z zlibstaticd zlibstatic PATH_SUFFIXES lib64 lib32 lib PATHS ${ZLIB_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    include_directories(${ZLIB_INCLUDE_DIR} ${ZLIB_INCLUDE_CONFIG_DIR})
    find_package_handle_standard_args(ZLIB DEFAULT_MSG ZLIB_INCLUDE_DIR ZLIB_LIBRARY)
  endif()
endfunction()

function(vpvl2_link_libxml2 target)
  if(NOT VPVL2_ENABLE_LAZY_LINK AND VPVL2_ENABLE_EXTENSIONS_PROJECT)
    target_link_libraries(${target} ${LIBXML2_LIBRARY})
  endif()
endfunction()

function(vpvl2_find_libxml2)
  if(VPVL2_ENABLE_EXTENSIONS_PROJECT)
    __get_install_path(LIBXML2_INSTALL_DIR "libxml2-src")
    find_path(LIBXML2_INCLUDE_DIR NAMES libxml/xmlwriter.h PATH_SUFFIXES include/libxml2 PATHS ${LIBXML2_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    find_library(LIBXML2_LIBRARY NAMES xml2 libxml2_a libxml2 PATH_SUFFIXES lib64 lib32 lib PATHS ${LIBXML2_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    include_directories(${LIBXML2_INCLUDE_DIR})
    find_package_handle_standard_args(LIBXML2 DEFAULT_MSG LIBXML2_INCLUDE_DIR LIBXML2_LIBRARY)
  endif()
endfunction()

function(vpvl2_link_alsoft target)
  if(NOT VPVL2_ENABLE_LAZY_LINK AND VPVL2_ENABLE_EXTENSIONS_APPLICATIONCONTEXT)
    target_link_libraries(${target} ${ALSOFT_LIBRARY})
  endif()
endfunction()

function(vpvl2_find_alsoft)
  if(VPVL2_ENABLE_EXTENSIONS_APPLICATIONCONTEXT)
    __get_install_path(ALSOFT_INSTALL_DIR "openal-soft-src")
    if(WIN32)
      find_path(ALSOFT_INCLUDE_DIR NAMES al.h PATH_SUFFIXES include/AL PATHS ${ALSOFT_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    else()
      find_path(ALSOFT_INCLUDE_DIR NAMES OpenAL/al.h AL/al.h PATH_SUFFIXES include PATHS ${ALSOFT_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    endif()
    find_library(ALSOFT_LIBRARY NAMES OpenAL32 openal PATH_SUFFIXES lib64 lib32 lib PATHS ${ALSOFT_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    include_directories(${ALSOFT_INCLUDE_DIR})
    find_package_handle_standard_args(ALSOFT DEFAULT_MSG ALSOFT_INCLUDE_DIR ALSOFT_LIBRARY)
  endif()
endfunction()

function(vpvl2_link_alure target)
  if(NOT VPVL2_ENABLE_LAZY_LINK AND VPVL2_ENABLE_EXTENSIONS_APPLICATIONCONTEXT)
    target_link_libraries(${target} ${ALURE_LIBRARY})
  endif()
endfunction()

function(vpvl2_find_alure)
  if(VPVL2_ENABLE_EXTENSIONS_APPLICATIONCONTEXT)
    __get_install_path(ALURE_INSTALL_DIR "alure-src")
    find_path(ALURE_INCLUDE_DIR NAMES OpenAL/alure.h AL/alure.h PATH_SUFFIXES include PATHS ${ALURE_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    find_library(ALURE_LIBRARY NAMES ALURE32-static alure-static ALURE32 alure PATH_SUFFIXES lib64 lib32 lib PATHS ${ALURE_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    include_directories(${ALURE_INCLUDE_DIR})
    find_package_handle_standard_args(ALURE DEFAULT_MSG ALURE_INCLUDE_DIR ALURE_LIBRARY)
  endif()
endfunction()

function(vpvl2_link_gl_runtime target)
  if(NOT VPVL2_ENABLE_LAZY_LINK)
    target_link_libraries(${target} ${OPENGL_gl_LIBRARY})
  endif()
endfunction()

function(vpvl2_find_gl_runtime)
  if(VPVL2_ENABLE_EXTENSIONS_APPLICATIONCONTEXT)
    if(VPVL2_ENABLE_OSMESA)
      __get_source_path(MESA3D_SOURCE_DIR "mesa-src")
      find_library(MESA3D_MESA_LIBRARY NAMES mesa PATH_SUFFIXES "embed-darwin-x86_64/mesa" "darwin-x86_64/mesa" PATHS "${MESA3D_SOURCE_DIR}/build" NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
      find_library(MESA3D_OSMESA_LIBRARY NAMES osmesa PATH_SUFFIXES "embed-darwin-x86_64/mesa/drivers/osmesa" "darwin-x86_64/mesa/drivers/osmesa" PATHS "${MESA3D_SOURCE_DIR}/build" NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
      set(OPENGL_gl_LIBRARY "${OPENGL_gl_mesa_LIBRARY} ${OPENGL_gl_osmesa_LIBRARY}")
      find_path(OPENGL_INCLUDE_DIR GL/osmesa.h PATH_SUFFIXES include PATHS ${MESA3D_SOURCE_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    elseif(VPVL2_ENABLE_GLES2)
      find_path(OPENGL_INCLUDE_DIR NAMES OpenGLES2/gl2.h GLES2/gl2.h PATHS $ENV{QTSDK_TOOLCHAIN}/include/QtANGLE $ENV{EMSCRIPTEN}/system/include)
      if(NOT VPVL2_ENABLE_LAZY_LINK)
        find_library(OPENGL_gl_LIBRARY NAMES ppapi_gles2 libGLESv2 GLESv2 PATHS $ENV{QTSDK_TOOLCHAIN}/lib)
      endif()
    elseif(NOT VPVL2_ENABLE_LAZY_LINK)
      find_package(OpenGL REQUIRED)
    endif()
    include_directories(${OPENGL_INCLUDE_DIR})
    message(STATUS "Activated OpenGL")
  endif()
endfunction()

function(vpvl2_link_cg_runtime target)
  if(NOT VPVL2_ENABLE_LAZY_LINK AND VPVL2_ENABLE_NVIDIA_CG AND VPVL2_ENABLE_EXTENSIONS_APPLICATIONCONTEXT)
    target_link_libraries(${target} ${CG_LIBRARY} ${CG_GL_LIBRARY})
  endif()
endfunction()

function(vpvl2_find_cg_runtime)
  if(VPVL2_ENABLE_NVIDIA_CG AND VPVL2_ENABLE_EXTENSIONS_APPLICATIONCONTEXT)
    find_package(Cg REQUIRED)
    include_directories(${CG_INCLUDE_DIR})
    message(STATUS "Added OpenGL")
  endif()
endfunction()

function(vpvl2_link_cl_runtime target)
  if(NOT VPVL2_ENABLE_LAZY_LINK AND VPVL2_ENABLE_OPENCL)
    target_link_libraries(${target} ${OPENCL_LIBRARY})
  endif()
endfunction()

function(vpvl2_find_cl_runtime)
  if(VPVL2_ENABLE_OPENCL)
    find_library(OPENCL_LIBRARY NAMES OpenCL)
    find_path(OPENCL_INCLUDE_DIR cl.h PATH_SUFFIXES include/OpenCL include/CL include)
    include_directories(${OPENCL_INCLUDE_DIR})
    find_package_handle_standard_args(OPENCL DEFAULT_MSG OPENCL_INCLUDE_DIR OPENCL_LIBRARY)
  endif()
endfunction()

function(vpvl2_find_gli)
  __get_source_path(GLI_SOURCE_DIRECTORY "gli-src")
  find_path(GLI_INCLUDE_DIR NAMES gli/gli.hpp PATHS ${GLI_SOURCE_DIRECTORY} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
  include_directories(${GLI_INCLUDE_DIR})
  find_package_handle_standard_args(GLI DEFAULT_MSG GLI_INCLUDE_DIR)
endfunction()

function(vpvl2_find_glm)
  __get_source_path(GLM_SOURCE_DIRECTORY "glm-src")
  find_path(GLM_INCLUDE_DIR NAMES glm/glm.hpp PATHS ${GLM_SOURCE_DIRECTORY} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
  include_directories(${GLM_INCLUDE_DIR})
  find_package_handle_standard_args(GLM DEFAULT_MSG GLM_INCLUDE_DIR)
endfunction()

function(vpvl2_link_glog target)
  if(NOT VPVL2_ENABLE_LAZY_LINK AND VPVL2_LINK_GLOG)
    target_link_libraries(${target} ${GLOG_LIBRARY})
  endif()
endfunction()

function(vpvl2_find_glog)
  if(VPVL2_LINK_GLOG)
    if(MSVC)
      __get_source_path(GLOG_INSTALL_DIR "glog-src")
      find_path(GLOG_INCLUDE_DIR NAMES glog/logging.h PATH_SUFFIXES src/windows PATHS ${GLOG_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
      # libglog always statically be linked
      find_library(GLOG_LIBRARY NAMES libglog_static PATH_SUFFIXES ${CMAKE_BUILD_TYPE} PATHS ${GLOG_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    else()
      __get_install_path(GLOG_INSTALL_DIR "glog-src")
      find_path(GLOG_INCLUDE_DIR NAMES glog/logging.h PATH_SUFFIXES include PATHS ${GLOG_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
      find_library(GLOG_LIBRARY NAMES glog PATH_SUFFIXES lib64 lib32 lib PATHS ${GLOG_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    endif()
    include_directories(${GLOG_INCLUDE_DIR})
    find_package_handle_standard_args(GLOG DEFAULT_MSG GLOG_INCLUDE_DIR GLOG_LIBRARY)
  endif()
endfunction()

function(vpvl2_link_hlslxc target)
  if(NOT VPVL2_ENABLE_LAZY_LINK AND VPVL2_ENABLE_EXTENSIONS_EFFECT)
    target_link_libraries(${target} ${HLSLXC_LIBRARY})
  endif()
endfunction()

function(vpvl2_find_hlslxc)
  if(VPVL2_ENABLE_EXTENSIONS_EFFECT)
    __get_source_path(HLSLXC_SRC_DIR "hlslxc-src")
    find_path(HLSLXC_INCLUDE_DIR NAMES toGLSL.h PATHS ${HLSLXC_SRC_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    find_library(HLSLXC_LIBRARY NAMES libHLSLcc PATH_SUFFIXES lib PATHS ${HLSLXC_SRC_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    include_directories(${HLSLXC_INCLUDE_DIR} "${HLSLXC_INCLUDE_DIR}/cbstring")
    find_package_handle_standard_args(HLSLXC DEFAULT_MSG HLSLXC_INCLUDE_DIR HLSLXC_LIBRARY)
  endif()
endfunction()

function(vpvl2_link_regal target)
  if(NOT VPVL2_ENABLE_LAZY_LINK AND VPVL2_LINK_REGAL)
    target_link_libraries(${target} ${REGAL_LIBRARY})
  endif()
endfunction()

function(vpvl2_find_regal)
  if(VPVL2_LINK_REGAL)
    __get_source_path(REGAL_INSTALL_DIR "regal-src")
    find_path(REGAL_INCLUDE_DIR NAMES GL/Regal.h PATH_SUFFIXES include PATHS ${REGAL_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    include_directories(${REGAL_INCLUDE_DIR})
    if(MSVC)
      find_library(REGAL_LIBRARY NAMES regal32 PATHS ${REGAL_INSTALL_DIR}/build/win32/vs2010/regal/${CMAKE_BUILD_TYPE}/win32 NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    elseif(APPLE)
      find_library(REGAL_LIBRARY NAMES Regal PATH_SUFFIXES lib/darwin PATHS ${REGAL_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
      find_library(REGAL_LIBRARY NAMES Regal PATH_SUFFIXES lib/linux PATHS ${REGAL_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    else()
      find_library(REGAL_LIBRARY NAMES Regal PATH_SUFFIXES lib64 lib32 lib PATHS ${REGAL_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    endif()
    find_package_handle_standard_args(REGAL DEFAULT_MSG REGAL_INCLUDE_DIR REGAL_LIBRARY)
  endif()
endfunction()

function(vpvl2_link_nvfx target)
  if(NOT VPVL2_ENABLE_LAZY_LINK AND VPVL2_LINK_NVFX)
    target_link_libraries(${target} ${NVFX_FXPARSER_LIBRARY} ${NVFX_FXLIBGL_LIBRARY} ${NVFX_FXLIB_LIBRARY})
  endif()
endfunction()

function(vpvl2_find_nvfx)
  if(VPVL2_LINK_NVFX)
    __get_install_path(NVFX_INSTALL_DIR "nvFX-src")
    find_path(NVFX_INCLUDE_DIR NAMES FxParser.h PATH_SUFFIXES include PATHS ${NVFX_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    find_library(NVFX_FXLIB_LIBRARY NAMES FxLib64 FxLib64D FxLib FxLibD PATH_SUFFIXES lib64 lib32 lib PATHS ${NVFX_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    find_library(NVFX_FXLIBGL_LIBRARY NAMES FxLibGL64 FxLibGL64D FxLibGL FxLibGLD PATH_SUFFIXES lib64 lib32 lib PATHS ${NVFX_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    find_library(NVFX_FXPARSER_LIBRARY NAMES FxParser64 FxParser64D FxParser FxParserD PATH_SUFFIXES lib64 lib32 lib PATHS ${NVFX_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    include_directories(${NVFX_INCLUDE_DIR})
    find_package_handle_standard_args(NVFX DEFAULT_MSG NVFX_INCLUDE_DIR NVFX_FXLIB_LIBRARY NVFX_FXLIBGL_LIBRARY NVFX_FXPARSER_LIBRARY)
  endif()
endfunction()

function(vpvl2_link_atb target)
  if(NOT VPVL2_ENABLE_LAZY_LINK AND VPVL2_LINK_ATB)
    target_link_libraries(${target} ${ATB_LIBRARY})
    if(APPLE)
      find_library(COCOA_FRAMEWORK NAMES Cocoa)
      target_link_libraries(${target} ${COCOA_FRAMEWORK})
     endif()
  endif()
endfunction()

function(vpvl2_find_atb)
  if(VPVL2_LINK_ATB)
    __get_source_path(ATB_INSTALL_DIR "AntTweakBar-src")
    find_path(ATB_INCLUDE_DIR NAMES AntTweakBar.h PATH_SUFFIXES include PATHS ${ATB_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    find_library(ATB_LIBRARY NAMES AntTweakBar PATH_SUFFIXES lib64 lib32 lib PATHS ${ATB_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    include_directories(${ATB_INCLUDE_DIR})
    find_package_handle_standard_args(ANTTWEAKBAR DEFAULT_MSG ATB_INCLUDE_DIR ATB_LIBRARY)
  endif()
endfunction()

function(vpvl2_link_freeimage target)
  if(NOT VPVL2_ENABLE_LAZY_LINK AND VPVL2_LINK_FREEIMAGE)
    target_link_libraries(${target} ${FREEIMAGE_LIBRARY})
  endif()
endfunction()

function(vpvl2_find_freeimage)
  if(VPVL2_LINK_FREEIMAGE)
    find_path(FREEIMAGE_INCLUDE_DIR NAMES FreeImage.h PATH_SUFFIXES include)
    find_library(FREEIMAGE_LIBRARY NAMES freeimage PATH_SUFFIXESlib lib64 lib32 )
    include_directories(${FREEIMAGE_INCLUDE_DIR})
    find_package_handle_standard_args(FREEIMAGE DEFAULT_MSG FREEIMAGE_INCLUDE_DIR FREEIMAGE_LIBRARY)
  endif()
endfunction()

function(vpvl2_create_executable target)
  if(target)
    add_dependencies(${target} ${VPVL2_PROJECT_NAME})
    target_link_libraries(${target} ${VPVL2_PROJECT_NAME})
    vpvl2_link_all(${target})
  endif()
endfunction()

function(vpvl2_add_qt_helpers)
  if(VPVL2_BUILD_QT_RENDERER OR VPVL2_LINK_QT)
    find_package(Qt5Core QUIET)
    if (Qt5Core_FOUND)
      qt5_add_resources(vpvl2qtcommon_rc_src "${CMAKE_CURRENT_SOURCE_DIR}/src/qt/resources/libvpvl2qtcommon.qrc")
    else()
      find_package(Qt4 4.8 REQUIRED QtCore QtGui QtOpenGL)
      include(${QT_USE_FILE})
      qt4_add_resources(vpvl2qtcommon_rc_src "${CMAKE_CURRENT_SOURCE_DIR}/src/qt/resources/libvpvl2qtcommon.qrc")
    endif()
    file(GLOB vpvl2qtcommon_sources_common "${CMAKE_CURRENT_SOURCE_DIR}/src/qt/common/*.cc")
    file(GLOB vpvl2qtcommon_sources_unzip "${CMAKE_CURRENT_SOURCE_DIR}/src/qt/unzip/*.c")
    file(GLOB vpvl2qtcommon_headers_extensions "${CMAKE_CURRENT_SOURCE_DIR}/include/vpvl2/extensions/details/*.h")
    file(GLOB vpvl2qtcommon_headers "${CMAKE_CURRENT_SOURCE_DIR}/include/vpvl2/qt/*.h")
    source_group("VPVL2 for Qt sources" FILES ${vpvl2qtcommon_sources_common}
                                              ${vpvl2qtcommon_sources_unzip}
                                              ${vpvl2qtcommon_headers_extensions}
                                              ${vpvl2qtcommon_headers})
    set(vpvl2_qt_sources "${CMAKE_CURRENT_SOURCE_DIR}/render/qt/main.cc"
                         "${CMAKE_CURRENT_SOURCE_DIR}/render/qt/UI.cc")
    include_directories("${CMAKE_CURRENT_SOURCE_DIR}/render/qt"
                        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpvl2/qt")
    if(APPLE AND NOT Qt5Core_FOUND)
      find_library(COCOA_LIBRARY NAMES Cocoa)
      find_path(COCOA_INCLUDE_DIR NAMES Cocoa/Cocoa.h)
      file(GLOB vpvl2qtcommon_sources_osx "${CMAKE_CURRENT_SOURCE_DIR}/src/qt/osx/*.mm")
      include_directories(${COCOA_INCLUDE_DIR})
    endif()
    add_library(vpvl2qtcommon ${VPVL2_LIB_TYPE} ${vpvl2qtcommon_sources_common} ${vpvl2qtcommon_sources_unzip}
                                                ${vpvl2qtcommon_headers} ${vpvl2qtcommon_headers_extensions}
                                                ${vpvl2qtcommon_sources_osx} ${vpvl2qtcommon_rc_src})
    vpvl2_set_library_properties(vpvl2qtcommon ${vpvl2qtcommon_headers})
    target_link_libraries(vpvl2qtcommon ${VPVL2_PROJECT_NAME})
    if (Qt5Core_FOUND)
      qt5_use_modules(vpvl2qtcommon Concurrent Widgets Core)
    else()
      target_link_libraries(vpvl2qtcommon ${VPVL2_PROJECT_NAME} ${QT_LIBRARIES})
    endif()
    if(APPLE)
      target_link_libraries(vpvl2qtcommon ${COCOA_LIBRARY})
    endif()
    set(VPVL2QTCOMMON_OUTPUT_NAME "vpvl2qtcommon")
    set_target_properties(vpvl2qtcommon PROPERTIES OUTPUT_NAME ${VPVL2QTCOMMON_OUTPUT_NAME}
                                                   VERSION ${VPVL2_VERSION}
                                                   SOVERSION ${VPVL2_VERSION_COMPATIBLE})
    if(VPVL2_BUILD_QT_RENDERER)
      set(VPVL2_EXECUTABLE vpvl2_qt)
      add_executable(${VPVL2_EXECUTABLE} ${vpvl2_qt_sources} ${vpvl2_internal_headers})
      add_dependencies(${VPVL2_EXECUTABLE} vpvl2qtcommon)
      vpvl2_link_alsoft(${VPVL2_EXECUTABLE})
      vpvl2_link_alure(${VPVL2_EXECUTABLE})
      if (Qt5Core_FOUND)
        qt5_use_modules(${VPVL2_EXECUTABLE} Widgets OpenGL Concurrent Core)
        target_link_libraries(${VPVL2_EXECUTABLE} vpvl2qtcommon)
      else()
        target_link_libraries(${VPVL2_EXECUTABLE} ${VPVL2_PROJECT_NAME} vpvl2qtcommon ${QT_LIBRARIES})
      endif()
      vpvl2_create_executable(${VPVL2_EXECUTABLE})
    endif()
    if(VPVL2_ENABLE_TEST)
      __get_source_path(GTEST_INSTALL_DIR "gtest-src")
      __get_source_path(GMOCK_INSTALL_DIR "gmock-src")
      file(GLOB vpvl2_unit_tests_sources "${CMAKE_CURRENT_SOURCE_DIR}/test/*.cc")
      source_group("VPVL2 Test Case Classes" FILES ${vpvl2_unit_tests_sources})
      file(GLOB vpvl2_mock_headers "${CMAKE_CURRENT_SOURCE_DIR}/test/mock/*.h")
      source_group("VPVL2 Mock Classes" FILES ${vpvl2_mock_headers})
      file(GLOB gtest_source "${GTEST_INSTALL_DIR}/src/gtest-all.cc")
      file(GLOB gmock_source "${GMOCK_INSTALL_DIR}/src/gmock-all.cc")
      if(Qt5Core_FOUND)
        qt5_add_resources(vpvl2_unit_tests_qrc "${CMAKE_CURRENT_SOURCE_DIR}/test/fixtures.qrc")
      else()
        qt4_add_resources(vpvl2_unit_tests_qrc "${CMAKE_CURRENT_SOURCE_DIR}/test/fixtures.qrc")
      endif()
      add_executable(vpvl2_unit_tests ${vpvl2_unit_tests_sources} ${vpvl2_mock_headers} ${vpvl2_unit_tests_qrc} ${gtest_source} ${gmock_source})
      if(Qt5Core_FOUND)
        qt5_use_modules(vpvl2_unit_tests OpenGL Concurrent)
      endif()
      include_directories(${GTEST_INSTALL_DIR} "${GTEST_INSTALL_DIR}/include")
      include_directories(${GMOCK_INSTALL_DIR} "${GMOCK_INSTALL_DIR}/include")
      target_link_libraries(vpvl2_unit_tests vpvl2qtcommon ${VPVL2_PROJECT_NAME} ${QT_LIBRARIES})
      add_dependencies(vpvl2_unit_tests ${VPVL2_PROJECT_NAME} vpvl2qtcommon)
      vpvl2_link_all(vpvl2_unit_tests)
      add_executable(vpvl2_generator_test "${CMAKE_CURRENT_SOURCE_DIR}/test/generator/main.cc")
      target_link_libraries(vpvl2_generator_test ${VPVL2_PROJECT_NAME})
    endif()
  endif()
endfunction()

function(vpvl2_add_sdl_renderer)
  if(VPVL2_LINK_SDL2)
    __get_install_path(SDL_INSTALL_DIR "SDL2-src")
    find_library(SDL_LIBRARY NAMES SDL2 PATH_SUFFIXES lib PATHS ${SDL_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    find_library(SDL_MAIN_LIBRARY NAMES SDL2main PATH_SUFFIXES lib PATHS ${SDL_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    find_path(SDL_INCLUDE_DIR NAMES SDL.h PATH_SUFFIXES include/SDL2 PATHS ${SDL_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    add_definitions(${SDL_DEFINITIONS})
    include_directories(${SDL_INCLUDE_DIR} ${SDLIMAGE_INCLUDE_DIR})
    set(vpvl2_sdl_sources "render/sdl/main.cc")
    set(VPVL2_EXECUTABLE vpvl2_sdl)
    add_executable(${VPVL2_EXECUTABLE} ${vpvl2_sdl_sources})
    target_link_libraries(${VPVL2_EXECUTABLE} ${SDL_MAIN_LIBRARY} ${SDL_LIBRARY})# ${SDLIMAGE_LIBRARY})
    vpvl2_create_executable(${VPVL2_EXECUTABLE})
    if(WIN32)
      find_library(WIN32_WINMM_LIBRARY NAMES winmm)
      find_library(WIN32_VERSION_LIBRARY NAMES version)
      find_library(WIN32_IMM_LIBRARY NAMES imm32)
      target_link_libraries(${VPVL2_EXECUTABLE} ${WIN32_WINMM_LIBRARY} ${WIN32_IMM_LIBRARY} ${WIN32_VERSION_LIBRARY})
    endif()
  endif()
endfunction()

function(vpvl2_add_glfw_renderer)
  if(VPVL2_LINK_GLFW)
    __get_install_path(GLFW_INSTALL_DIR "glfw-src")
    find_path(GLFW_INCLUDE_DIR NAMES GLFW/glfw3.h PATH_SUFFIXES include PATHS ${GLFW_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    find_library(GLFW_LIBRARY NAMES glfw3 glfw PATH_SUFFIXES lib64 lib32 lib PATHS ${GLFW_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    include_directories(${GLFW_INCLUDE_DIR})
    set(vpvl2_glfw_sources "render/glfw/main.cc")
    set(VPVL2_EXECUTABLE vpvl2_glfw)
    add_executable(${VPVL2_EXECUTABLE} ${vpvl2_glfw_sources})
    if(APPLE)
      find_library(COCOA_LIBRARY NAMES Cocoa)
      find_library(IOKIT_LIBRARY NAMES IOKit)
      find_library(COREFOUNDATION_LIBRARY NAMES CoreFoundation)
      target_link_libraries(${VPVL2_EXECUTABLE} ${COCOA_LIBRARY} ${IOKIT_LIBRARY} ${COREFOUNDATION_LIBRARY})
    endif()
    target_link_libraries(${VPVL2_EXECUTABLE} ${GLFW_LIBRARY})
    vpvl2_create_executable(${VPVL2_EXECUTABLE})
  endif()
endfunction()

function(vpvl2_add_sfml_renderer)
  if(VPVL2_LINK_SFML)
    __get_install_path(SFML2_INSTALL_DIR "SFML2-src")
    find_library(SFML2_GRAPHICS_LIBRARY NAMES sfml-graphicss-s sfml-graphics sfml-graphics-s-d sfml-graphics-d PATH_SUFFIXES lib64 lib32 lib PATHS ${SFML2_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    find_library(SFML2_SYSTEM_LIBRARY NAMES sfml-system-s sfml-system sfml-system-s-d sfml-system-d PATH_SUFFIXES lib64 lib32 lib PATHS ${SFML2_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    find_library(SFML2_WINDOW_LIBRARY NAMES sfml-window-s sfml-window sfml-window-s-d sfml-window-d PATH_SUFFIXES lib64 lib32 lib PATHS ${SFML2_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    find_path(SFML_INCLUDE_DIR SFML/System.hpp PATH_SUFFIXES include PATHS ${SFML2_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    include_directories(${SFML_INCLUDE_DIR})
    set(vpvl2_sfml_sources "render/sfml/main.cc")
    set(VPVL2_EXECUTABLE vpvl2_sfml)
    add_executable(${VPVL2_EXECUTABLE} ${vpvl2_sfml_sources})
    target_link_libraries(${VPVL2_EXECUTABLE} ${SFML2_GRAPHICS_LIBRARY}
                                              ${SFML2_WINDOW_LIBRARY}
                                              ${SFML2_SYSTEM_LIBRARY})
    if(APPLE)
      find_library(COREFOUNDATION_FRAMEWORK NAMES CoreFoundation)
      find_path(COREFOUNDATION_INCLUDE_PATH "CoreFoundation/CoreFoundation.h")
      target_link_libraries(${VPVL2_EXECUTABLE} ${COREFOUNDATION_FRAMEWORK})
      include_directories(${COREFOUNDATION_INCLUDE_PATH})
    endif()
    vpvl2_create_executable(${VPVL2_EXECUTABLE})
  endif()
endfunction()

function(vpvl2_add_allegro_renderer)
  if(VPVL2_LINK_ALLEGRO)
    __get_install_path(ALLEGRO_INSTALL_DIR "allegro-src")
    find_library(ALLEGRO_LIBRARY NAMES allegro-static allegro allegro-debug-static allegro-debug PATH_SUFFIXES lib64 lib32 lib PATHS ${ALLEGRO_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    find_library(ALLEGRO_MAIN_LIBRARY NAMES allegro_main-static allegro_main allegro_main-debug-static allegro_main-debug PATH_SUFFIXES lib64 lib32 lib PATHS ${ALLEGRO_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    find_path(ALLEGRO_INCLUDE_DIR NAMES allegro5/allegro5.h PATH_SUFFIXES include PATHS ${ALLEGRO_INSTALL_DIR} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    include_directories(${ALLEGRO_INCLUDE_DIR})
    set(vpvl2_sfml_sources "render/allegro/main.cc")
    set(VPVL2_EXECUTABLE vpvl2_allegro)
    add_executable(${VPVL2_EXECUTABLE} ${vpvl2_sfml_sources})
    target_link_libraries(${VPVL2_EXECUTABLE} ${ALLEGRO_MAIN_LIBRARY} ${ALLEGRO_LIBRARY})
    if(APPLE)
      find_library(IOKIT_FRAMEWORK NAMES IOKit)
      target_link_libraries(${VPVL2_EXECUTABLE} ${IOKIT_FRAMEWORK})
    endif()
    vpvl2_create_executable(${VPVL2_EXECUTABLE})
  endif()
endfunction()

function(vpvl2_add_egl_renderer)
  if(VPVL2_LINK_EGL)
    find_path(EGL_INCLUDE_DIR NAMES EGL/egl.h)
    find_library(EGL_LIBRARY NAMES EGL)
    set(vpvl2_egl_sources "render/egl/main.cc")
    set(VPVL2_EXECUTABLE vpvl2_egl)
    add_executable(${VPVL2_EXECUTABLE} ${vpvl2_egl_sources})
    target_link_libraries(${VPVL2_EXECUTABLE} ${EGL_LIBRARY})
    include_directories(${EGL_INCLUDE_DIR})
    if(VPVL2_PLATFORM_RASPBERRY_PI)
      find_path(VCOS_INCLUDE_DIR vcos_platform_types.h)
      find_library(BCM_HOST_LIBRARY NAMES bcm_host)
      find_library(VCOS_LIBRARY NAMES vcos)
      find_library(VCHIQ_ARM_LIBRARY NAMES vchiq_arm)
      include_directories(${VCOS_INCLUDE_DIR})
      target_link_libraries(${VPVL2_EXECUTABLE} ${BCM_HOST_LIBRARY} ${VCOS_LIBRARY} ${VCHIQ_ARM_LIBRARY})
    elseif(UNIX)
      find_package(X11)
      target_link_libraries(${VPVL2_EXECUTABLE} ${X11_LIBRARIES})
      include_directories(${X11_INCLUDE_DIR})
    endif()
    vpvl2_create_executable(${VPVL2_EXECUTABLE})
  endif()
endfunction()

function(vpvl2_find_all)
  vpvl2_find_vpvl()
  vpvl2_find_nvfx()
  vpvl2_find_regal()
  vpvl2_find_atb()
  vpvl2_find_hlslxc()
  vpvl2_find_alsoft()
  vpvl2_find_alure()
  vpvl2_find_freeimage()
  vpvl2_find_tbb()
  vpvl2_find_bullet()
  vpvl2_find_assimp()
  vpvl2_find_icu()
  vpvl2_find_glog()
  vpvl2_find_glm()
  vpvl2_find_zlib()
  vpvl2_find_openmp()
  vpvl2_find_cg_runtime()
  vpvl2_find_cl_runtime()
  vpvl2_find_gl_runtime()
endfunction()

function(vpvl2_link_all target)
  vpvl2_link_vpvl(${target})
  vpvl2_link_nvfx(${target})
  vpvl2_link_regal(${target})
  vpvl2_link_atb(${target})
  vpvl2_link_hlslxc(${target})
  vpvl2_link_freeimage(${target})
  vpvl2_link_tbb(${target})
  vpvl2_link_bullet(${target})
  vpvl2_link_assimp(${target})
  vpvl2_link_icu(${target})
  vpvl2_link_glog(${target})
  vpvl2_link_zlib(${target})
  vpvl2_link_cg_runtime(${target})
  vpvl2_link_cl_runtime(${target})
  vpvl2_link_gl_runtime(${target})
endfunction()
# end of functions
