# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/brett/Documents/code/c++/image-gp-6/cmake-build-relwithdebinfo/_deps/threatexchange-src"
  "/home/brett/Documents/code/c++/image-gp-6/cmake-build-relwithdebinfo/_deps/threatexchange-build"
  "/home/brett/Documents/code/c++/image-gp-6/cmake-build-relwithdebinfo/_deps/threatexchange-subbuild/threatexchange-populate-prefix"
  "/home/brett/Documents/code/c++/image-gp-6/cmake-build-relwithdebinfo/_deps/threatexchange-subbuild/threatexchange-populate-prefix/tmp"
  "/home/brett/Documents/code/c++/image-gp-6/cmake-build-relwithdebinfo/_deps/threatexchange-subbuild/threatexchange-populate-prefix/src/threatexchange-populate-stamp"
  "/home/brett/Documents/code/c++/image-gp-6/cmake-build-relwithdebinfo/_deps/threatexchange-subbuild/threatexchange-populate-prefix/src"
  "/home/brett/Documents/code/c++/image-gp-6/cmake-build-relwithdebinfo/_deps/threatexchange-subbuild/threatexchange-populate-prefix/src/threatexchange-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/brett/Documents/code/c++/image-gp-6/cmake-build-relwithdebinfo/_deps/threatexchange-subbuild/threatexchange-populate-prefix/src/threatexchange-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/brett/Documents/code/c++/image-gp-6/cmake-build-relwithdebinfo/_deps/threatexchange-subbuild/threatexchange-populate-prefix/src/threatexchange-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
