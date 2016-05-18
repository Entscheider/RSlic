 project(3DTest)

find_package( OpenCV REQUIRED )
include_directories ("${3DTest_SOURCE_DIR}/../../lib")
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(SOURCE_FILES main.cpp parser.cpp exporter.cpp)
add_executable(3DTest ${SOURCE_FILES})

target_link_libraries(3DTest rslic ${OpenCV_LIBS})
