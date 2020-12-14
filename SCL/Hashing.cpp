////////////////////////////////////////////
// Small Compression Library              //
// author: Mariusz Ziach                  //
// www   : http://ziach.pl/               //
// date  : 2020                           //
////////////////////////////////////////////

#include "Hashing.h"

using namespace SCL;

// hashing interface
HashingInterface::~HashingInterface() {}

Hashing::Hashing() {
    mem = new Byte[0xFFFF];
    init();
}

Hashing::~Hashing() {
    if (mem) delete[] mem;
}

void Hashing::init() {
    this->hash = 0x811C9DC5;
}

void Hashing::updateHash(Byte* in, QWord size) {
    for (QWord i = 0; i < size; i++) {
        this->hash ^= in[i];
        this->hash *= 0x1000193;
    }
}

void Hashing::hashStream(InputStreamInterface *in) {
    this->hash = 0x811C9DC5;

    QWord temp_pos = in->getPos();
    while (in->getPos() < in->getSize()) {
        in->read  (mem, 0xFFFF);
        updateHash(mem, in->getReadSize());
    }
    in->setPos(temp_pos); 
}

DWord Hashing::getHash() {
    return this->hash;
}

HashingOutputStream::HashingOutputStream(HashingInterface* hashing, OutputStreamInterface *output_stream) {
    this->hashing       = hashing;
    this->output_stream = output_stream;
}

bool HashingOutputStream::write(Byte* buf, QWord size) {
    hashing->updateHash(buf, size);
    output_stream->write(buf, size);
    return true;
}

