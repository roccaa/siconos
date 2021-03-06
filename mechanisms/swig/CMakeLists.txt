#==================================================================
# cmake utilities to build python-bindings for mechanisms component
#==================================================================

# wrap - Set extra dependencies
set(SWIG_MODULE_${COMPONENT}_EXTRA_DEPS)

# DEFS
set(${COMPONENT}_SWIG_DEFS "${SWIG_DEFS}")

# List of modules to be built in the python bindings
set(${COMPONENT}_PYTHON_MODULES
  "mechanisms/cadmbtb"
  "mechanisms/mbtb")

set(${COMPONENT}_SWIG_INCLUDE_DIRECTORIES
  ${CMAKE_SOURCE_DIR}/kernel/swig)

include(swig_python_tools)

swig_module_setup(${COMPONENT}_PYTHON_MODULES)

build_python_tests()


# we need siconos-mechanism for build and install tree with different 'PYTHONPATH' for each of them.

# -- prepare siconos_mechanisms tests --
set(CURRENT_PYTHON_PATH ${CMAKE_BINARY_DIR}/wrap)
set(EXAMPLE_PATH ${SICONOS_SWIG_ROOT_DIR}/tests/) # @EXAMPLE_PATH@ is required in siconos_mechanisms script
# Each file is copy to siconos/tests.
# Maybe we can create a 'tests' dir for each subpackage?
# --> Easier to deal with plugins and data if only one package
configure_file(siconos_mechanisms.py.in  ${SICONOS_SWIG_ROOT_DIR}/tests/siconos_mechanisms.py)
set(exename ${SICONOS_SWIG_ROOT_DIR}/tests/test_siconos_mechanisms.py)
add_python_test("python_test_siconos_mechanisms" ${exename})

# data (CAD and body) taken from slider crank example in mechanisms component
set(path_to_test_sources ${CMAKE_SOURCE_DIR}/mechanisms/swig/tests/slider_crank)
configure_file(${path_to_test_sources}/bodydef.py ${SICONOS_SWIG_ROOT_DIR}/tests/ COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tests/mbtbLocalOptions.py ${SICONOS_SWIG_ROOT_DIR}/tests/ COPYONLY)
file(GLOB cad4tests RELATIVE ${path_to_test_sources}/CAD ${path_to_test_sources}/CAD/*)
foreach(datafile ${cad4tests})
  configure_file(${path_to_test_sources}/CAD/${datafile}
    ${SICONOS_SWIG_ROOT_DIR}/tests/CAD/${datafile} COPYONLY)
endforeach()

# -- install the script --
set(CURRENT_PYTHON_PATH ${SICONOS_PYTHON_INSTALL_DIR})
set(EXAMPLE_PATH ./) # @EXAMPLE_PATH@ is required in siconos_mechanisms script
configure_file(siconos_mechanisms.py.in ${CMAKE_BINARY_DIR}/scripts/siconos_mechanisms.py)
configure_file(mechanisms/mbtbDefaultOptions.py ${SICONOS_SWIG_ROOT_DIR}/mechanisms/mbtbDefaultOptions.py)
install(PROGRAMS ${CMAKE_BINARY_DIR}/scripts/siconos_mechanisms.py DESTINATION bin RENAME siconos_mechanisms)

