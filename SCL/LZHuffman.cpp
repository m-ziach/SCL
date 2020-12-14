////////////////////////////////////////////
// Small Compression Library              //
// author: Mariusz Ziach                  //
// www   : http://ziach.pl/               //
// date  : 2020                           //
////////////////////////////////////////////

#include "LZHuffman.h"

using namespace SCL;

LZHuffmanCodecCallback::LZHuffmanCodecCallback(CodecCallbackInterface* parent_callback, bool is_huffman) {
    this->parent_callback = parent_callback;
    this->is_huffman      = is_huffman;
}

CodecCallbackInfo LZHuffmanCodecCallback::lz_info      = { 0, 0, 0, 0, 0, 0 };
CodecCallbackInfo LZHuffmanCodecCallback::huffman_info = { 0, 0, 0, 0, 0, 0 };

void LZHuffmanCodecCallback::init(bool is_compression) {
    clock_begin = clock();
    this->is_compression = is_compression;
    if (is_huffman) memset(&huffman_info, 0, sizeof(CodecCallbackInfo));
    else            memset(&lz_info,      0, sizeof(CodecCallbackInfo));
    if (parent_callback)
        memset(&parent_callback->info,    0, sizeof(CodecCallbackInfo));
}

bool LZHuffmanCodecCallback::callback(CallbackType callback_type) {
    CodecCallbackInfo* c_info = is_huffman ? &huffman_info : &lz_info;
    bool ret = true;
    c_info->clock    = COUNTTIME(clock_begin);
    c_info->progress = info.progress;
    c_info->ratio    = info.ratio;

    if (is_compression) {
        if (!is_huffman) {
            parent_callback->info.in_pos   = c_info->in_pos   + info.in_pos;
            parent_callback->info.in_size  = c_info->in_size  + info.in_size;
        } else {
            parent_callback->info.out_size = c_info->out_size + info.out_size;
        }
    } else {
        if (!is_huffman) {
            parent_callback->info.out_size = c_info->out_size + info.out_size;
        } else {
            parent_callback->info.in_pos   = c_info->in_pos   + info.in_pos;
            parent_callback->info.in_size  = c_info->in_size  + info.in_size;
        }
    }

    if (!is_huffman) {
        parent_callback->info.clock    = c_info->clock;
        parent_callback->info.progress = c_info->progress;
        ret = parent_callback->callback(callback_type);
    }

    if (callback_type == CLT_STREAM_FINISH) {
        c_info->in_pos += info.in_pos;
        c_info->in_size += info.in_size;
        c_info->out_size += info.out_size;
    }

    return ret;
}

LZHuffman::LZHuffman(LZCompressionLevel comp_level) {
    lz_codec         = new LZ(comp_level);
    huffman_codec    = new Huffman;
    lz_callback      = nullptr;
    huffman_callback = nullptr;
}

LZHuffman::~LZHuffman() {
    if (lz_codec)         delete lz_codec;
    if (huffman_codec)    delete huffman_codec;
    if (lz_callback)      delete lz_callback;
    if (huffman_callback) delete huffman_callback;
    lz_codec            = nullptr;
    huffman_codec       = nullptr;
    lz_callback         = nullptr;
    huffman_callback    = nullptr;
}

void LZHuffman::setCallback(CodecCallbackInterface* callback) {
    parent_callback = callback;

    if (this->lz_callback)      delete this->lz_callback;
    if (this->huffman_callback) delete this->huffman_callback;
    this->lz_callback         = new LZHuffmanCodecCallback(parent_callback, false);
    this->huffman_callback    = new LZHuffmanCodecCallback(parent_callback, true);

    lz_codec     ->setCallback(this->lz_callback);
    huffman_codec->setCallback(this->huffman_callback);
}

QWord LZHuffman::compressStream(InputStreamInterface* input, OutputStreamInterface* output) {
    this->lz_callback     ->init(true);
    this->huffman_callback->init(true);

    CodecOutputStream huffman_output(input, output, huffman_codec);

    QWord begin = output->getPos();
    lz_codec->compressStream(input, &huffman_output);
    QWord end = output->getPos();

    return end - begin;
}

QWord LZHuffman::decompressStream(InputStreamInterface* input, OutputStreamInterface* output) {
    this->lz_callback     ->init(false);
    this->huffman_callback->init(false);

    CodecInputStream huffman_input(input, output, huffman_codec);
    return lz_codec->decompressStream(&huffman_input, output);
}
