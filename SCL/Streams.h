////////////////////////////////////////////
// Small Compression Library              //
// author: Mariusz Ziach                  //
// www   : http://ziach.pl/               //
// date  : 2020                           //
////////////////////////////////////////////

#ifndef SCL_STREAMS_H
#define SCL_STREAMS_H

// Archive
#include "Types.h"

namespace SCL {

// streams
class StreamInterface {
protected:
    QWord pos, size;
public:
    virtual ~StreamInterface() = 0;
    StreamInterface();
    virtual QWord getPos();
    virtual QWord getSize();
    virtual void setPos(QWord pos);
};

class OutputStreamInterface : public StreamInterface {
protected:
    QWord written_size;
public:
    virtual ~OutputStreamInterface() = 0;
    OutputStreamInterface();
    virtual bool write(Byte* buf, QWord size) = 0;
    virtual QWord getWrittenSize();
};

class InputStreamInterface : public StreamInterface {
protected:
    QWord read_size;
public:
    virtual ~InputStreamInterface() = 0;
    InputStreamInterface();
    virtual bool read(Byte* buf, QWord size) = 0;
    virtual QWord getReadSize();
};

class MemoryStream {
protected:
    Byte* mem;
};

// memory stream
class MemoryInputStream : public InputStreamInterface, MemoryStream {
public:
    MemoryInputStream(Byte* mem, QWord size);
    virtual bool read(Byte* buf, QWord size);
};

class MemoryOutputStream : public OutputStreamInterface, MemoryStream {
public:
    MemoryOutputStream(Byte* mem, QWord size);
    virtual bool write(Byte* buf, QWord size);
};

// file stream
class FileInputStream : public InputStreamInterface {
protected:
    ifstream* ifs;
    bool created;
public:
    FileInputStream(char* file_name);
    FileInputStream(ifstream* ifs);
    virtual ~FileInputStream();
    virtual bool read(Byte* buf, QWord size);
    virtual void setPos(QWord pos);
    virtual QWord getPos();
};

class FileOutputStream : public OutputStreamInterface {
protected:
    ofstream* ofs;
    bool created;
public:
    FileOutputStream(char* file_name);
    FileOutputStream(ofstream* ofs);
    virtual ~FileOutputStream();
    virtual bool write(Byte* buf, QWord size);
    virtual void setPos(QWord pos);
    virtual QWord getPos();
};

// codec stream
class CodecOutputStream : public OutputStreamInterface {
private:
    InputStreamInterface* input;
    OutputStreamInterface* output;
    CodecInterface* codec;
public:
    CodecOutputStream(InputStreamInterface* input, OutputStreamInterface* output,
        CodecInterface* codec);
    virtual void setPos(QWord pos);
    virtual QWord getPos();
    virtual QWord getSize();
    virtual bool write(Byte* buf, QWord size);
};

class CodecInputStream : public InputStreamInterface {
private:
    InputStreamInterface* input;
    OutputStreamInterface* output;
    CodecInterface* codec;
    Byte* mem;
public:
    CodecInputStream(InputStreamInterface* input, OutputStreamInterface* output,
        CodecInterface* codec);
    ~CodecInputStream();
    virtual QWord getSize();
    virtual QWord getPos();
    virtual bool read(Byte* buf, QWord size);
};

// file part stream
class FilePartInputStream : public FileInputStream {
public:
    FilePartInputStream(ifstream* ifs, QWord size);
};

class FilePartOutputStream : public FileOutputStream {
public:
    FilePartOutputStream(ofstream *ofs, QWord size);
};

}
#endif
