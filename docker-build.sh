#!/bin/bash

# Build Docker image for x86_64 with SIMD support
echo "Building Docker image for x86_64 with SIMD support..."

# Build for linux/amd64 platform
docker buildx build \
    --platform linux/amd64 \
    --tag perpetual-exchange:simd \
    --load \
    -f Dockerfile .

echo "Build complete!"
echo ""
echo "To run the benchmark:"
echo "  docker run --rm --platform linux/amd64 perpetual-exchange:simd"
echo ""
echo "Or use docker-compose:"
echo "  docker-compose up"



# Build Docker image for x86_64 with SIMD support
echo "Building Docker image for x86_64 with SIMD support..."

# Build for linux/amd64 platform
docker buildx build \
    --platform linux/amd64 \
    --tag perpetual-exchange:simd \
    --load \
    -f Dockerfile .

echo "Build complete!"
echo ""
echo "To run the benchmark:"
echo "  docker run --rm --platform linux/amd64 perpetual-exchange:simd"
echo ""
echo "Or use docker-compose:"
echo "  docker-compose up"


