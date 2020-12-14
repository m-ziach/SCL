////////////////////////////////////////////
// Small Compression Library              //
// author: Mariusz Ziach                  //
// www   : http://ziach.pl/               //
// date  : 2020                           //
////////////////////////////////////////////

#ifndef SCL_LZHUFFMAN_H
#define SCL_LZHUFFMAN_H

#include "Types.h"
#include "Utils.h"
#include "LZ.h"
#include "Huffman.h"
#include "BitStream.h"
#include "Streams.h"

namespace SCL {

class LZHuffmanCodecCallback : public CodecCallbackInterface {
private:
    static CodecCallbackInfo huffman_info;
    static CodecCallbackInfo lz_info;
    CodecCallbackInterface* parent_callback;
    bool is_huffman;
    bool is_compression;
    clock_t clock_begin;
public:
    LZHuffmanCodecCallback(CodecCallbackInterface* parent_callback, bool is_huffman);
    void init(bool is_compression);
    bool callback(CallbackType callback_type);
};

class LZHuffman : public CodecInterface {
private:
    LZ          *lz_codec;
    Huffman     *huffman_codec;
    CodecCallbackInterface* parent_callback;
    LZHuffmanCodecCallback* huffman_callback;
    LZHuffmanCodecCallback* lz_callback;

public:
    LZHuffman(LZCompressionLevel comp_level = LCL_NORMAL);
    ~LZHuffman();
    void setCallback(CodecCallbackInterface* callback);
    QWord compressStream  (InputStreamInterface* input, OutputStreamInterface* output);
    QWord decompressStream(InputStreamInterface* input, OutputStreamInterface* output);
};
}
#endif // SCL_LZHUFFMAN_H
