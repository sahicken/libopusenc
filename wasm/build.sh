#!/bin/bash

set -e

echo "Building minimal libopusenc WebAssembly encoder..."

# Ensure we're in the wasm directory
cd "$(dirname "$0")"

# Check if Emscripten is available
if ! command -v emcc &> /dev/null; then
    echo "❌ Emscripten not found!"
    echo "Please install and activate emsdk:"
    echo ""
    echo "  git clone https://github.com/emscripten-core/emsdk.git"
    echo "  cd emsdk"
    echo "  ./emsdk install latest"
    echo "  ./emsdk activate latest"
    echo "  source ./emsdk_env.sh"
    echo ""
    exit 1
fi

echo "✓ Emscripten found: $(emcc --version | head -n1)"

# Build with Emscripten's opus port (auto-downloads libopus)
echo "Building WASM module (this will download libopus automatically)..."

# First, check if we need to download and build opus
if [ ! -d "opus-build" ]; then
    echo "Downloading and building libopus..."
    mkdir -p opus-build
    cd opus-build
    
    # Download opus
    if [ ! -f "opus-1.5.2.tar.gz" ]; then
        wget https://downloads.xiph.org/releases/opus/opus-1.5.2.tar.gz
        tar -xzf opus-1.5.2.tar.gz
    fi
    
    cd opus-1.5.2
    
    # Configure and build with Emscripten (disable intrinsics for WASM)
    emconfigure ./configure --disable-shared --disable-doc --disable-extra-programs \
        --disable-intrinsics --disable-rtcd
    emmake make
    
    cd ../..
fi

emcc encoder.c \
    ../src/opusenc.c \
    ../src/ogg_packer.c \
    ../src/opus_header.c \
    ../src/resample.c \
    -I../include \
    -I../src \
    -Iopus-build/opus-1.5.2/include \
    -DOUTSIDE_SPEEX \
    -DRANDOM_PREFIX=opusenc \
    -DPACKAGE_NAME='"libopusenc"' \
    -DPACKAGE_VERSION='"0.2.1-wasm"' \
    opus-build/opus-1.5.2/.libs/libopus.a \
    -O3 \
    -sWASM=1 \
    -sEXPORTED_FUNCTIONS='["_encoder_create","_encoder_write","_encoder_get_page","_encoder_drain","_encoder_get_data","_encoder_get_size","_encoder_destroy","_malloc","_free"]' \
    -sEXPORTED_RUNTIME_METHODS='["ccall","cwrap","getValue","HEAP8","HEAPF32"]' \
    -sALLOW_MEMORY_GROWTH=1 \
    -sINITIAL_MEMORY=33554432 \
    -sMAXIMUM_MEMORY=536870912 \
    -sMODULARIZE=1 \
    -sEXPORT_NAME='createOpusEncoder' \
    -o opus-encoder.js

echo ""
echo "✅ Build complete!"
echo ""
echo "Generated files:"
echo "  📄 opus-encoder.js   - JavaScript loader"
echo "  📦 opus-encoder.wasm - WebAssembly binary"
echo ""
echo "To test, run:"
echo "  python3 -m http.server 8000"
echo "Then open: http://localhost:8000/index.html"
