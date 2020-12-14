////////////////////////////////////////////
// Small Compression Library              //
// author: Mariusz Ziach                  //
// www   : http://ziach.pl/               //
// date  : 2020                           //
////////////////////////////////////////////

// LHZX
#include "Types.h"

using namespace SCL;

// codec callback
CodecCallbackInterface::CodecCallbackInterface() {
    info.in_pos = 0;
    info.in_size = 0;
    info.out_size = 0;
    info.progress = 0;
    info.clock = 0;
    info.clock = 0;
}

CodecCallbackInterface::~CodecCallbackInterface() {}

// codec interface
CodecInterface::CodecInterface() {
    this->callback = nullptr;
}

CodecInterface::~CodecInterface() {};

void CodecInterface::setCallback(CodecCallbackInterface* callback) {
    this->callback = callback;
}
