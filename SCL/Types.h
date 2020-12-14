////////////////////////////////////////////
// Small Compression Library              //
// author: Mariusz Ziach                  //
// www   : http://ziach.pl/               //
// date  : 2020                           //
////////////////////////////////////////////

#ifndef SCL_TYPES_H
#define SCL_TYPES_H

// windows
#include <windows.h>

// c
#include <cstdint>
#include <cassert>

// c++
#include <fstream>
#include <string>
#include <iostream>

using namespace std;

namespace SCL {

// basic unsigned var types
typedef uint8_t  Byte;
typedef uint16_t Word;
typedef uint32_t DWord;
typedef uint64_t QWord;

class  InputStreamInterface;
class  OutputStreamInterface;

// enums
enum CallbackType {
    CLT_PROGRESS              = 1,
    CLT_FILE_FINISH           = 2,
    CLT_ARCHIVE_FINISH        = 3,
    CLT_FILE_BEGIN            = 4,
    CLT_ARCHIVE_BEGIN         = 5,
    CLT_COUNTING_FILES        = 6,
    CLT_COUNTING_FILES_FINISH = 7,
    CLT_STREAM_FINISH         = 8
};

enum CallbackAction { CLA_NOACTION = 1, CLA_COMPRESS = 2, CLA_DECOMPRESS = 3 };

// codec callback
struct CodecCallbackInfo {
    QWord in_pos;
    QWord in_size;
    QWord out_size;
    int   progress;
    int   ratio;
    int   clock;
};

class CodecCallbackInterface {
public:
    CodecCallbackInfo info;
    CodecCallbackInterface();
    virtual ~CodecCallbackInterface();
    virtual bool callback(CallbackType callback_type) = 0;
};

// interface of compression algorithm
class CodecInterface {
protected:
    CodecCallbackInterface* callback;
public:
    CodecInterface();
    virtual void setCallback(CodecCallbackInterface* callback);
    virtual QWord compressStream  (InputStreamInterface* input, OutputStreamInterface* output) = 0;
    virtual QWord decompressStream(InputStreamInterface* input, OutputStreamInterface* output) = 0;
    virtual ~CodecInterface() = 0;
};

} // namespace

#endif // SCL_TYPES_H
