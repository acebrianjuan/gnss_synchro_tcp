# Copyright (C) 2010-2015 (see AUTHORS file for a list of contributors)
#
# This file is part of GNSS-SDR.
#
# GNSS-SDR is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# GNSS-SDR is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNSS-SDR. If not, see <http://www.gnu.org/licenses/>.
#


########################################################################
# Project setup
########################################################################
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(WARNING "In-tree build is bad practice. Try 'cd build && cmake ../' ")
endif(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
cmake_minimum_required(VERSION 2.8)
set(GNSSSDR_MATIO_MIN_VERSION "1.5.3")
project(gnss_synchro_tcp CXX C)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)
file(RELATIVE_PATH RELATIVE_CMAKE_CALL ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})



########################################################################
# Determine optional blocks/libraries to be built (default: not built)
# Enable them here or at the command line by doing 'cmake -DENABLE_XXX=ON ../'
########################################################################


# Performance analysis tools
option(ENABLE_GPERFTOOLS "Enable linking to Gperftools libraries (tcmalloc and profiler)" OFF)
option(ENABLE_GPROF "Enable the use of the GNU profiler tool 'gprof'" OFF)

# Acceleration
option(ENABLE_PROFILING "Enable execution of volk_gnsssdr_profile at the end of the building" OFF)

# Building and packaging options
option(ENABLE_GENERIC_ARCH "Builds a portable binary" OFF)
option(ENABLE_PACKAGING "Enable software packaging" OFF)
option(ENABLE_OWN_GLOG "Download glog and link it to gflags" OFF)
option(ENABLE_LOG "Enable logging" ON)
if(ENABLE_PACKAGING)
    set(ENABLE_GENERIC_ARCH ON)
endif(ENABLE_PACKAGING)

# Testing
option(ENABLE_SYSTEM_TESTING "Builds system tests" OFF)
option(ENABLE_UNIT_TESTING "Builds unit tests" ON)

