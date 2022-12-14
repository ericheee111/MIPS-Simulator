cmake_minimum_required(VERSION 3.5...3.19)
project(Milestone5 CXX)

# add source for any library modules here
set(LIB_SOURCE
  token.hpp token.cpp
  lexer.hpp lexer.cpp
  parser.hpp parser.cpp
  thread.hpp thread.cpp
)

# add source for any additional unit tests here
set(LIB_TEST_SOURCE
  test_lexer.cpp
  test_parser.cpp
)

# add source for any additional gui modules here
set(UI_SOURCE
  virtual_machine_gui.hpp virtual_machine_gui.cpp)

# add source for UI tests here
set(UI_TEST_SOURCE
  virtual_machine_gui_test.cpp
  )

# add source for simmips program here
set(SIMMIPS_SOURCE simmips.cpp)

# You should not need to edit below this line
#-----------------------------------------------------------------------
#-----------------------------------------------------------------------

set(UI_TEST_SOURCE
  virtual_machine_gui_test.cpp)

# try to prevent accidental in-source builds, these cause lots of problems
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "In-source builds not allowed. Remove any files created thus far and use a different directory for the build.")
endif()

# require a C++11 compiler for all targets
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# configure Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
find_package(Qt5 COMPONENTS Widgets Test REQUIRED)

# optional strict mode
if(UNIX AND STRICT)
  message("-- Enabling strict compilation mode")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")
endif()

# setup testing
set(TEST_FILE_DIR "${CMAKE_SOURCE_DIR}/tests")
configure_file(${CMAKE_SOURCE_DIR}/test_config.hpp.in 
  ${CMAKE_BINARY_DIR}/test_config.hpp)
configure_file(${CMAKE_SOURCE_DIR}/test_config.py.in 
  ${CMAKE_BINARY_DIR}/test_config.py)
include_directories(${CMAKE_BINARY_DIR})

# executables
add_executable(unit_tests catch.hpp unit_tests.cpp ${LIB_TEST_SOURCE} ${LIB_SOURCE})
if(UNIX AND NOT APPLE AND CMAKE_COMPILER_IS_GNUCXX)
  target_link_libraries(unit_tests pthread)
endif()

add_executable(virtual_machine_gui_test ${UI_TEST_SOURCE} ${UI_SOURCE} ${LIB_SOURCE})
if(UNIX AND NOT APPLE AND CMAKE_COMPILER_IS_GNUCXX)
  target_link_libraries(virtual_machine_gui_test Qt5::Widgets Qt5::Test pthread)
else(UNIX AND NOT APPLE AND CMAKE_COMPILER_IS_GNUCXX)
  target_link_libraries(virtual_machine_gui_test Qt5::Widgets Qt5::Test)
endif()

add_executable(simmips ${SIMMIPS_SOURCE} ${UI_SOURCE} ${LIB_SOURCE})
if(UNIX AND NOT APPLE AND CMAKE_COMPILER_IS_GNUCXX)
  target_link_libraries(simmips Qt5::Widgets pthread)
else(UNIX AND NOT APPLE AND CMAKE_COMPILER_IS_GNUCXX)
  target_link_libraries(simmips Qt5::Widgets)
endif()



# Tests
#----------------------------------------------------------------------
enable_testing()

# unit tests
add_test(unit_tests unit_tests)
add_test(ui_tests virtual_machine_gui_test)

# In the reference environment enable coverage on tests and threading
if(UNIX AND NOT APPLE AND CMAKE_COMPILER_IS_GNUCXX)
  message("-- Enabling test coverage")
  set(GCC_COVERAGE_COMPILE_FLAGS "-g -O0 -fprofile-arcs -ftest-coverage")
  set_target_properties(unit_tests PROPERTIES COMPILE_FLAGS ${GCC_COVERAGE_COMPILE_FLAGS} )
  set_target_properties(virtual_machine_gui_test PROPERTIES COMPILE_FLAGS ${GCC_COVERAGE_COMPILE_FLAGS} )
  target_link_libraries(unit_tests gcov)
  target_link_libraries(virtual_machine_gui_test gcov)
  add_custom_target(coverage
    COMMAND ${CMAKE_COMMAND} -E env "ROOT=${CMAKE_CURRENT_SOURCE_DIR}"
    ${CMAKE_CURRENT_SOURCE_DIR}/scripts/coverage.sh)
endif()

# In the reference environment enable memory checking on tests
if(UNIX AND NOT APPLE AND CMAKE_COMPILER_IS_GNUCXX)
  message("-- Enabling memory checks")
  add_custom_target(memtest
    COMMAND valgrind ${CMAKE_BINARY_DIR}/unit_tests)
endif()
