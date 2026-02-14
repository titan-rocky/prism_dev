FROM debian:stable-slim

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
      build-essential cmake pkg-config git ca-certificates \
      libpcap-dev libboost-all-dev libssl-dev golang-go && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /src

# Copy only simulator sources to avoid host build artifacts
COPY CMakeLists.txt /src/
COPY core /src/core
COPY proto /src/proto
COPY README.md /src/README.md
COPY api /src/api

# Ensure a clean out-of-source build and provide a non-invasive compile-time
# include for <cstdlib> so existing source files do not need editing.
ENV CXXFLAGS="-include cstdlib"

RUN rm -rf /src/build && mkdir -p build && cd build && \
    cmake -DCMAKE_CXX_FLAGS="$CXXFLAGS" .. && \
    make -j$(nproc) && \
    cp /src/build/scada-sim /usr/local/bin/scada-sim

WORKDIR /src/api
RUN go build -o /usr/local/bin/sim-api

EXPOSE 8080 502 20000

ENTRYPOINT ["/usr/local/bin/sim-api"]
