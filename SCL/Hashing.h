////////////////////////////////////////////
// Small Compression Library              //
// author: Mariusz Ziach                  //
// www   : http://ziach.pl/               //
// date  : 2020                           //
////////////////////////////////////////////

#ifndef SCL_HASHING_H
#define SCL_HASHING_H

#include "Types.h"
#include "Streams.h"

namespace SCL {

class HashingOutputStream;

// hashing
class HashingInterface {
public:
    virtual ~HashingInterface() = 0;
    virtual void init() = 0;
    virtual void hashStream(InputStreamInterface* in) = 0;
    virtual void updateHash(Byte* in, QWord size) = 0;
    virtual DWord getHash() = 0;
};

class Hashing : public HashingInterface {
protected:
    Byte* mem;
    DWord hash;
 
public:
    Hashing();
    ~Hashing();
    void init();
    void  updateHash(Byte* in, QWord size);
    void hashStream(InputStreamInterface* in);
    DWord getHash();
};

class HashingOutputStream : public OutputStreamInterface {
private:
    HashingInterface* hashing;
    OutputStreamInterface *output_stream;
public:
    HashingOutputStream(HashingInterface *hashing, OutputStreamInterface* output_stream);
    bool write(Byte* buf, QWord size);
};


}

#endif // SCL_HASHING_H
