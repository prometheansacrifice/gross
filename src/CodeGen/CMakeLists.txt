
set(_SOURCE_FILES
    BasicBlock.cpp
    DLXNodeUtils.cpp
    PreMachineLowering.cpp
    GraphScheduling.cpp
    PostMachineLowering.cpp
    RegisterAllocator.cpp
    PostRALowering.cpp
    )

add_library(GrossCodeGen OBJECT
  ${_SOURCE_FILES})

if(GROSS_ENABLE_UNIT_TESTS)
  set(_TEST_SOURCE_FILES
      GraphSchedulingTest.cpp
      PreMachineLoweringTest.cpp
      PostMachineLoweringTest.cpp
      RegisterAllocatorTest.cpp
      )

  add_executable(GrossCodeGenTest
    ${_TEST_SOURCE_FILES})
  target_link_libraries(GrossCodeGenTest
    $<TARGET_OBJECTS:GrossGraph>
    $<TARGET_OBJECTS:GrossCodeGen>
    gtest_main)
  gtest_add_tests(TARGET GrossCodeGenTest)
  add_dependencies(unittests GrossCodeGenTest)
endif()
