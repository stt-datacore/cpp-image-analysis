cmake_minimum_required(VERSION 3.8)
project(DataCoreImServer)

set(CMAKE_CXX_STANDARD 17)

#sudo apt-get install cmake build-essential libopencv-dev boost openssl

find_package(OpenCV CONFIG REQUIRED)
find_package(Boost COMPONENTS system REQUIRED)
find_package(OpenSSL REQUIRED)

add_executable(main src/main.cpp src/networkhelper.cpp src/wsserver.cpp)

target_link_libraries(main PRIVATE opencv_core opencv_imgcodecs opencv_xfeatures2d Boost::system OpenSSL::SSL OpenSSL::Crypto)
target_compile_options(main PRIVATE "/EHsc")
