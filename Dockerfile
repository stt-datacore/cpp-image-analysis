FROM alpine:3.12

ENV LANG=C.UTF-8

EXPOSE 5001

# Install required packages
RUN apk update && apk upgrade && apk --no-cache add \
  build-base \
  pkgconfig \
  cmake \
  coreutils \
  gcc \
  g++ \
  git \
  libc-dev \
  make \
  unzip \
  tesseract-ocr-dev \
  leptonica-dev \
  openssl-dev \
  linux-headers

# Build and install OpenCV
RUN mkdir /usr/src && cd /usr/src && \
  wget https://github.com/opencv/opencv/archive/4.3.0.zip && \
  unzip 4.3.0.zip && rm 4.3.0.zip && \
  cd /usr/src/opencv-4.3.0 && mkdir build && cd build && \
  cmake -D CMAKE_BUILD_TYPE=RELEASE \
    -DBUILD_LIST=core,imgproc,imgcodecs,features2d,flann \
    -D ENABLE_CXX11=ON \
    -D WITH_V4L=OFF \
    -D WITH_QT=OFF \
    -D WITH_OPENGL=OFF \
    -D OPENCV_ENABLE_PRECOMPILED_HEADERS=OFF \
    -D BUILD_EXAMPLES=OFF \
    -D BUILD_DOCS=OFF \
    .. \
  && \
  make -j$(nproc) && make install && rm -rf /usr/src/opencv-4.3.0

WORKDIR /usr/src/dcimageanalysis
COPY . .
RUN apk --no-cache add pkgconfig tesseract-ocr && \
    cd /usr/src && \
    wget https://dl.bintray.com/boostorg/release/1.73.0/source/boost_1_73_0.zip && \
    unzip boost_1_73_0.zip && rm boost_1_73_0.zip && mv boost_1_73_0 boost && \
    cd /usr/src/dcimageanalysis && \
    mkdir build && cd build && cmake -DDC_BOOST_SRC=/usr/src/boost .. && make -j$(nproc) && cd .. && rm -rf /usr/src/boost

#docker build --tag dcimageanalysis:1.8 .
#docker run --publish 5001:5001 -it --entrypoint /bin/bash dcimageanalysis:1.0