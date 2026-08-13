add_executable(TextPoolTest
  TextPoolTest.cpp
)

target_link_libraries(TextPoolTest PRIVATE
  crosspoint_test_common
  GTest::gtest_main
)

target_include_directories(TextPoolTest PRIVATE
  ${REPO_ROOT}/src
)

gtest_discover_tests(TextPoolTest)
