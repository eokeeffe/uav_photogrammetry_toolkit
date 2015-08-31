cmake_minimum_required (VERSION 2.8)
project (A_Opp_DoB_H_SIFT)

find_package(OpenCV REQUIRED)

find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS} -O3")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS} -O3")
endif()

include_directories(
    ${OPENCV_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include/
)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
        message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

#set(CMAKE_C_FLAGS_RELEASE "-O0 -g3 -Wall -c -fmessage-length=0 -std=c++0x -MMD -MP")
#set(CMAKE_C_FLAGS_DEBUG "-O0 -g3 -Wall -c -fmessage-length=0 -std=c++0x -MMD -MP")

add_executable(main
    src/main.cpp
    src/ASIFT.cpp
#    src/sift2.cpp
    src/rootSIFT.cpp
    src/vsfm_sift.cpp
    src/vsfm_mat.cpp
    src/my_sift.cpp
)
target_link_libraries(main ${OpenCV_LIBS})
