find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_test(NAME AppStoreWifiBehavior
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/run_test.py)
