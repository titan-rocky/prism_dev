FROM debian:stable-slim AS builder

ENV DEBIAN_FRONTEND=noninteractive
ENV CXXFLAGS="-include cstdlib"

RUN apt-get update && \
    apt-get install -y --no-install-recommends build-essential cmake pkg-config git ca-certificates libpcap-dev libboost-all-dev libssl-dev golang-go && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY CMakeLists.txt /src/
COPY core /src/core
COPY proto /src/proto
COPY api /src/api

RUN rm -rf /src/build && mkdir -p build && cd build && \
    cmake -DCMAKE_CXX_FLAGS="$CXXFLAGS" .. && \
    make -j$(nproc) && \
    cp /src/build/scada-sim /usr/local/bin/scada-sim

RUN cd /src/api && go build -o /usr/local/bin/sim-api

FROM debian:stable-slim
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends ca-certificates && rm -rf /var/lib/apt/lists/*
COPY --from=builder /usr/local/bin/sim-api /usr/local/bin/sim-api
COPY --from=builder /usr/local/bin/scada-sim /usr/local/bin/scada-sim
EXPOSE 8080
ENTRYPOINT ["/usr/local/bin/sim-api"]
