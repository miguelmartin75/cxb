> [!WARNING]  
> Under Construction

# cxb

A C++ utility library with focus on performance and simplicity.

## Features

* **UTF-8 Processing**: Complete UTF-8 encoding/decoding with both single-character and batch processing support
* **String Handling**: Dynamic string type (`Str8`) with UTF-8 awareness
* **Memory Management**: Custom allocators with tracking capabilities
* **Containers**: Dynamic arrays (`Seq`) and other essential data structures
* **Math Types**: Common vector, matrix, and geometric types
* **Cross Platform**: Works on major platforms with consistent behavior

### UTF-8 Batch Processing

The library includes efficient batch UTF-8 decoding for processing multiple codepoints at once:

```cpp
Str8 text("Hello 🌍 World!");
Utf8BatchIterator iter(text, 32);

rune buffer[32];
while (iter.has_next()) {
    auto result = iter.next_batch(buffer);
    // Process batch of decoded codepoints
}
```

See [docs/UTF8_BATCH_DECODING.md](docs/UTF8_BATCH_DECODING.md) for detailed documentation.

Current Header Compile Times:
* `cxb.h`: 52ms on 2020 MBP (i5 Quad Core)
    * TODO: run in CI and obtain this number

## Development

For development setup, building, testing, and contribution guidelines, see [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md).
