FROM ubuntu:latest

WORKDIR /usr/src/app

# For apt to be noninteractive
ENV DEBIAN_FRONTEND noninteractive
ENV DEBCONF_NONINTERACTIVE_SEEN true

# Install needed packages
RUN apt-get update
RUN apt-get install -y cmake build-essential libboost-all-dev libopencv-dev openssl libboost-all-dev libtesseract-dev libssl-dev

EXPOSE 5001

COPY . .
RUN rm -rf build/
RUN mkdir build && cd build && cmake .. && make

#docker build --tag dcimageanalysis:1.0 .
#docker run --publish 5001:5001 -it --entrypoint /bin/bash dcimageanalysis:1.0