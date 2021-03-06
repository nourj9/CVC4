include_directories(.)
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/src/include)
include_directories(${CMAKE_BINARY_DIR}/src)

#-----------------------------------------------------------------------------#
# Add target 'systemtests', builds and runs
# > system tests

add_custom_target(build-systemtests)
add_dependencies(build-tests build-systemtests)

add_custom_target(systemtests
  COMMAND ctest --output-on-failure -L "system" -j${CTEST_NTHREADS} $$ARGS
  DEPENDS build-systemtests)

set(CVC4_SYSTEM_TEST_FLAGS
  -D__BUILDING_CVC4_SYSTEM_TEST -D__STDC_LIMIT_MACROS -D__STDC_FORMAT_MACROS)

macro(cvc4_add_system_test name)
  add_executable(${name} ${name}.cpp)
  target_link_libraries(${name} main-test)
  target_compile_definitions(${name} PRIVATE ${CVC4_SYSTEM_TEST_FLAGS})
  add_test(system/${name} ${CMAKE_CURRENT_BINARY_DIR}/${name})
  set_tests_properties(system/${name} PROPERTIES LABELS "system")
  add_dependencies(build-systemtests ${name})
endmacro()

cvc4_add_system_test(boilerplate)
cvc4_add_system_test(ouroborous)
cvc4_add_system_test(reset_assertions)
cvc4_add_system_test(sep_log_api)
cvc4_add_system_test(smt2_compliance)
# TODO(cvc4-projects#209): Add methods for retrieving statistics to new API
# cvc4_add_system_test(statistics)
cvc4_add_system_test(two_solvers)

# if we've built using libedit, then we want the interactive shell tests
if (USE_EDITLINE)

  # Check for pexpect -- zero return code is success
  execute_process(
    COMMAND ${PYTHON_EXECUTABLE} -c "import pexpect"
    RESULT_VARIABLE PEXPECT_RC
    ERROR_QUIET
  )

  # Add the test if we have pexpect
  if(PEXPECT_RC EQUAL 0)
    add_test(
      NAME interactive_shell
      COMMAND
      "${PYTHON_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/test/system/interactive_shell.py"
      WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    )
  endif()
endif()

# TODO: Move CVC4JavaTest.java to test/java and delete run_java_test (after full cmake migration)
