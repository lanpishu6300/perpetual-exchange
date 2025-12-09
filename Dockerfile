# Multi-stage build for x86_64 with SIMD support
FROM --platform=linux/amd64 ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    g++ \
    make \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source files
COPY . .

# Build with AVX2 support
RUN mkdir -p build && cd build && \
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_FLAGS="-march=native -mavx2 -mfma -O3" \
        -DCMAKE_CXX_STANDARD=17 && \
    cmake --build . --config Release -j$(nproc)

# Runtime stage
FROM --platform=linux/amd64 ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libstdc++6 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy binaries from builder
COPY --from=builder /app/build/bin/PerpetualExchange .
COPY --from=builder /app/build/quick_benchmark .
COPY --from=builder /app/build/quick_comparison .
COPY --from=builder /app/build/simd_benchmark .

CMD ["./simd_benchmark"]


FROM --platform=linux/amd64 ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    g++ \
    make \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source files
COPY . .

# Build with AVX2 support
RUN mkdir -p build && cd build && \
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_FLAGS="-march=native -mavx2 -mfma -O3" \
        -DCMAKE_CXX_STANDARD=17 && \
    cmake --build . --config Release -j$(nproc)

# Runtime stage
FROM --platform=linux/amd64 ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libstdc++6 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy binaries from builder
COPY --from=builder /app/build/bin/PerpetualExchange .
COPY --from=builder /app/build/quick_benchmark .
COPY --from=builder /app/build/quick_comparison .
COPY --from=builder /app/build/simd_benchmark .

CMD ["./simd_benchmark"]


