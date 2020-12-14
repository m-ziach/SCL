////////////////////////////////////////////
// Small Compression Library              //
// author: Mariusz Ziach                  //
// www   : http://ziach.pl/               //
// date  : 2020                           //
////////////////////////////////////////////

// Archive
#include "Streams.h"

namespace SCL {

// stream interfaces
StreamInterface::StreamInterface() {
    this->pos = 0;
    this->size = 0;
}
StreamInterface::~StreamInterface() {}

QWord StreamInterface::getSize() {
    return this->size;
}

QWord StreamInterface::getPos() {
    return this->pos;
}

void StreamInterface::setPos(QWord pos) {
    this->pos = pos;
}

// writer stream
OutputStreamInterface::~OutputStreamInterface() {}

OutputStreamInterface::OutputStreamInterface() {
    this->written_size = 0;
}

QWord OutputStreamInterface::getWrittenSize() {
    return this->written_size;
}

// reader stream
InputStreamInterface::~InputStreamInterface() {}

InputStreamInterface::InputStreamInterface() {
    this->read_size = 0;
}

QWord InputStreamInterface::getReadSize() {
    return this->read_size;
}

// memory reader
MemoryInputStream::MemoryInputStream(Byte* mem, QWord size) {
    this->mem = mem;
    this->size = size;
}

bool MemoryInputStream::read(Byte* buf, QWord size) {
    QWord i;
    for (i = 0; i < size; i++) {
        if (this->pos + i >= this->size) {
            break;
        }
        buf[i] = this->mem[this->pos + i];
    }
    this->read_size = i;
    this->pos += i;
    return true;
}

// memory writer
MemoryOutputStream::MemoryOutputStream(Byte* mem, QWord size) {
    this->mem  = mem;
    this->size = size;
}

bool MemoryOutputStream::write(Byte* buf, QWord size) {
    QWord i;
    for (i = 0; i < size; i++) {
        if (this->pos + i >= this->size)  break;
        this->mem[this->pos + i] = buf[i];
    }

    this->written_size = i;
    this->pos += this->written_size;

    return true;
}

// file reader
FileInputStream::FileInputStream(char* file_name) {
    this->ifs = new ifstream(file_name, ifstream::binary);
    this->ifs->seekg(0, ios::end);
    this->size = (DWord)this->ifs->tellg();
    this->ifs->seekg(0, ios::beg);
    this->created = true;
}

FileInputStream::FileInputStream(ifstream* ifs) {
    this->ifs = ifs;
    streampos temp = this->ifs->tellg();
    this->ifs->seekg(0, ios::end);
    this->size = (DWord)this->ifs->tellg();
    this->ifs->seekg(temp, ios::beg);
    this->created = false;
}

FileInputStream::~FileInputStream() {
    if (this->created)
        delete this->ifs;
}

bool FileInputStream::read(Byte* buf, QWord size) {
    this->ifs->read((char*)buf, size);
    this->read_size = (QWord)this->ifs->gcount();
    this->pos = (QWord)this->ifs->tellg();
    return true;
}

void FileInputStream::setPos(QWord pos) {
    this->ifs->clear();
    this->ifs->seekg(pos, ios::beg);
    this->pos = (QWord)this->ifs->tellg();
}

QWord FileInputStream::getPos() {
    this->pos = (QWord)this->ifs->tellg();
    return this->pos;
}

// file writer
FileOutputStream::FileOutputStream(char* file_name) {
    this->ofs = new ofstream(file_name, ofstream::binary);
    this->created = true;
}

FileOutputStream::FileOutputStream(ofstream* ofs) {
    this->ofs = ofs;
    this->created = false;
}

FileOutputStream::~FileOutputStream() {
    if (this->created)
        delete this->ofs;
}

bool FileOutputStream::write(Byte* buf, QWord size) {
    QWord before = (QWord)this->ofs->tellp();
    this->ofs->write((char*)buf, size);
    this->pos = (QWord)this->ofs->tellp();
    this->written_size = this->pos - before;

    this->ofs->seekp(0, ios::end);
    this->size = (QWord)this->ofs->tellp();
    this->ofs->seekp(this->pos);

    return true;
}

void FileOutputStream::setPos(QWord pos) {
    this->ofs->clear();
    this->ofs->seekp(pos, ios::beg);
    this->pos = (DWord)this->ofs->tellp();
}

QWord FileOutputStream::getPos() {
    this->pos = (DWord)this->ofs->tellp();
    return this->pos;
}

// codec stream
CodecOutputStream::CodecOutputStream(InputStreamInterface* input,
    OutputStreamInterface* output, CodecInterface* codec) {
    this->input = input;
    this->output = output;
    this->codec = codec;
}

void CodecOutputStream::setPos(QWord pos) {
    output->setPos(pos);
}

QWord CodecOutputStream::getPos() {
    return output->getPos();
}

QWord CodecOutputStream::getSize() {
    return input->getSize();
}

bool CodecOutputStream::write(Byte* buf, QWord size) {
    MemoryInputStream mrs(buf, size);
    
    // temp write size
    QWord temp0 = output->getPos();
    output->write((Byte*)&size, sizeof(QWord));
    QWord temp1 = output->getPos();
    
    // compress
    codec->compressStream(&mrs, output);
    
    // write real size
    QWord temp2 = output->getPos();
    QWord csize = (temp2 - temp1);
    output->setPos(temp0);
    output->write((Byte*)&csize, sizeof(QWord));
    output->setPos(temp2);
    
    this->size = output->getSize();
    this->pos = output->getPos();
    this->written_size = csize;

    return true;
}

CodecInputStream::CodecInputStream(InputStreamInterface* input, OutputStreamInterface* output,
    CodecInterface* codec) {
    this->input = input;
    this->output = output;
    this->codec = codec;
    this->mem = new Byte[0xFFFF << 1];
    this->size = input->getSize();
    this->pos = 0;
}

CodecInputStream::~CodecInputStream() {
    delete[] mem;
}

QWord CodecInputStream::getSize() {
    return this->size;
}

QWord CodecInputStream::getPos() {
    return this->pos;
}

bool CodecInputStream::read(Byte* buf, QWord size) {
    QWord csize(0);
    input->read((Byte*)&csize, sizeof(QWord));
    input->read((Byte*)mem, csize);

    MemoryInputStream  mem_in(mem, csize);
    MemoryOutputStream mem_out(buf, size);
    //printf("[isize: %lli;; size: %lli]\n", input->getSize(), size);

    this->read_size = codec->decompressStream(&mem_in, &mem_out);
    this->pos += csize + sizeof(QWord);

    return true;
}

// file part stream
FilePartInputStream::FilePartInputStream(ifstream* ifs, QWord size) : FileInputStream(ifs) {
    this->size = size;
}

FilePartOutputStream::FilePartOutputStream(ofstream* ofs, QWord size) : FileOutputStream(ofs) {
    this->size = size;
}

}
