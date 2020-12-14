////////////////////////////////////////////
// Small Compression Library              //
// author: Mariusz Ziach                  //
// www   : http://ziach.pl/               //
// date  : 2020                           //
////////////////////////////////////////////

#include "Huffman.h"

using namespace SCL;

// huffman tree node
HuffmanTree::HuffmanTree() { clear(); }
void HuffmanTree::clear() {
    symbol = freq = 0;
    left = right = nullptr;
}
bool HuffmanTree::isLeaf() {
	if (left == nullptr && right == nullptr) { return true; }
	return false;
}

// huffman code
void HuffmanCode::clear()  { code = 0; bit_count = 0; }
HuffmanCode::HuffmanCode() { clear(); }

// huffman symbol pair
HuffmanSymbolPair::HuffmanSymbolPair() { first = second = nullptr; }

// huffman main class
void Huffman::reset() {
	for (int i = 0; i < alphabet_size * 2; i++) {
		nodes[i].clear();
		nodes[i].symbol = i;
	}
	for (int i = 0; i < alphabet_size; i++) codes[i].clear();
	root = nullptr;
	parents = nodes + alphabet_size;
}

// create histogram
void Huffman::countFrequencies(Byte *buf, QWord in_size) {
	for (QWord i = 0; i < in_size; i++) nodes[buf[i]].freq++;
}

// sorting histogram
void Huffman::findLowestFreqSymbolPair(HuffmanSymbolPair &sp) {
	sp.first = sp.second = nullptr;
	int i = 0;
	while (i < nodes_array_size) {
		HuffmanTree *currentNode = nodes + i;
		if (currentNode->freq > 0) {
			if (sp.first == nullptr)       { sp.first  = currentNode; }
			else if (sp.second == nullptr) { sp.second = currentNode; }
			else if (sp.first->freq  > currentNode->freq) { sp.first  = currentNode; }
			else if (sp.second->freq > currentNode->freq) { sp.second = currentNode; }
		}
		i++;
	}
}

// tree building
void Huffman::buildTree() {
	while (true) {
		HuffmanSymbolPair sp;
		findLowestFreqSymbolPair(sp);
		if (sp.second == nullptr) {
			root = sp.first;
			break;
		} else {
			parents->freq   = sp.first->freq + sp.second->freq;
			parents->right  = sp.first;
			parents->left   = sp.second;
			sp.first ->freq = 0;
			sp.second->freq = 0;
			parents++;
		}
	}
}

// make huffan codes
void Huffman::makeCodes(HuffmanTree *node, int code, int bit_count) {
	if (node->isLeaf()) {
		codes[node->symbol].code = code;
		codes[node->symbol].bit_count = bit_count;
	}
	if (node->right != nullptr)
		makeCodes(node->right, code | (1 << bit_count), bit_count + 1);
	if (node->left != nullptr)
		makeCodes(node->left, code, bit_count + 1);
}

// writing tree to stream
void Huffman::writeTree(HuffmanTree *node) {
	if (node->isLeaf()) {
		bit_stream->writeBit(1);
		bit_stream->writeBits(node->symbol, 8);
	} else { bit_stream->writeBit(0); }
	if (node->right != nullptr) { writeTree(node->right); }
	if (node->left  != nullptr) { writeTree(node->left);  }
}

// reading tree from stream
HuffmanTree *Huffman::readTree(HuffmanTree *node) {
	int bit = bit_stream->readBit();
	if (bit == 1) {
		node->symbol = bit_stream->readBits(8);
		node->right  = node->left  = nullptr;
		node->freq   = 0;
		return node;
	} else if (bit == 0) {
		node->right = node + 1;
		node->left  = readTree(node->right) + 1;
		return readTree(node->left);
	}
	return nullptr;
}

// reading symbol from stream using huffman tree
int Huffman::decodeSymbol(HuffmanTree *node) {
	if (node->isLeaf()) { return node->symbol; }
	int bit = bit_stream->readBit();
	if      (bit == 1) { return decodeSymbol(node->right); }
	else if (bit == 0) { return decodeSymbol(node->left);  }
	else                 return 0;
}

