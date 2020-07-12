cmake_minimum_required(VERSION 3.8)
project(DataCoreImServer)

set(CMAKE_CXX_STANDARD 17)

# There is no ubuntu package for xfeatures2d (SURF). OpenCV needs to be built from source to include them :(
#sudo apt-get install cmake build-essential libboost-all-dev libopencv-dev libopencv-contrib-dev openssl libboost-all-dev libtesseract-dev libssl-dev

if(WIN32)
	set(CMAKE_CXX_FLAGS_RELEASE "/MT")
	set(CMAKE_CXX_FLAGS_DEBUG "/MTd /Zi")

	find_package(Tesseract CONFIG REQUIRED)
else()
	find_package(PkgConfig REQUIRED)

	pkg_search_module(TESSERACT REQUIRED tesseract)
	pkg_search_module(LEPTONICA REQUIRED lept)

	include_directories(${TESSERACT_INCLUDE_DIRS})
	include_directories(${LEPTONICA_INCLUDE_DIRS})

	link_directories(${TESSERACT_LIBRARY_DIRS})
	link_directories(${LEPTONICA_LIBRARY_DIRS})
endif()

find_package(OpenCV CONFIG REQUIRED)

find_package(Boost COMPONENTS system REQUIRED)
find_package(OpenSSL REQUIRED)

add_executable(imserver src/main.cpp src/networkhelper.cpp src/wsserver.cpp src/beholdhelper.cpp "src/voyimage.h" "src/voyimage.cpp")

target_link_libraries(imserver PRIVATE opencv_core opencv_imgcodecs opencv_xfeatures2d Boost::system OpenSSL::SSL OpenSSL::Crypto)

if(WIN32)
	target_link_libraries(imserver PRIVATE libtesseract)
	target_compile_options(imserver PRIVATE "/EHsc")
	target_compile_definitions(imserver PRIVATE _WIN32_WINNT=0x0601)
else()
	target_link_libraries(imserver PRIVATE ${TESSERACT_LIBRARIES} ${LEPTONICA_LIBRARIES})
endif()