###############################
# GNSS-SDR version information
###############################
set(THIS_IS_A_RELEASE OFF)   # only related to version name, no further implications.
if(NOT ${THIS_IS_A_RELEASE})
    # Get the current working branch
    execute_process(
        COMMAND git rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Get the latest abbreviated commit hash of the working branch
    execute_process(
        COMMAND git log -1 --format=%h
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif(NOT ${THIS_IS_A_RELEASE})

set(VERSION_INFO_MAJOR_VERSION 0)
set(VERSION_INFO_API_COMPAT    0)
if(${THIS_IS_A_RELEASE})
    set(VERSION_INFO_MINOR_VERSION 8)
else(${THIS_IS_A_RELEASE})
    set(VERSION_INFO_MINOR_VERSION 8.git-${GIT_BRANCH}-${GIT_COMMIT_HASH})
endif(${THIS_IS_A_RELEASE})

set(VERSION ${VERSION_INFO_MAJOR_VERSION}.${VERSION_INFO_API_COMPAT}.${VERSION_INFO_MINOR_VERSION})


########################################################################
# Environment setup
########################################################################
include(ExternalProject)

# Detect 64-bits machine
if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
     set(ARCH_64BITS TRUE)
endif( CMAKE_SIZEOF_VOID_P EQUAL 8 )

set(OS_IS_MACOSX "")
set(OS_IS_LINUX "")

# Detect Linux Distribution
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
     set(OperatingSystem "Linux")
     set(OS_IS_LINUX TRUE)
     if(ARCH_64BITS)
         set(ARCH_ "(64 bits)")
     else(ARCH_64BITS)
         set(ARCH_ "(32 bits)")
     endif(ARCH_64BITS)
     if(EXISTS "/etc/lsb-release")
         execute_process(COMMAND cat /etc/lsb-release
                         COMMAND grep DISTRIB_ID
                         COMMAND awk -F= "{ print $2 }"
                         COMMAND tr "\n" " "
                         COMMAND sed "s/ //"
                         OUTPUT_VARIABLE LINUX_DISTRIBUTION
                         RESULT_VARIABLE LINUX_ID_RESULT
         )
         execute_process(COMMAND cat /etc/lsb-release
                         COMMAND grep DISTRIB_RELEASE
                         COMMAND awk -F= "{ print $2 }"
                         COMMAND tr "\n" " "
                         COMMAND sed "s/ //"
                         OUTPUT_VARIABLE LINUX_VER
                         RESULT_VARIABLE LINUX_VER_RESULT
         )
     endif(EXISTS "/etc/lsb-release")
     if(NOT LINUX_DISTRIBUTION)
         if(EXISTS "/etc/linuxmint/info")
             set(LINUX_DISTRIBUTION "LinuxMint")
             execute_process(COMMAND cat /etc/linuxmint/info
                             COMMAND grep -m1 RELEASE
                             COMMAND awk -F= "{ print $2 }"
                             COMMAND tr "\n" " "
                             COMMAND sed "s/ //"
                             OUTPUT_VARIABLE LINUX_VER
                             RESULT_VARIABLE LINUX_VER_RESULT
             )
         endif(EXISTS "/etc/linuxmint/info")
     endif(NOT LINUX_DISTRIBUTION)
     if(NOT LINUX_DISTRIBUTION)
         if(EXISTS "/etc/os-release")
             execute_process(COMMAND cat /etc/os-release
                             COMMAND grep -m1 NAME
                             COMMAND awk -F= "{ print $2 }"
                             COMMAND tr "\n" " "
                             COMMAND sed "s/ //"
                             OUTPUT_VARIABLE LINUX_DISTRIBUTION
                             RESULT_VARIABLE LINUX_ID_RESULT
             )
             execute_process(COMMAND cat /etc/os-release
                             COMMAND grep VERSION_ID
                             COMMAND awk -F= "{ print $2 }"
                             COMMAND tr "\n" " "
                             COMMAND sed "s/ //"
                             OUTPUT_VARIABLE LINUX_VER
                             RESULT_VARIABLE LINUX_VER_RESULT
             )
             if(${LINUX_DISTRIBUTION} MATCHES "Debian")
                 set(LINUX_DISTRIBUTION "Debian")
                 file(READ /etc/debian_version LINUX_VER)
             endif(${LINUX_DISTRIBUTION} MATCHES "Debian")
         endif(EXISTS "/etc/os-release")
     endif(NOT LINUX_DISTRIBUTION)
     if(NOT LINUX_DISTRIBUTION)
         if(EXISTS "/etc/redhat-release")
             set(LINUX_DISTRIBUTION "Red Hat")
             file (READ /etc/redhat-release LINUX_VER)
         endif(EXISTS "/etc/redhat-release")
     endif(NOT LINUX_DISTRIBUTION)
     if(NOT LINUX_DISTRIBUTION)
         if(EXISTS "/etc/debian_version")
             set(LINUX_DISTRIBUTION "Debian")
             file (READ /etc/debian_version LINUX_VER)
         endif(EXISTS "/etc/debian_version")
     endif(NOT LINUX_DISTRIBUTION)
     if(NOT LINUX_DISTRIBUTION)
         set(LINUX_DISTRIBUTION "Generic")
         set(LINUX_VER "Unknown")
     endif(NOT LINUX_DISTRIBUTION)
     message(STATUS "Configuring GNSS-SDR v${VERSION} to be built on ${LINUX_DISTRIBUTION} GNU/Linux Release ${LINUX_VER} ${ARCH_}")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

# Detect Mac OS X Version
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
     set(OperatingSystem "Mac OS X")
     set(OS_IS_MACOSX TRUE)
     exec_program(uname ARGS -v  OUTPUT_VARIABLE DARWIN_VERSION)
     string(REGEX MATCH "[0-9]+" DARWIN_VERSION ${DARWIN_VERSION})
     if(${DARWIN_VERSION} MATCHES "16")
         set(MACOS_SIERRA TRUE)
         set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++11")
         set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
         message(STATUS "Configuring GNSS-SDR v${VERSION} to be built on macOS Sierra 10.12")
     endif(${DARWIN_VERSION} MATCHES "16")
     if(${DARWIN_VERSION} MATCHES "15")
         set(MACOSX_EL_CAPITAN TRUE)
         set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++11")
         set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
         message(STATUS "Configuring GNSS-SDR v${VERSION} to be built on Mac OS X 10.11 El Capitan")
     endif(${DARWIN_VERSION} MATCHES "15")
     if(${DARWIN_VERSION} MATCHES "14")
         set(MACOSX_YOSEMITE TRUE)
         set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++11")
         set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
         message(STATUS "Configuring GNSS-SDR v${VERSION} to be built on Mac OS X 10.10 Yosemite")
     endif(${DARWIN_VERSION} MATCHES "14")
     if(${DARWIN_VERSION} MATCHES "13")
         set(MACOSX_MAVERICKS TRUE)
         set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++11")
         set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
         set(CMAKE_XCODE_ATTRIBUTE_GCC_VERSION="com.apple.compilers.llvm.clang.1_0")
         message(STATUS "Configuring GNSS-SDR v${VERSION} to be built on Mac OS X 10.9 Mavericks")
     endif(${DARWIN_VERSION} MATCHES "13")
     if(${DARWIN_VERSION} MATCHES "12")
         message(STATUS "Configuring GNSS-SDR v${VERSION} to be built on Mac OS X 10.8 Mountain Lion")
     endif(${DARWIN_VERSION} MATCHES "12")
     if(${DARWIN_VERSION} MATCHES "11")
         message(STATUS "Configuring GNSS-SDR v${VERSION} to be built on Mac OS X 10.7 Lion")
     endif(${DARWIN_VERSION} MATCHES "11")
     if(${DARWIN_VERSION} MATCHES "10")
         message(STATUS "Configuring GNSS-SDR v${VERSION} to be built on Mac OS X 10.6 Snow Leopard")
     endif(${DARWIN_VERSION} MATCHES "10")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")


#select the release build type by default to get optimization flags
if(NOT CMAKE_BUILD_TYPE)
     if(ENABLE_GPERFTOOLS OR ENABLE_GPROF)
         set(CMAKE_BUILD_TYPE "RelWithDebInfo")
         message(STATUS "Build type not specified: defaulting to RelWithDebInfo.")
     else(ENABLE_GPERFTOOLS OR ENABLE_GPROF)
         set(CMAKE_BUILD_TYPE "Release")
         message(STATUS "Build type not specified: defaulting to Release.")
     endif(ENABLE_GPERFTOOLS OR ENABLE_GPROF)
else(NOT CMAKE_BUILD_TYPE)
     message(STATUS "Build type set to ${CMAKE_BUILD_TYPE}.")
endif(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "")

# Append -O2 optimization flag for Debug builds
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O2")


################################################################################
# Checkout cmake version
################################################################################
if(CMAKE_VERSION VERSION_LESS 2.8.8)
      message(STATUS "Your CMake version is too old and does not support some features required by GNSS-SDR. CMake version must be at least 2.8.8. For more information check https://github.com/joakimkarlsson/bandit/issues/40")
      message(FATAL_ERROR "Fatal error: CMake >= 2.8.8 required.")
endif(CMAKE_VERSION VERSION_LESS 2.8.8)


################################################################################
# Checkout compiler version
################################################################################

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
      if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.7)
          message(STATUS "Your GCC version is too old and does not support some C++11 features required by GNSS-SDR. GCC version must be at least 4.7")
          if(${LINUX_DISTRIBUTION} MATCHES "Ubuntu")
              if(${LINUX_VER} MATCHES "12.04")
                  message(STATUS "For instructions on how to upgrade GCC, check http://askubuntu.com/a/271561")
              endif(${LINUX_VER} MATCHES "12.04")
          endif(${LINUX_DISTRIBUTION} MATCHES "Ubuntu")
          message(FATAL_ERROR "Fatal error: GCC >= 4.7 required.")
      endif(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.7)
endif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")


################################################################################
# Check if the compiler defines the architecture as ARM
################################################################################
if(NOT OS_IS_MACOSX)
     include(TestForARM)
endif(NOT OS_IS_MACOSX)


################################################################################
# Check for availability of SSE
################################################################################
if(NOT ENABLE_GENERIC_ARCH)
     include(TestForSSE)
endif(NOT ENABLE_GENERIC_ARCH)


################################################################################
# Googletest - https://github.com/google/googletest
################################################################################
enable_testing()
if(ENABLE_UNIT_TESTING OR ENABLE_SYSTEM_TESTING)
    if(EXISTS $ENV{GTEST_DIR})
        set(GTEST_DIR $ENV{GTEST_DIR})
    endif(EXISTS $ENV{GTEST_DIR})
    if(GTEST_DIR)
        message(STATUS "Googletest root folder set at ${GTEST_DIR}")
        find_path(LIBGTEST_DEV_DIR NAMES src/gtest-all.cc PATHS ${GTEST_DIR})
        if(LIBGTEST_DEV_DIR)
            message (STATUS "Googletest has been found.")
        else(LIBGTEST_DEV_DIR)
            message (FATAL_ERROR " Googletest source code has not been found at ${GTEST_DIR}.")
        endif(LIBGTEST_DEV_DIR)
        find_path(GTEST_INCLUDE_DIRS NAMES gtest/gtest.h PATHS ${GTEST_DIR}/include)
    else(GTEST_DIR)
        find_path(LIBGTEST_DEV_DIR NAMES src/gtest-all.cc PATHS /usr/src/gtest /opt/local/src/gtest-1.7.0)
        find_path(GTEST_INCLUDE_DIRS NAMES gtest/gtest.h PATHS /usr/include /opt/local/src/gtest-1.7.0/include)
        if(LIBGTEST_DEV_DIR)
            message (STATUS "Googletest (libgtest-dev package) has been found.")
        else(LIBGTEST_DEV_DIR)
            message (STATUS " Googletest has not been found.")
            message (STATUS " Googletest will be downloaded and built automatically ")
            message (STATUS " when doing 'make'. ")
        endif(LIBGTEST_DEV_DIR)
    endif(GTEST_DIR)
endif(ENABLE_UNIT_TESTING OR ENABLE_SYSTEM_TESTING)



################################################################################
# Boost - http://www.boost.org
################################################################################
if(UNIX AND EXISTS "/usr/lib64")
    list(APPEND BOOST_LIBRARYDIR "/usr/lib64") # Fedora 64-bit fix
endif(UNIX AND EXISTS "/usr/lib64")
set(Boost_ADDITIONAL_VERSIONS
    "1.45.0" "1.45" "1.46.0" "1.46"                 "1.48.0" "1.48" "1.49.0" "1.49"
    "1.50.0" "1.50" "1.51.0" "1.51"                 "1.53.0" "1.53" "1.54.0" "1.54"
    "1.55.0" "1.55" "1.56.0" "1.56" "1.57.0" "1.57" "1.58.0" "1.58" "1.59.0" "1.59"
    "1.60.0" "1.60" "1.61.0" "1.61" "1.62.0" "1.62" "1.63.0" "1.63" "1.64.0" "1.64"
    "1.65.0" "1.65" "1.66.0" "1.66" "1.67.0" "1.67" "1.68.0" "1.68" "1.69.0" "1.69"
    "1.70.0" "1.70" "1.71.0" "1.71" "1.72.0" "1.72" "1.73.0" "1.73" "1.74.0" "1.74"
)
set(Boost_USE_MULTITHREAD ON)
set(Boost_USE_STATIC_LIBS OFF)
find_package(Boost COMPONENTS date_time system filesystem thread serialization chrono REQUIRED)
if(NOT Boost_FOUND)
     message(FATAL_ERROR "Fatal error: Boost (version >=1.45.0) required.")
endif(NOT Boost_FOUND)



################################################################################
# gflags - https://github.com/gflags/gflags
################################################################################
set(LOCAL_GFLAGS false)
set(gflags_RELEASE 2.1.2)
find_package(GFlags)
if (NOT GFlags_FOUND)
     message (STATUS " gflags library has not been found.")
     message (STATUS " gflags will be downloaded and built automatically ")
     message (STATUS " when doing 'make'. ")

     ExternalProject_Add(
          gflags-${gflags_RELEASE}
          PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gflags-${gflags_RELEASE}
          GIT_REPOSITORY git://github.com/gflags/gflags.git
          GIT_TAG v${gflags_RELEASE}
          SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/gflags/gflags-${gflags_RELEASE}
          BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/gflags-${gflags_RELEASE}
          CMAKE_ARGS -DBUILD_SHARED_LIBS=ON -DBUILD_STATIC_LIBS=ON -DBUILD_gflags_nothreads_LIB=OFF -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
          BUILD_COMMAND make
          UPDATE_COMMAND ""
          PATCH_COMMAND ""
          INSTALL_COMMAND ""
     )

     set(GFlags_INCLUDE_DIRS
          ${CMAKE_CURRENT_BINARY_DIR}/gflags-${gflags_RELEASE}/include CACHE PATH "Local Gflags headers"
     )

     add_library(gflags UNKNOWN IMPORTED)
     set_property(TARGET gflags PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/gflags-${gflags_RELEASE}/lib/${CMAKE_FIND_LIBRARY_PREFIXES}gflags.a)
     add_dependencies(gflags gflags-${gflags_RELEASE})
     set(GFlags_LIBS gflags)
     file(GLOB GFlags_SHARED_LIBS "${CMAKE_CURRENT_BINARY_DIR}/gflags-${gflags_RELEASE}/lib/${CMAKE_FIND_LIBRARY_PREFIXES}gflags${CMAKE_SHARED_LIBRARY_SUFFIX}*")
     set(GFlags_LIBRARY gflags-${gflags_RELEASE})
     set(GFlags_LIBRARY_PATH ${CMAKE_CURRENT_BINARY_DIR}/gflags-${gflags_RELEASE}/lib )
     link_directories(${GFlags_LIBRARY_PATH})
     set(GFlags_lib ${GFlags_LIBS} CACHE FILEPATH "Local Gflags library")
     set(GFlags_LIBRARY_PATH ${GFlags_LIBS})
     set(LOCAL_GFLAGS true CACHE STRING "GFlags downloaded and built automatically" FORCE)
endif(NOT GFlags_FOUND)


################################################################################
# glog - https://github.com/google/glog
################################################################################
if(NOT ${ENABLE_OWN_GLOG})
     find_package(GLOG)
     if(GLOG_INCLUDE_DIRS)
        set(GLOG_FOUND ON)
     endif(GLOG_INCLUDE_DIRS)
endif(NOT ${ENABLE_OWN_GLOG})
set(glog_RELEASE 0.3.4)
if (NOT GLOG_FOUND OR ${LOCAL_GFLAGS})
     message (STATUS " glog library has not been found")
     if(NOT GFlags_FOUND)
          message(STATUS " or it is likely not linked to gflags.")
     endif(NOT GFlags_FOUND)
     message (STATUS " glog will be downloaded and built automatically ")
     message (STATUS " when doing 'make'. ")
     if(NOT ${LOCAL_GFLAGS})
         add_library(gflags-${gflags_RELEASE} UNKNOWN IMPORTED)
         set_property(TARGET gflags-${gflags_RELEASE} PROPERTY IMPORTED_LOCATION "${GFlags_LIBS}")
     endif(NOT ${LOCAL_GFLAGS})

     if(${LOCAL_GFLAGS})
         set(TARGET_GFLAGS ${gflags})
         if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
             file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/glog-${glog_RELEASE}/tmp/configure_with_gflags
"#!/bin/sh
export CPPFLAGS=-I${GFlags_INCLUDE_DIRS}
export LDFLAGS=-L${CMAKE_CURRENT_BINARY_DIR}/gflags-${gflags_RELEASE}/lib
export LIBS=${GFlags_SHARED_LIBS}
export CXXFLAGS=\"-stdlib=libc++\"
export CC=clang
export CXX=clang++
${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glog/glog-${glog_RELEASE}/configure")
             file(COPY ${CMAKE_CURRENT_BINARY_DIR}/glog-${glog_RELEASE}/tmp/configure_with_gflags
               DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/glog-${glog_RELEASE}
               FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ
                                GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
          else("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
             file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/glog-${glog_RELEASE}/tmp/configure_with_gflags
"#!/bin/sh
export CPPFLAGS=-I${GFlags_INCLUDE_DIRS}
export LDFLAGS=-L${CMAKE_CURRENT_BINARY_DIR}/gflags-${gflags_RELEASE}/lib
export LIBS=${GFlags_SHARED_LIBS}
${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glog/glog-${glog_RELEASE}/configure")
             file(COPY ${CMAKE_CURRENT_BINARY_DIR}/glog-${glog_RELEASE}/tmp/configure_with_gflags
               DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/glog-${glog_RELEASE}
               FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ
                                GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
         endif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")

     else(${LOCAL_GFLAGS})
         set(TARGET_GFLAGS gflags-${gflags_RELEASE})
         if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
              file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/glog-${glog_RELEASE}/tmp/configure_with_gflags
"#!/bin/sh
export CPPFLAGS=-I${GFlags_INCLUDE_DIRS}
export LDFLAGS=-L${GFlags_LIBRARY_DIRS}
export LIBS=\"${GFlags_LIBS} -lc++\"
export CXXFLAGS=\"-stdlib=libc++\"
export CC=clang
export CXX=clang++
${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glog/glog-${glog_RELEASE}/configure")
              file(COPY ${CMAKE_CURRENT_BINARY_DIR}/glog-${glog_RELEASE}/tmp/configure_with_gflags
                DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/glog-${glog_RELEASE}
                FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ
                                GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
         else("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
              file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/glog-${glog_RELEASE}/tmp/configure_with_gflags
"#!/bin/sh
export CPPFLAGS=-I${GFlags_INCLUDE_DIRS}
export LDFLAGS=-L${CMAKE_CURRENT_BINARY_DIR}/gflags-${gflags_RELEASE}/lib
export LIBS=${GFlags_SHARED_LIBS}
${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glog/glog-${glog_RELEASE}/configure")
              file(COPY ${CMAKE_CURRENT_BINARY_DIR}/glog-${glog_RELEASE}/tmp/configure_with_gflags
                DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/glog-${glog_RELEASE}
                FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ
                                GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
         endif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
     endif(${LOCAL_GFLAGS})

     set(GLOG_CONFIGURE ${CMAKE_CURRENT_BINARY_DIR}/glog-${glog_RELEASE}/configure_with_gflags)

     ExternalProject_Add(
         glog-${glog_RELEASE}
         DEPENDS ${TARGET_GFLAGS}
         PREFIX ${CMAKE_CURRENT_BINARY_DIR}/glog-${glog_RELEASE}
         GIT_REPOSITORY https://github.com/google/glog/
         GIT_TAG v${glog_RELEASE}
         SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glog/glog-${glog_RELEASE}
         BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/glog-${glog_RELEASE}
         CONFIGURE_COMMAND ${GLOG_CONFIGURE} --prefix=<INSTALL_DIR>
         BUILD_COMMAND make
         UPDATE_COMMAND ""
         PATCH_COMMAND ""
         INSTALL_COMMAND ""
     )

     # Set up variables
     set(GLOG_INCLUDE_DIRS
          ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glog/glog-${glog_RELEASE}/src/
          ${CMAKE_CURRENT_BINARY_DIR}/glog-${glog_RELEASE}/src
     )
     set(GLOG_LIBRARIES
          ${CMAKE_CURRENT_BINARY_DIR}/glog-${glog_RELEASE}/.libs/${CMAKE_FIND_LIBRARY_PREFIXES}glog.a
     )
     set(LOCAL_GLOG true CACHE STRING "Glog downloaded and built automatically" FORCE)
else(NOT GLOG_FOUND OR ${LOCAL_GFLAGS})
     add_library(glog-${glog_RELEASE} UNKNOWN IMPORTED)
     set_property(TARGET glog-${glog_RELEASE} PROPERTY IMPORTED_LOCATION "${GLOG_LIBRARIES}")
endif(NOT GLOG_FOUND OR ${LOCAL_GFLAGS})

if(NOT ENABLE_LOG)
     message(STATUS "Logging is not enabled")
     add_definitions(-DGOOGLE_STRIP_LOG=1)
endif(NOT ENABLE_LOG)



################################################################################
# Armadillo - http://arma.sourceforge.net/
################################################################################
if(OS_IS_LINUX)
    #############################################################################
    # Check that LAPACK is found in the system
    # LAPACK is required for matrix decompositions (eg. SVD) and matrix inverse.
    #############################################################################
    find_library(LAPACK lapack)
    if(NOT LAPACK)
        message(" The LAPACK library has not been found.")
        message(" You can try to install it by typing:")
        if(${LINUX_DISTRIBUTION} MATCHES "Fedora" OR ${LINUX_DISTRIBUTION} MATCHES "Red Hat")
            message(" sudo yum install lapack-devel")
        elseif(${LINUX_DISTRIBUTION} MATCHES "openSUSE")
            message(" sudo zypper install lapack-devel")
        else(${LINUX_DISTRIBUTION} MATCHES "Fedora" OR ${LINUX_DISTRIBUTION} MATCHES "Red Hat")
            message(" sudo apt-get install liblapack-dev")
        endif(${LINUX_DISTRIBUTION} MATCHES "Fedora" OR ${LINUX_DISTRIBUTION} MATCHES "Red Hat")
        message(FATAL_ERROR "LAPACK is required to build gnss-sdr")
    endif(NOT LAPACK)
    #############################################################################
    # Check that BLAS is found in the system
    # BLAS is used for matrix multiplication.
    # Without BLAS, matrix multiplication will still work, but might be slower.
    #############################################################################
    find_library(BLAS blas)
    if(NOT BLAS)
        message(" The BLAS library has not been found.")
        message(" You can try to install it by typing:")
        if(${LINUX_DISTRIBUTION} MATCHES "Fedora" OR ${LINUX_DISTRIBUTION} MATCHES "Red Hat")
            message(" sudo yum install blas-devel")
        else(${LINUX_DISTRIBUTION} MATCHES "Fedora" OR ${LINUX_DISTRIBUTION} MATCHES "Red Hat")
            message(" sudo apt-get install libopenblas-dev")
        endif(${LINUX_DISTRIBUTION} MATCHES "Fedora" OR ${LINUX_DISTRIBUTION} MATCHES "Red Hat")
        message(FATAL_ERROR "BLAS is required to build gnss-sdr")
    endif(NOT BLAS)
    #############################################
    # Check if GFORTRAN is found in the system
    #############################################
    find_library(GFORTRAN NAMES gfortran
                 PATHS /usr/lib
                       /usr/lib64
                       /usr/local/lib
                       /usr/local/lib/i386
                       /usr/lib/gcc/x86_64-linux-gnu
                       /usr/lib/gcc/i686-linux-gnu
                       /usr/lib/gcc/i386-linux-gnu
                       /usr/lib/gcc/x86_64-linux-gnu/4.6 # Ubuntu 12.04
                       /usr/lib/gcc/i686-linux-gnu/4.6
                       /usr/lib/gcc/x86_64-linux-gnu/4.7
                       /usr/lib/gcc/i686-linux-gnu/4.7
                       /usr/lib/gcc/x86_64-linux-gnu/4.8
                       /usr/lib/gcc/i686-linux-gnu/4.8
                       /usr/lib/gcc/x86_64-linux-gnu/4.9
                       /usr/lib/gcc/i686-linux-gnu/4.9
                       /usr/lib/gcc/x86_64-redhat-linux/4.7.2 # Fedora 18
                       /usr/lib/gcc/i686-redhat-linux/4.7.2
                       /usr/lib/gcc/x86_64-redhat-linux/4.8.1 # Fedora 19
                       /usr/lib/gcc/x86_64-redhat-linux/4.8.3 # Fedora 20
                       /usr/lib/gcc/x86_64-redhat-linux/4.9.1 # Fedora 21
                       /usr/lib/gcc/i686-redhat-linux/4.8.1
                       /usr/lib/gcc/i686-redhat-linux/4.8.3
                       /usr/lib/gcc/i686-redhat-linux/4.9.1
                       /usr/lib/gcc/x86_64-redhat-linux/4.4.4 # CentOS 6
                       /usr/lib/gcc/i686-redhat-linux/4.4.4
                       /usr/lib/gcc/x86_64-redhat-linux/4.8.2
                       /usr/lib/gcc/i686-redhat-linux/4.8.2
                       /usr/lib/gcc/i586-suse-linux/4.8  # OpenSUSE 13.1
                       /usr/lib/gcc/i586-suse-linux/4.9
                       /usr/lib/gcc/x86_64-suse-linux/4.8
                       /usr/lib/gcc/x86_64-suse-linux/4.9
                       /usr/lib/gcc/i486-linux-gnu # Debian 7
                       /usr/lib/gcc/i486-linux-gnu/4.4
                       /usr/lib/gcc/i486-linux-gnu/4.6
                       /usr/lib/gcc/i486-linux-gnu/4.7
                       /usr/lib/gcc/i486-linux-gnu/4.8
                       /usr/lib/gcc/i486-linux-gnu/4.9
                       /usr/lib/gcc/i586-linux-gnu/4.9
                       /usr/lib/gcc/arm-linux-gnueabihf/4.4 # Debian armhf
                       /usr/lib/gcc/arm-linux-gnueabihf/4.5
                       /usr/lib/gcc/arm-linux-gnueabihf/4.6
                       /usr/lib/gcc/arm-linux-gnueabihf/4.7
                       /usr/lib/gcc/arm-linux-gnueabihf/4.8
                       /usr/lib/gcc/arm-linux-gnueabihf/4.9
                       /usr/lib/gcc/aarch64-linux-gnu/4.9   # Debian arm64
                       /usr/lib/gcc/arm-linux-gnueabi/4.7   # Debian armel
                       /usr/lib/gcc/arm-linux-gnueabi/4.9
                       /usr/lib/gcc/x86_64-linux-gnu/5
                       /usr/lib/gcc/i686-linux-gnu/5
                       /usr/lib/gcc/arm-linux-gnueabi/5
                       /usr/lib/gcc/arm-linux-gnueabihf/5
                       /usr/lib/gcc/aarch64-linux-gnu/5
                )
    if(NOT GFORTRAN)
        message(STATUS "The gfortran library has not been found.")
        message(STATUS " You can try to install it by typing:")
        if(${LINUX_DISTRIBUTION} MATCHES "Fedora" OR ${LINUX_DISTRIBUTION} MATCHES "Red Hat")
            message(STATUS " sudo yum install gcc-fortran")
        elseif(${LINUX_DISTRIBUTION} MATCHES "openSUSE")
            message(STATUS " sudo zypper install gcc-fortran")
        else(${LINUX_DISTRIBUTION} MATCHES "Fedora" OR ${LINUX_DISTRIBUTION} MATCHES "Red Hat")
            message(STATUS " sudo apt-get install gfortran")
        endif(${LINUX_DISTRIBUTION} MATCHES "Fedora" OR ${LINUX_DISTRIBUTION} MATCHES "Red Hat")
    endif(NOT GFORTRAN)
endif(OS_IS_LINUX)

find_package(Armadillo)
if(NOT ARMADILLO_FOUND)
     message(STATUS " Armadillo has not been found.")
     message(STATUS " Armadillo will be downloaded and built automatically ")
     message(STATUS " when doing 'make'. ")

     if(CMAKE_VERSION VERSION_LESS 3.1)
         # ExternalProject in CMake > 3.1 cannot open .xz files
         set(armadillo_RELEASE 6.700.7)
         set(armadillo_MD5 "8116185e1d7391eed3bf6c500f81b4d8")
         set(ARMA_FILE_EXTENSION "gz")
     else(CMAKE_VERSION VERSION_LESS 3.1)
         set(armadillo_RELEASE 7.400.1)
         set(armadillo_MD5 "27fb0b62c02bf3f9ace1edd6e100dc78")
         set(ARMA_FILE_EXTENSION "xz")
     endif(CMAKE_VERSION VERSION_LESS 3.1)

     ExternalProject_Add(
         armadillo-${armadillo_RELEASE}
         PREFIX ${CMAKE_CURRENT_BINARY_DIR}/armadillo-${armadillo_RELEASE}
         URL http://sourceforge.net/projects/arma/files/armadillo-${armadillo_RELEASE}.tar.${ARMA_FILE_EXTENSION}
         DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}/download/armadillo-${armadillo_RELEASE}
         URL_MD5 ${armadillo_MD5}
         CMAKE_ARGS -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} -DBUILD_SHARED_LIBS=OFF
         BUILD_IN_SOURCE 1
         BUILD_COMMAND make
         UPDATE_COMMAND ""
         INSTALL_COMMAND ""
     )

     # Set up variables
     ExternalProject_Get_Property(armadillo-${armadillo_RELEASE} binary_dir)
     set(ARMADILLO_INCLUDE_DIRS ${binary_dir}/include )
     find_library(LAPACK NAMES lapack HINTS /usr/lib /usr/local/lib /usr/lib64)
     if(OS_IS_MACOSX)
         find_library(BLAS blas)
     endif(OS_IS_MACOSX)
     find_package(OpenBLAS)
     if(OPENBLAS_FOUND)
         set(BLAS ${OPENBLAS})
     endif(OPENBLAS_FOUND)
     if(NOT GFORTRAN)
         set(GFORTRAN "")
     endif(NOT GFORTRAN)
     set(ARMADILLO_LIBRARIES ${BLAS} ${LAPACK} ${GFORTRAN} ${binary_dir}/${CMAKE_FIND_LIBRARY_PREFIXES}armadillo.a)
     set(LOCAL_ARMADILLO true CACHE STRING "Armadillo downloaded and built automatically" FORCE)
     # Save a copy at the thirdparty folder
     file(COPY ${CMAKE_CURRENT_BINARY_DIR}/armadillo-${armadillo_RELEASE}
          DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/armadillo
     )
else(NOT ARMADILLO_FOUND)
     set(armadillo_RELEASE ${ARMADILLO_VERSION_STRING})
     add_library(armadillo-${armadillo_RELEASE} UNKNOWN IMPORTED)
     set_property(TARGET armadillo-${armadillo_RELEASE} PROPERTY IMPORTED_LOCATION "${ARMADILLO_LIBRARIES}")
endif(NOT ARMADILLO_FOUND)


########################################################################
# Set compiler flags
########################################################################

# Enable C++11 support in GCC
# For "-std=c++0x"  GCC's support for C++11 see http://gcc.gnu.org/projects/cxx0x.html
if(CMAKE_COMPILER_IS_GNUCXX AND NOT WIN32)
     set(MY_CXX_FLAGS "${MY_CXX_FLAGS} -std=c++11 -Wall -Wextra")  #Add warning flags: For "-Wall" see http://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
endif(CMAKE_COMPILER_IS_GNUCXX AND NOT WIN32)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(MY_CXX_FLAGS "${MY_CXX_FLAGS} -std=c++11 -stdlib=libc++ -Wno-c++11-narrowing")
    if(CMAKE_BUILD_TYPE MATCHES "Release")
        set(MY_CXX_FLAGS "${MY_CXX_FLAGS} -Wno-unused-private-field")
    endif(CMAKE_BUILD_TYPE MATCHES "Release")
endif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")

# Processor-architecture related flags
# See http://gcc.gnu.org/onlinedocs/gcc/i386-and-x86_002d64-Options.html#i386-and-x86_002d64-Options
if (NOT ARCH_COMPILER_FLAGS)
  if(CMAKE_COMPILER_IS_GNUCXX AND NOT WIN32)
    if(OS_IS_MACOSX)
      set(ARCH_COMPILER_FLAGS "-march=corei7 -mfpmath=sse")
    else(OS_IS_MACOSX)
      if(NOT ${ENABLE_GENERIC_ARCH})
         if(IS_ARM) # ARM-specific options (https://gcc.gnu.org/onlinedocs/gcc/ARM-Options.html)
            if(ARM_VERSION STREQUAL "arm")
               # Unknown arm version - try our best to detect
               set(ARCH_COMPILER_FLAGS "-mcpu=native")
            else(ARM_VERSION STREQUAL "arm")
               set(ARCH_COMPILER_FLAGS "-march=${ARM_VERSION}")
            endif(ARM_VERSION STREQUAL "arm")
         else(IS_ARM)
            set(ARCH_COMPILER_FLAGS "-march=native -mfpmath=sse")
         endif(IS_ARM)
      endif(NOT ${ENABLE_GENERIC_ARCH})
    endif(OS_IS_MACOSX)
  endif(CMAKE_COMPILER_IS_GNUCXX AND NOT WIN32)
endif (NOT ARCH_COMPILER_FLAGS)
set(MY_CXX_FLAGS "${MY_CXX_FLAGS} ${ARCH_COMPILER_FLAGS}")

if(CMAKE_COMPILER_IS_GNUCXX AND NOT WIN32)
    #http://gcc.gnu.org/wiki/Visibility
    add_definitions(-fvisibility=hidden)
endif()

if(ENABLE_GPERFTOOLS)
    # Set GPerftools related flags if it is available
    # See http://gperftools.googlecode.com/svn/trunk/README
    if(GPERFTOOLS_FOUND)
        if(CMAKE_COMPILER_IS_GNUCXX AND NOT WIN32)
            set(MY_CXX_FLAGS "${MY_CXX_FLAGS} -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free")
        endif(CMAKE_COMPILER_IS_GNUCXX AND NOT WIN32)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            set(MY_CXX_FLAGS "${MY_CXX_FLAGS} -fno-builtin")
        endif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    endif(GPERFTOOLS_FOUND)
endif(ENABLE_GPERFTOOLS)

if(ENABLE_GPROF)
    #if(CMAKE_COMPILER_IS_GNUCXX)
        set(MY_CXX_FLAGS "${MY_CXX_FLAGS} -pg")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")
    #endif(CMAKE_COMPILER_IS_GNUCXX)
endif(ENABLE_GPROF)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MY_CXX_FLAGS}")

if(OS_IS_LINUX)
    if(${LINUX_DISTRIBUTION} MATCHES "Fedora" OR ${LINUX_DISTRIBUTION} MATCHES "openSUSE" OR ${LINUX_DISTRIBUTION} MATCHES "ArchLinux")
        link_libraries(pthread)
    endif(${LINUX_DISTRIBUTION} MATCHES "Fedora" OR ${LINUX_DISTRIBUTION} MATCHES "openSUSE" OR ${LINUX_DISTRIBUTION} MATCHES "ArchLinux")
endif(OS_IS_LINUX)



include_directories(
        ${CMAKE_SOURCE_DIR}
        ${Boost_INCLUDE_DIRS}
	${GLog_INCLUDE_DIRS}
     )
     
ADD_EXECUTABLE(tcp_client ${CMAKE_SOURCE_DIR}/client.cc)
target_link_libraries(tcp_client pthread ${Boost_LIBRARIES} ${GLog_LIBRARIES}) 

ADD_EXECUTABLE(tcp_server ${CMAKE_SOURCE_DIR}/server.cc)
target_link_libraries(tcp_server pthread ${Boost_LIBRARIES} ${GLog_LIBRARIES}) 
