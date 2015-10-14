#================================================================
# cmake utilities to build numerics component
#================================================================

set(COMPONENT externals)
message("-- Set up for ${PROJECT_NAME}_${COMPONENT} library ...\n")
# Name used for the target to be built
set(COMPONENT_LIBRARY_NAME ${PROJECT_NAME}_${COMPONENT})

# Define a Fortran interface file (FCMangle.hpp)
#FortranCInterface_VERIFY(QUIET)
if(NOT FortranCInterface_VERIFIED_C)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/DefaultMangle.h.in ${CMAKE_CURRENT_BINARY_DIR}/FCMangle.h)
else()
  FortranCInterface_HEADER(${CMAKE_CURRENT_BINARY_DIR}/FCMangle.h
    MACRO_NAMESPACE "myF2C" 
    SYMBOL_NAMESPACE "myF2C"
    )
endif()

# --- Sources and headers ---
# source directories for numerics
set(${COMPONENT}_DIRS
  tools
  blas_lapack
  hairer
  netlib/odepack
  netlib/dftemplates
  SOL/lumod-c
)

# Unstable sources. Will be included only
# if WITH_${COMPONENT}_UNSTABLE is true.
set(${COMPONENT}_Unstable_SRCS)

# List of directories of headers not to be installed
set(${COMPONENT}_HDRS_EXCLUDE_DIR SOL/lumod-c)

# List of directories for which no doxygen doc will be generated
# By default all directories matching "test" are excluded.
set(${COMPONENT}_EXCLUDE_DOXY
  netlib
  hairer)

# ---- Final setup for the library ----
# --- set linked libraries and linker language ---
set(${COMPONENT}_LINKER_LANGUAGE C)
# but CXX may be needed with MlcpSimplex
if(MLCPSIMPLEX_FOUND) 
  set(${COMPONENT}_LINKER_LANGUAGE CXX)
endif()
set(${COMPONENT}_LINK_LIBRARIES ${SICONOS_LINK_LIBRARIES})
include(LibraryProjectSetup)
library_project_setup()

if(BUILD_AS_CPP)
  file(GLOB_RECURSE C_FILES ${CMAKE_CURRENT_SOURCE_DIR} *.c)
  set_source_files_properties(${C_FILES} PROPERTIES LANGUAGE CXX)
endif()

# --- specific install ----
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/FCMangle.h DESTINATION include/${PROJECT_NAME})

# --- tests ---
include(${COMPONENT}_tests.cmake)