// constructors/destructors
Huffman::Huffman() {
	alphabet_size = 256;
	nodes_array_size = alphabet_size * 2;
	nodes = new HuffmanTree[nodes_array_size];
	codes = new HuffmanCode[alphabet_size];
	bit_stream = new BitStream;
    uncompressed_bytes = new Byte[0xFFFF << 1];
    compressed_bytes   = new Byte[0xFFFF << 1];

}
Huffman::~Huffman() {
	if (nodes) delete[] nodes;
	if (codes) delete[] codes;
    if(uncompressed_bytes) delete[] uncompressed_bytes;
    if (compressed_bytes)  delete[] compressed_bytes;
    if (bit_stream)        delete bit_stream;
}

QWord Huffman::compressStream(InputStreamInterface* input, OutputStreamInterface* output) {
    QWord in_size = 0, out_size = 0;

    clock_t clock_begin = clock();
    CodecCallbackInfo callback_info = { 0, 0, 0, 0, 0, 0 };

    while (input->getPos() < input->getSize()) {

        // read data
        input->read(uncompressed_bytes, 0xFFFF);
        in_size = input->getReadSize();

        reset();

        // write input size to stream
        bit_stream->assignBuffer(compressed_bytes);

        // build and write tree
        countFrequencies(uncompressed_bytes, in_size);
        buildTree();
        makeCodes(root, 0, 0);
        writeTree(root);

        // for each byte write assigned code to output
        for (QWord i = 0; i < in_size; i++) {
            HuffmanCode* currentCode = codes + uncompressed_bytes[i];
            bit_stream->writeBits(currentCode->code, currentCode->bit_count);
        }

        // close last byte with 0 bits
        while (bit_stream->getBitPos() > 0) bit_stream->writeBit(0);

        // update info
        out_size   = bit_stream->getBytePos();

        // write data
        output->write((Byte*)&in_size,  sizeof(QWord));
        output->write((Byte*)&out_size, sizeof(QWord));
        output->write(compressed_bytes, out_size);

        // callback
        callback_info.in_pos    = input->getPos() == -1 ? input->getSize() : input->getPos();
        callback_info.in_size   = input->getSize();
        callback_info.out_size += out_size + (sizeof(QWord) * 2);
        callback_info.progress  = COUNTPRECENT(callback_info.in_pos, callback_info.in_size);
        callback_info.ratio     = COUNTPRECENT(callback_info.out_size, callback_info.in_pos);
        callback_info.clock     = COUNTTIME(clock_begin);

        if (this->callback) {
            memcpy_s(&this->callback->info, sizeof(CodecCallbackInfo),
                &callback_info, sizeof(CodecCallbackInfo));
            if (!this->callback->callback((input->getPos() < input->getSize()) ? CLT_PROGRESS : CLT_STREAM_FINISH))
                break;
        }
    }



    return callback_info.out_size;
}

QWord Huffman::decompressStream(InputStreamInterface* input, OutputStreamInterface* output) {
    QWord in_size = 0, out_size = 0;

    clock_t clock_begin = clock();
    CodecCallbackInfo callback_info = { 0, 0, 0, 0, 0, 0 };

    while (input->getPos() < input->getSize()) {

        input->read((Byte*)&out_size, sizeof(QWord));
        input->read((Byte*)&in_size,  sizeof(QWord));
        input->read(compressed_bytes, in_size);

        in_size = input->getReadSize();

        reset();

        // read decompressed size and tree
        bit_stream->assignBuffer(compressed_bytes);
        readTree(nodes);

        // decode each symbol
        for (int o = 0; o < out_size; o++)
            uncompressed_bytes[o] = Byte(decodeSymbol(nodes));

        output->write(uncompressed_bytes, out_size);

        callback_info.in_pos    = input->getPos() == -1 ? input->getSize() : input->getPos();
        callback_info.in_size   = in_size + (sizeof(QWord) * 2);
        callback_info.out_size += out_size;
        callback_info.progress  = COUNTPRECENT(callback_info.in_pos,   callback_info.in_size);
        callback_info.ratio     = COUNTPRECENT(callback_info.out_size, callback_info.in_pos);
        callback_info.clock     = COUNTTIME(clock_begin);

        if (this->callback) {
            memcpy_s(&this->callback->info, sizeof(CodecCallbackInfo),
                &callback_info, sizeof(CodecCallbackInfo));
            if (!this->callback->callback((input->getPos() < input->getSize()) ? CLT_PROGRESS : CLT_STREAM_FINISH))
                break;
        }
    }

    return callback_info.out_size;
}
