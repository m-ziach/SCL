////////////////////////////////////////////
// Small Compression Library              //
// author: Mariusz Ziach                  //
// www   : http://ziach.pl/               //
// date  : 2020                           //
////////////////////////////////////////////

// LHZX
#include "LZ.h"
#include "Utils.h"

using namespace SCL;

// convert bit values to byte values and masks
// TODO: check settings if they are supported by codec
void LZCodecSettings::Set( DWord blc, DWord blh,
    DWord bml, DWord bmp, DWord br) {

    // lookup table capacity
    this->bit_lkp_cap = blc;
    byte_lkp_cap = 1 << blc;
    mask_lkp_cap = byte_lkp_cap - 1;

    // how many bytes are used to count hash for lookup table key
    this->bit_lkp_hsh = blh;
    byte_lkp_hsh = 1 << blh;
    mask_lkp_hsh = byte_lkp_hsh - 1;

    // max LZ match size
    this->bit_mtch_len = bml;
    byte_mtch_len = 1 << bml;
    mask_mtch_len = byte_mtch_len - 1;

    // max LZ match pos
    this->bit_mtch_pos = bmp;
    byte_mtch_pos = 1 << bmp;
    mask_mtch_pos = byte_mtch_pos - 1;

    // how many times LZ searching procedure is trying to find best match
    this->bit_runs = br;
    byte_runs = 1 << br;
    mask_runs = byte_runs - 1;

}

// dictionary mem
LZDictionaryBuffer::~LZDictionaryBuffer() { if (arr) delete[] arr; }
LZDictionaryBuffer::LZDictionaryBuffer(QWord cap) {
    this->cap = cap;
    arr = new Byte[cap];
    clear();
}
// clear
void LZDictionaryBuffer::clear() {
    pos = size = 0;
    for (int i = 0; i < cap; i++) arr[i] = 0;
}
// insert byte into ring mem
Byte *LZDictionaryBuffer::putByte(Byte val) {
    if (pos >= cap) pos = 0;
    if (size < cap) size++;
    arr[pos] = val;
    return &arr[pos++];
}
// convert position absolute to relative and conversely
QWord LZDictionaryBuffer::convPos(bool abs_to_rel, QWord p) {
    if (abs_to_rel) {
        if (this->pos >= p) return  this->pos - p;
        else                return (this->cap - p) + this->pos;
    } else {
        if (this->pos >= p) return this->pos -  p;
        else                return this->cap - (p - this->pos);
    }
}
Byte LZDictionaryBuffer::getByte(QWord p) {
    if (p >= size) return 0;
    return arr[p];
}
QWord  LZDictionaryBuffer::getPos() { return pos; }

// lz match
LZMatch::LZMatch()    { clear(); }
void LZMatch::clear() {pos = len = 0; }
void LZMatch::copy(LZMatch *lzm) {
    pos = lzm->pos;
    len = lzm->len;
}

// lz dictionary node - item of lookup table - first item of linked list
LZDictionaryNode::LZDictionaryNode() {
    next = prev = nullptr; clear();
}
void LZDictionaryNode::clear() {
    if (next != nullptr) next->prev = nullptr;
    if (prev != nullptr) prev->next = nullptr;
    next = prev = nullptr;
    valid = false;
    pos = 0;
}

// match finder
LZMatchFinder::LZMatchFinder(LZCodecSettings *cdc_sttgs) {
    this->lkp_tab    = new LZDictionaryNode[cdc_sttgs->byte_lkp_cap ];  
    this->dict_tab   = new LZDictionaryNode[cdc_sttgs->byte_mtch_pos];
    this->best_match = new LZMatch; 
    this->cdc_sttgs  = cdc_sttgs;
    clear();
}
LZMatchFinder::~LZMatchFinder() {
    if (this->lkp_tab)    delete[] this->lkp_tab;
    if (this->dict_tab)   delete[] this->dict_tab;
    if (this->best_match) delete this->best_match;
}
void LZMatchFinder::clear() {
    this->lz_buf = nullptr;
    this->buf    = nullptr;
    this->buf_size = 0;
    this->dict_i   = 0;
    for (int i = 0; i < (int)cdc_sttgs->byte_lkp_cap; i++)  lkp_tab[i].clear();
    for (int i = 0; i < (int)cdc_sttgs->byte_mtch_pos; i++) dict_tab[i].clear();
}

