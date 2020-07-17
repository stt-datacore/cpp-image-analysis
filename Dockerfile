FROM ubuntu:latest

WORKDIR /usr/src/dcimageanalysis
COPY . .

# For apt to be noninteractive
ENV DEBIAN_FRONTEND noninteractive
ENV DEBCONF_NONINTERACTIVE_SEEN true

EXPOSE 5001

# Install needed packages
RUN cd .. && \
    apt-get update && \
    apt-get install -y wget build-essential cmake pkg-config git libjpeg8-dev libpng-dev software-properties-common libtesseract-dev libssl-dev && \
    wget https://github.com/opencv/opencv/archive/4.3.0.tar.gz -O - | tar -xzvf - && \
    mv opencv-4.3.0 opencv && \
    wget https://dl.bintray.com/boostorg/release/1.73.0/source/boost_1_73_0.tar.gz -O - | tar -xzvf - && \
    mv boost_1_73_0 boost && \
    cd opencv && mkdir build && cd build && \
    cmake \
        -DBUILD_LIST=core,imgproc,imgcodecs,features2d,flann \
        -D CMAKE_BUILD_TYPE=RELEASE \
        -D ENABLE_CXX11=ON \
        -D WITH_V4L=OFF \
        -D WITH_QT=OFF \
        -D WITH_OPENGL=OFF \
        -D OPENCV_ENABLE_PRECOMPILED_HEADERS=OFF \
        -D BUILD_EXAMPLES=OFF \
        -D BUILD_DOCS=OFF \
        .. && \
    make && make install && \
    cd /usr/src/dcimageanalysis && \
    mkdir build && cd build && cmake -DDC_BOOST_SRC=/usr/src/boost .. && make && \
    cd .. && rm -rf boost/ && rm -rf opencv/

#docker build --tag dcimageanalysis:1.0 .
#docker run --publish 5001:5001 -it --entrypoint /bin/bash dcimageanalysis:1.0