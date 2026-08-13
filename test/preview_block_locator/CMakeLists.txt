find_package(EXPAT REQUIRED)

add_executable(PreviewBlockLocatorTest
  PreviewBlockLocatorTest.cpp
  ${REPO_ROOT}/lib/Epub/Epub/parsers/PreviewBlockLocator.cpp
)

target_include_directories(PreviewBlockLocatorTest PRIVATE
  ${REPO_ROOT}/lib/Epub/Epub/parsers
)

target_link_libraries(PreviewBlockLocatorTest PRIVATE
  crosspoint_test_common
  EXPAT::EXPAT
  GTest::gtest_main
)

gtest_discover_tests(PreviewBlockLocatorTest)
