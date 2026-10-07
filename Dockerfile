FROM debian:bookworm-slim AS build

RUN apt-get update \
    && apt-get install --yes --no-install-recommends cmake g++ make \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /source
COPY CMakeLists.txt README.md ./
COPY src/ src/

RUN cmake -S . -B build \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_TESTING=OFF \
    && cmake --build build --parallel \
    && cmake --install build --prefix /opt/telemetry-guard

FROM debian:bookworm-slim AS runtime

COPY --from=build /opt/telemetry-guard/bin/telemetry_guard /usr/local/bin/telemetry_guard

RUN mkdir /data && chown 65532:65532 /data
USER 65532:65532
WORKDIR /data

ENTRYPOINT ["telemetry_guard"]
CMD ["--version"]
