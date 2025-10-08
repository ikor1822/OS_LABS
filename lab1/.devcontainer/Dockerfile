
FROM gcc:14.2.0

RUN apt-get update && apt-get install -y \
    cmake python3 python3-dev python3-pip wrk clangd\
    && rm -rf /var/lib/apt/lists/*

RUN pip install numpy --break-system-packages

FROM mcr.microsoft.com/devcontainers/cpp:1-debian-11

# Установка GoogleTest
RUN apt-get update && apt-get install -y \
    libgtest-dev \
    cmake \
    && rm -rf /var/lib/apt/lists/*

# Компиляция gtest
RUN cd /usr/src/gtest && \
    cmake CMakeLists.txt && \
    make && \
    cp lib/*.a /usr/lib

# Установка дополнительных инструментов
RUN apt-get update && apt-get install -y \
    git \
    gdb \
    valgrind

WORKDIR /workspace