void LZMatchFinder::assignBuffer(Byte *b, QWord bs, LZDictionaryBuffer *lzb) {
    this->buf      = b;
    this->buf_size = bs;
    this->lz_buf   = lzb;
}
// FNV hash
QWord LZMatchFinder::hash(Byte *in) {
    DWord hash = 0x811C9DC5;
    for (int i = 0; i < int(cdc_sttgs->byte_lkp_hsh); i++) {
        hash ^= in[i];
        hash *= 0x1000193;
    }
    return hash & cdc_sttgs->mask_lkp_cap;
}
// insert new item into dictionary
void LZMatchFinder::insert(QWord pos) {
    if (buf_size - pos  <= int(cdc_sttgs->byte_lkp_hsh )) return;
    if (dict_i >= int(cdc_sttgs->byte_mtch_pos))
        dict_i = 0;
    LZDictionaryNode *current = lkp_tab   + hash(buf + pos);
    LZDictionaryNode *empty   = dict_tab + dict_i++;
    empty->clear();
    empty->pos     = current->pos;
    empty->next    = current->next;
    if (empty->next) empty->next->prev = empty;
    empty->prev    = current;
    empty->valid   = true;
    current->pos   = lz_buf->getPos();
    current->next  = empty;
    current->prev  = nullptr;
    current->valid = true;
}

// find item uncompressed_bytes dictionary
LZMatch *LZMatchFinder::find(QWord pos) {
    DWord runs, i;
    LZDictionaryNode *current;

    best_match->clear();
    if (buf_size - pos <= (cdc_sttgs->byte_lkp_hsh) || pos == 0) return best_match;
    current = lkp_tab + hash(buf + pos);
    runs    = 0;
    while (current && (runs++ < (cdc_sttgs->byte_runs))) {
        if (current->valid) {
            i = 0;
            while (true) {
                if (pos + i >= buf_size - 1 || current->pos + i >= lz_buf->size - 1) break;
                if (i >= (cdc_sttgs->mask_mtch_len) - 1) break;
                if (buf[pos + i] != lz_buf->getByte(current->pos + i)) break;
                i++;
            }
            if (best_match->len < i) {
                best_match->pos = current->pos;
                best_match->len = i;
            }
        }
        current = current->next;
    }
    return best_match;
}

// lz algorith main class
LZ::LZ(LZCompressionLevel comp_level) {
    switch (comp_level) {
    case LCL_FASTEST:
    case LCL_FAST:
    case LCL_NORMAL:
    case LCL_BEST:
    default:
        cdc_sttgs.Set(16, 2, 8, 16, 4);
        cdc_sttgs.byte_lkp_hsh = 5;
        break;
    }
    lz_mf = new LZMatchFinder(&cdc_sttgs);
    lz_buf = new LZDictionaryBuffer(cdc_sttgs.byte_mtch_pos);
    uncompressed_bytes = new Byte[0xFFFF << 1];
    for (int i = 0; i < LZ_NUMBER_OF_STREAMS; i++)
        compressed_bytes[i] = new Byte[0xFFFF << 1];
}

LZ::~LZ() {
    if (lz_mf)  delete lz_mf;
    if (lz_buf) delete lz_buf;
    if (uncompressed_bytes) delete[] uncompressed_bytes;
    for (int i = 0; i < LZ_NUMBER_OF_STREAMS; i++) {
        if (compressed_bytes[i]) delete[] compressed_bytes[i];
    }

}

// get settings
LZCodecSettings* LZ::getSettings() {
    return &this->cdc_sttgs;
}

