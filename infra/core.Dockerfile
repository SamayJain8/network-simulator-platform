# Stage 1: Compile the C++20 release target
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

WORKDIR /app/build
RUN cmake -DCMAKE_BUILD_TYPE=Release .. && make

# Stage 2: Deploy final binary into minimal runtime container
FROM ubuntu:22.04

WORKDIR /usr/local/bin

# Copy only the compiled target executable from stage 1
COPY --from=builder /app/build/network_core .

# Run as a non-privileged user for production security compliance
RUN useradd -m -u 1001 appuser
USER appuser

ENTRYPOINT ["./network_core"]