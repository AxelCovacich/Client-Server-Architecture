FROM ubuntu:22.04

# Install dependencies needed for building the project
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libsqlite3-dev \
    zlib1g-dev \
    wget

# Install Prometheus C++ client library
RUN git clone https://github.com/jupp0r/prometheus-cpp.git /tmp/prometheus-cpp \
    && cd /tmp/prometheus-cpp \
    && git submodule update --init --recursive \
    && mkdir _build && cd _build \
    && cmake .. -DBUILD_SHARED_LIBS=ON -DENABLE_PUSH=OFF -DENABLE_COMPRESSION=OFF \
    && make -j$(nproc) \
    && make install

# Install CMake 3.27.9
RUN apt-get update && apt-get install -y wget \
    && wget https://github.com/Kitware/CMake/releases/download/v3.27.9/cmake-3.27.9-linux-x86_64.sh \
    && sh cmake-3.27.9-linux-x86_64.sh --skip-license --prefix=/usr/local \
    && rm cmake-3.27.9-linux-x86_64.sh

# Install Python 3 and pip to run scripts
RUN apt-get update && apt-get install -y python3 python3-pip

# Set the working directory
WORKDIR /my_project

# Copy the entire project into the container
COPY . /my_project

#build the project with coverage enabled
RUN cmake -S . -B build \
    -DENABLE_COVERAGE=ON \
    -DCMAKE_BUILD_TYPE=Debug

#compile the project
RUN cmake --build build -- -j$(nproc)