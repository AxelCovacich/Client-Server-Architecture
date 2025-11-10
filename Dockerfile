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
RUN pip3 install posix_ipc

# Set the working directory
WORKDIR /Last_of_Us_System

# Copy the entire project into the container
COPY . /Last_of_Us_System

#build the project with coverage enabled
RUN cmake -S . -B build \
    -DENABLE_COVERAGE=ON \
    -DCMAKE_BUILD_TYPE=Debug

#compile the project
RUN cmake --build build -- -j$(nproc)

RUN apt-get update && apt-get install -y adduser

#create necessary directories and a non-root user to run the server
RUN addgroup --system serveruser && adduser --system --ingroup serveruser --no-create-home --shell /usr/sbin/nologin serveruser

RUN chown -R serveruser:serveruser /Last_of_Us_System

RUN mkdir -p /Last_of_Us_System/var/lib /Last_of_Us_System/var/logs /Last_of_Us_System/var/run \
    && chown -R serveruser:serveruser /Last_of_Us_System/var /Last_of_Us_System/etc 

USER serveruser

CMD ["build/bin/server"]