QWord LZ::compressStream(InputStreamInterface* input, OutputStreamInterface* output) {
    clock_t clock_begin = clock();
    CodecCallbackInfo callback_info = {0, 0, 0, 0, 0, 0};

    while (input->getPos() < input->getSize()) {

        QWord o[LZ_NUMBER_OF_STREAMS], i = 0;
        // 0 -> instructions; 1 -> pos; 2 -> len; 3 ->literal;
        for (int i = 0; i < LZ_NUMBER_OF_STREAMS; i++)
            o[i] = 0;

        // input stream after compression will be empty
        input->read(uncompressed_bytes, 0xFFFF);
        QWord in_size = input->getReadSize();

        lz_mf->clear();
        lz_buf->clear();

        // assign mem to match finder, write input size int 1 stream
        lz_mf->assignBuffer(uncompressed_bytes, in_size, lz_buf);
        
        match_empty.clear();
        lz_match = &match_empty;

        while (i < in_size) {

            // search for best match
            if (i + int(cdc_sttgs.byte_lkp_hsh) >= in_size) {
                lz_match->len = 0;
            } else {
                lz_match = lz_mf->find(i);
            }

            if ((lz_match->len > LZ_MIN_MATCH) &&
                QWord(lz_match->len + lz_match->pos) < in_size) {

                // write match to stream
                lz_match->pos = lz_buf->convPos(true, lz_match->pos);

                if (lz_match->pos < 256) {
                    compressed_bytes[LZ_INSTRUCTION][o[LZ_INSTRUCTION]++] = (Byte)(LZ_WRITEMATCH8);
                    compressed_bytes[LZ_MATCH_LEN]  [o[LZ_MATCH_LEN]  ++] = (Byte)(lz_match->len);
                    compressed_bytes[LZ_MATCH_POS]  [o[LZ_MATCH_POS]  ++] = (Byte)(lz_match->pos);
                }  else {
                    compressed_bytes[LZ_INSTRUCTION][o[LZ_INSTRUCTION]++] = (Byte)(LZ_WRITEMATCH16);
                    compressed_bytes[LZ_MATCH_LEN]  [o[LZ_MATCH_LEN]  ++] = (Byte)(lz_match->len);
                    o[LZ_MATCH_POS] += write16To8Buf(compressed_bytes[LZ_MATCH_POS] + o[LZ_MATCH_POS], Word(lz_match->pos));
                }

                // add skipped bytes into dictionary
                while (lz_match->len--) {
                    lz_mf->insert(i);
                    lz_buf->putByte(uncompressed_bytes[i++]);
                }
            } else {
                if (uncompressed_bytes[i] != LZ_WRITEMATCH8 &&
                    uncompressed_bytes[i] != LZ_WRITEMATCH16 &&
                    uncompressed_bytes[i] != LZ_WRITECHAR) {
                    compressed_bytes[LZ_INSTRUCTION][o[LZ_INSTRUCTION]++] = uncompressed_bytes[i];
                } else {

                    // if previous instruction was 'write byte' then we will just increment length of bytes to write
                    if (compressed_bytes[LZ_INSTRUCTION][o[LZ_INSTRUCTION] - 1] == (LZ_WRITECHAR) &&
                        compressed_bytes[LZ_MATCH_LEN][o[LZ_MATCH_LEN] - 1] < 255) {

                        // increment number of bytes to write
                        compressed_bytes[LZ_MATCH_LEN]  [o[LZ_MATCH_LEN]   - 1]++; 
                    } else {

                        // 'write byte' instruction
                        compressed_bytes[LZ_INSTRUCTION][o[LZ_INSTRUCTION]++] = (Byte)(LZ_WRITECHAR);

                        // write one byte
                        compressed_bytes[LZ_MATCH_LEN]  [o[LZ_MATCH_LEN]  ++] = 1;
                    }
                    compressed_bytes[LZ_CHAR][o[LZ_CHAR]++] = (Byte)(uncompressed_bytes[i]);
                }

                // add byte into dictionary
                lz_mf->insert(i);
                lz_buf->putByte(uncompressed_bytes[i++]);
            }
        }

        output->write((Byte*)&in_size, sizeof(QWord));
        for (int j = 0; j < LZ_NUMBER_OF_STREAMS; j++) {
            output->write((Byte*)&o[j], sizeof(QWord));
            output->write(compressed_bytes[j], o[j]);

            callback_info.out_size += o[j] + sizeof(QWord) + sizeof(QWord) * LZ_NUMBER_OF_STREAMS;
        }

        // callback
        callback_info.in_pos    = input->getPos() == -1 ? input->getSize() : input->getPos();
        callback_info.in_size   = input->getSize();
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

QWord LZ::decompressStream(InputStreamInterface* input, OutputStreamInterface* output) {
    clock_t clock_begin = clock();
    CodecCallbackInfo callback_info = { 0, 0, 0, 0, 0, 0 };

    while (input->getPos() < input->getSize()) {

        QWord i[LZ_NUMBER_OF_STREAMS] = { 0,0,0,0 }, out_size(0), in_size[LZ_NUMBER_OF_STREAMS] = { 0, 0, 0, 0 }, total_in_size(0);

        lz_buf->clear();
        input->read((Byte*)&out_size, sizeof(QWord));

        for (int j = 0; j < LZ_NUMBER_OF_STREAMS; j++) {
            input->read((Byte*)&in_size[j], sizeof(QWord));
            input->read(compressed_bytes[j], in_size[j]);
            i[j] = 0;
            total_in_size += in_size[j];
        }

        QWord o = 0;
        while (o < out_size) {
            Byte c = compressed_bytes[LZ_INSTRUCTION][i[LZ_INSTRUCTION]++];

            // what to do?
            if (c == LZ_WRITEMATCH8 || c == LZ_WRITEMATCH16) {
                QWord pos(0), len(0);

                // read match
                if (c == LZ_WRITEMATCH16) {
                    pos = read16From8Buf(compressed_bytes[LZ_MATCH_POS] + i[LZ_MATCH_POS]);
                    i[LZ_MATCH_POS] += sizeof(Word);
                }
                else if (c == LZ_WRITEMATCH8) {
                    pos = compressed_bytes[LZ_MATCH_POS][i[LZ_MATCH_POS]++];
                }

                pos = lz_buf->convPos(false, pos);
                len = compressed_bytes[LZ_MATCH_LEN][i[LZ_MATCH_LEN]++];
                
                // copy match
                for (DWord j = 0; j < len; j++) {
                    Byte b = lz_buf->getByte(pos + j);
                    uncompressed_bytes[o++] = b;
                }

                // insert processed bytes into dictionary
                for (DWord j = 0; j < len; j++)  lz_buf->putByte(uncompressed_bytes[(o-len)+j]);

            } else if (c == LZ_WRITECHAR) {
                // read uncompressed_bytes bytes
                int len = compressed_bytes[LZ_MATCH_LEN][i[LZ_MATCH_LEN]++];
                for (int j = 0; j < len; j++) {
                    lz_buf->putByte(compressed_bytes[LZ_CHAR][i[LZ_CHAR]]);
                    uncompressed_bytes[o++] = compressed_bytes[LZ_CHAR][i[LZ_CHAR]++];
                }
            }
            else {
                lz_buf->putByte(c);
                uncompressed_bytes[o++] = c;
            }
        }

        output->write(uncompressed_bytes, o);

        // callback
        callback_info.in_pos   = input->getPos() == -1 ? input->getSize() : input->getPos();
        callback_info.in_size  = total_in_size + sizeof(QWord) + sizeof(QWord) * LZ_NUMBER_OF_STREAMS;
        callback_info.out_size += o;
        callback_info.progress = COUNTPRECENT(callback_info.in_pos, callback_info.in_size);
        callback_info.ratio    = COUNTPRECENT(callback_info.out_size, callback_info.in_pos);
        callback_info.clock    = COUNTTIME(clock_begin);

        if (this->callback) {
            memcpy_s(&this->callback->info, sizeof(CodecCallbackInfo),
                &callback_info, sizeof(CodecCallbackInfo));
            if (!this->callback->callback((input->getPos() < input->getSize()) ? CLT_PROGRESS : CLT_STREAM_FINISH))
                break;
        }
    }
    return callback_info.out_size;
}
