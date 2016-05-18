project(SimpleTest)

find_package( OpenCV REQUIRED )
include_directories ("${SimpleTest_SOURCE_DIR}/../../lib")
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(SOURCE_FILES main.cpp)
add_executable(SimpleTest ${SOURCE_FILES})

target_link_libraries(SimpleTest rslic ${OpenCV_LIBS})
