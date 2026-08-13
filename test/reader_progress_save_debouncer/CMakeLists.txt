add_executable(ReaderProgressSaveDebouncerTest
  ReaderProgressSaveDebouncerTest.cpp
)

target_include_directories(ReaderProgressSaveDebouncerTest PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/stubs
  ${REPO_ROOT}/src/activities/reader
)

target_link_libraries(ReaderProgressSaveDebouncerTest PRIVATE
  crosspoint_test_common
  GTest::gtest_main
)

gtest_discover_tests(ReaderProgressSaveDebouncerTest)
