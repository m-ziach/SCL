////////////////////////////////////////////
// Small Compression Library              //
// author: Mariusz Ziach                  //
// www   : http://ziach.pl/               //
// date  : 2020                           //
////////////////////////////////////////////

#ifndef SCL_HUFFMAN_H
#define SCL_HUFFMAN_H

// Archive
#include "Types.h"
#include "Utils.h"
#include "BitStream.h"
#include "Streams.h"

namespace SCL {

// huffman tree node
class HuffmanTree {
public:
	HuffmanTree *left, *right;
    int symbol, freq;
	HuffmanTree();
	void clear();
	bool isLeaf();
};

// huffman code
class HuffmanCode {
public:
	int code, bit_count;
	void clear();
	HuffmanCode();
};

// symbol pair for building tree
class HuffmanSymbolPair {
public:
	HuffmanTree *first, *second;
	HuffmanSymbolPair();
};

// huffman compression algorithm
class Huffman : public CodecInterface {
private:
    int alphabet_size, nodes_array_size;
	HuffmanTree  *nodes, *parents, *root;
	HuffmanCode  *codes;
	BitStream    *bit_stream;
	Byte* uncompressed_bytes;
	Byte* compressed_bytes;
	void reset();
	void countFrequencies(Byte *buf, QWord in_size);
	void findLowestFreqSymbolPair(HuffmanSymbolPair &sp);
	void buildTree();
	void makeCodes(HuffmanTree *node, int code, int bit_count);
	void writeTree(HuffmanTree *node);
	HuffmanTree *readTree(HuffmanTree *node);
	int decodeSymbol(HuffmanTree *node);
public:
	Huffman();
	~Huffman();
    QWord compressStream(InputStreamInterface* rs, OutputStreamInterface* ws);
    QWord decompressStream(InputStreamInterface* rs, OutputStreamInterface* ws);
};

} // namespace

#endif // SCL_HUFFMAN_H
