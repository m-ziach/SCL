////////////////////////////////////////////
// Small Compression Library              //
// author: Mariusz Ziach                  //
// www   : http://ziach.pl/               //
// date  : 2020                           //
////////////////////////////////////////////

#ifndef SCL_BITSTREAM_H
#define SCL_BITSTREAM_H

#include "Types.h"

namespace SCL {

// easy bit reading/writing used in huffman compression
class BitStream {
private:
    Byte *buf;
	int bit_pos, byte_pos;
public:
	BitStream();
	void assignBuffer(Byte *buf);
    // pos
	int  getBitPos();
	int  getBytePos();
	void setBitPos(int bit_pos);
	void setBytePos(int byte_pos);
	void resetPos();
    // write
	void writeBit(int bit);
	void writeBits(int bits, int count);
    // read
	int readBit();
	int readBits(int count);
};

} // namespace

#endif // SCL_BITSTREAM_H
