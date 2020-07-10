cmake_minimum_required(VERSION 3.8)
project(DataCoreImServer)

set(CMAKE_CXX_STANDARD 17)

#sudo apt-get install cmake build-essential libopencv-dev boost openssl

if(WIN32)
	set(CMAKE_CXX_FLAGS_RELEASE "/MT")
	set(CMAKE_CXX_FLAGS_DEBUG "/MTd /Zi")
endif()

find_package(OpenCV CONFIG REQUIRED)
find_package(Boost COMPONENTS system REQUIRED)
find_package(OpenSSL REQUIRED)

add_executable(imserver src/main.cpp src/networkhelper.cpp src/wsserver.cpp)

target_link_libraries(imserver PRIVATE opencv_core opencv_imgcodecs opencv_xfeatures2d Boost::system OpenSSL::SSL OpenSSL::Crypto)
target_compile_options(imserver PRIVATE "/EHsc")

if(WIN32)
	target_compile_definitions(imserver PRIVATE _WIN32_WINNT=0x0601)
endif()
