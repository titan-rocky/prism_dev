FROM debian:stable-slim

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
      build-essential cmake pkg-config git ca-certificates \
      libpcap-dev libboost-all-dev libssl-dev && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /src

# Copy only source files to avoid host build artifacts
COPY CMakeLists.txt /src/
COPY include /src/include
COPY src /src/src
COPY docs /src/docs
COPY compiler_flags.txt /src/

# build PRISM (assumes CMakeLists present at repo root)
RUN mkdir -p build && cd build && cmake .. && make -j$(nproc)

# Default command runs prism on loopback; override in compose as needed
CMD ["./build/prism", "lo"]
