////////////////////////////////////////////
// Small Compression Library              //
// author: Mariusz Ziach                  //
// www   : http://ziach.pl/               //
// date  : 2020                           //
////////////////////////////////////////////

#ifndef SCL_LZ_H
#define SCL_LZ_H

// Archive
#include "Types.h"
#include "Utils.h"
#include "BitStream.h"
#include "Streams.h"

// number of LZ streams
#define LZ_NUMBER_OF_STREAMS 4

// IDs
#define LZ_INSTRUCTION  0   // instruction stream
#define LZ_MATCH_POS    1   // match pos stream
#define LZ_MATCH_LEN    2   // match len stream
#define LZ_CHAR         3   // literal stream

// LZ instructions
#define LZ_WRITEMATCH8  0   // match 1 
#define LZ_WRITEMATCH16 1   // match 2
#define LZ_WRITECHAR    2   // char

// others
#define LZ_MIN_MATCH    4   // minimum match len

namespace SCL {

// compression level
enum LZCompressionLevel {LCL_FASTEST = 1, LCL_FAST = 2, LCL_NORMAL = 3, LCL_BEST = 4};

// codec settings used in LZ compressor
class LZCodecSettings {
public:
    void Set(DWord bit_lkp_cap,
        DWord bit_lkp_hsh, DWord bit_mtch_len,
        DWord bit_mtch_pos,
        DWord bit_runs);

    // settings values in bits
    DWord bit_lkp_cap;  // lookup table size 
    DWord bit_lkp_hsh;  // number of bytes to count hash for lookup table 
    DWord bit_mtch_len; // max match lenght size
    DWord bit_mtch_pos; // max match position size

    DWord bit_runs;
    // in bytes
    DWord byte_lkp_cap, byte_lkp_hsh,
        byte_mtch_len, byte_mtch_pos,
        byte_runs;
    // masks
    DWord  mask_lkp_cap, mask_lkp_hsh,
        mask_mtch_len, mask_mtch_pos,
        mask_runs;
};

// dictionary mem
class LZDictionaryBuffer {
public:
    Byte *arr;
    QWord pos, size, cap;
    LZDictionaryBuffer(QWord cap);
    ~LZDictionaryBuffer();
    Byte *putByte(Byte val);
    Byte  getByte(QWord p);
    QWord   getPos();
    QWord   convPos(bool absToRel, QWord pos);
    void  clear();
};

// lz match 
class LZMatch {
public:
    QWord pos, len;
    LZMatch();
    void clear();
    void copy(LZMatch *lzm);
};

// lz lookup table node
class LZDictionaryNode {
public:
    QWord pos;
    bool valid;
    LZDictionaryNode *next, *prev;
    LZDictionaryNode();
    void clear();
};

// lz match finder
class LZMatchFinder {
private:
    QWord dict_i, buf_size;
    Byte               *buf;
    LZCodecSettings    *cdc_sttgs;
    LZDictionaryBuffer *lz_buf;
    LZDictionaryNode   *lkp_tab, *dict_tab;
    LZMatch            *best_match;
public:
    LZMatchFinder(LZCodecSettings *cdc_sttgs);
    ~LZMatchFinder();
    void assignBuffer(Byte *buf, QWord buf_size, LZDictionaryBuffer *lz_buf);
    QWord  hash(Byte *in);
    void insert(QWord pos);
    LZMatch *find(QWord pos);
    void  clear();
};

// lz algorithm main class
class LZ : public CodecInterface {
private:
    Byte*               uncompressed_bytes;
    Byte*               compressed_bytes[LZ_NUMBER_OF_STREAMS];
    LZCodecSettings     cdc_sttgs;
    LZMatch             match_empty;
    LZMatch            *lz_match;
    LZMatchFinder      *lz_mf;
    LZDictionaryBuffer *lz_buf;
public:
    LZ(LZCompressionLevel comp_level = LCL_NORMAL);
    ~LZ();
    LZCodecSettings* getSettings();
    QWord compressStream  (InputStreamInterface* input, OutputStreamInterface* output);
    QWord decompressStream(InputStreamInterface* input, OutputStreamInterface* output);
};

} // namespace

#endif // SCL_LZ_H
