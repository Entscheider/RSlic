project(RSlic)


find_package( OpenCV REQUIRED )
set(CMAKE_INCLUDE_CURRENT_DIR ON)

option(PARALLEL "Use Threads" OFF) # Solange Ergebnisse sich unterscheiden pro Durchgang
IF(${PARALLEL})
  add_definitions( -DPARALLEL)
ENDIF()

set(SOURCE_FILES
    Pixel/RSlic2.cpp Pixel/ClusterSet.cpp Pixel/RSlic2Draw.cpp Pixel/RSlic2Util.cpp
    Voxel/RSlic3.cpp Voxel/ClusterSet.cpp Voxel/RSlic3Utils.cpp
    )
add_library(rslic STATIC ${SOURCE_FILES})

target_link_libraries(rslic ${OpenCV_LIBS})

install(TARGETS rslic DESTINATION lib/rslic EXPORT rslic-target)
install(DIRECTORY . DESTINATION include/RSlic
        FILES_MATCHING PATTERN "*.h")
install(EXPORT rslic-target DESTINATION lib/rslic)
install(FILES rslic-config.cmake DESTINATION lib/rslic)
# another package example https://github.com/forexample/package-example
