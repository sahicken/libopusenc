# WebAssembly WAV to Opus Encoder

Client-side audio encoding powered by libopusenc compiled to WebAssembly.

## Features

- 🎵 Convert WAV files to Opus format entirely in the browser
- 🔒 100% client-side processing - your files never leave your device
- 🚀 Fast WebAssembly-powered encoding
- 📦 Small bundle size: ~324KB total (13KB JS + 311KB WASM)
- 🎚️ Automatic resampling to 48kHz using Web Audio API
- 💾 Supports any WAV format (mono/stereo, various sample rates)

## Building

### Prerequisites

Install Emscripten SDK:

```bash
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
```

### Build

```bash
cd wasm
./build.sh
```

This will:
1. Download and compile libopus (1.5.2) with Emscripten
2. Compile libopusenc source files
3. Generate `opus-encoder.js` and `opus-encoder.wasm`

## Usage

### Testing Locally

```bash
cd wasm
python3 -m http.server 8000
```

Then open http://localhost:8000/index.html in your browser.

### Using in Your Own Project

1. Copy `opus-encoder.js` and `opus-encoder.wasm` to your project
2. Load the module:

```html
<script src="opus-encoder.js"></script>
<script>
  createOpusEncoder().then(module => {
    console.log('Encoder ready!');
    // Use module here
  });
</script>
```

3. Encode audio:

```javascript
// Create encoder (48kHz, stereo)
const encoderPtr = module._encoder_create(48000, 2);

// Write PCM data (float32 array, interleaved for stereo)
const pcmPtr = module._malloc(pcmData.length * 4);
module.HEAPF32.set(pcmData, pcmPtr / 4);
module._encoder_write(encoderPtr, pcmPtr, samples);
module._free(pcmPtr);

// Get pages during encoding
while (module._encoder_get_page(encoderPtr) > 0) {
  // Pages are collected internally
}

// Drain and get final output
module._encoder_drain(encoderPtr);

// Get encoded data
const dataPtr = module._encoder_get_data(encoderPtr);
const dataSize = module._encoder_get_size(encoderPtr);
const opusData = new Uint8Array(module.HEAPU8.buffer, dataPtr, dataSize);

// Clean up
module._encoder_destroy(encoderPtr);
```

## Encoder Settings

Default settings (defined in `encoder.c`):
- **Bitrate**: 128 kbps
- **Complexity**: 5 (medium)
- **Application**: Audio (auto-selected)

To modify, edit the `encoder_create()` function in `encoder.c` and rebuild.

## Technical Details

### Source Files

Compiled from libopusenc:
- `src/opusenc.c` - Main encoder logic
- `src/ogg_packer.c` - Ogg container muxing  
- `src/opus_header.c` - OpusHead/OpusTags generation
- `src/resample.c` - Speex resampler

Compiled from libopus:
- Uses libopus 1.5.2 static library

### Memory Management

- Initial memory: 32MB
- Maximum memory: 512MB
- Memory growth: Enabled (grows as needed)

### Browser Support

Works in all modern browsers with WebAssembly support:
- Chrome/Edge 57+
- Firefox 52+
- Safari 11+
- Opera 44+

## License

This WebAssembly wrapper is provided as-is. libopusenc and libopus are licensed under the BSD license.
