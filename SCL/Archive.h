////////////////////////////////////////////
// Small Compression Library              //
// author: Mariusz Ziach                  //
// www   : http://ziach.pl/               //
// date  : 2020                           //
////////////////////////////////////////////

#ifndef SCL_SCL_H
#define SCL_SCL_H

// stl
#include <fstream>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <memory>
#include <vector>

// c
#include <cassert>
#include <cstdlib>

// Archive
#include "Types.h"
#include "Utils.h"
#include "LZHuffman.h"
#include "Hashing.h"
#include "FileList.h"

using namespace std;

namespace SCL {

// archive header signature
const Byte   SCL_ARCHIVE_SIGNATURE0[4] = { 'L', 'Z', 'H', 'X' };
const DWord  SCL_ARCHIVE_SIGNATURE1    = 0x7FFFFFFF;

// enums
enum ArchiveDetectResult    { AD_UNKNOWN = 100, AD_CREATE  = 101, AD_EXTRACT = 102 };

// archive header
struct ArchiveHeader {
    Byte  signature0[4];
    DWord signature1;
    DWord flags;
    QWord file_list_pos;
};
// callback structure
struct ArchiveCallbackInfo {
    int            file_ratio;
    int            archive_ratio;
    int            file_precent;
    int            archive_precent;
    int            file_clock;
    int            archive_clock;
    QWord          number_of_files;
    QWord          file_read_bytes;
    QWord          file_written_bytes;
    QWord          archive_read_bytes;
    QWord          archive_written_bytes;
    wstring        file_name;
    wstring        file_path;
    wstring        archive_file;
    wstring        archive_path;
    wstring        output_path;
    FileTime       file_creation_time;
    FileTime       file_modification_time;
    FileTime       file_access_time;
    DWord          file_hash;
    CallbackAction callback_action;
    QWord          file_ID;
};

// codec callback interface for monitoring compression progress
class ArchiveCallbackInterface {
public:
    ArchiveCallbackInfo info;
    ArchiveCallbackInterface();
    virtual ~ArchiveCallbackInterface() = 0;
    virtual bool callback(CallbackType callback_type) = 0;
};

// callback for codecs
class CodecCallback : public CodecCallbackInterface {
private:
    ArchiveCallbackInterface* archive_callback;
    QWord   total_bytes_read;
    QWord   total_bytes_written;
    QWord   total_bytes_to_process;
    clock_t archive_clock_start;
public:
    CodecCallback(ArchiveCallbackInterface* archive_callback);
    void init();
    bool callback(CallbackType callback_type);
    void setTotalBytesToProcess(QWord i);
};

// main class
class Archive {
private:
    CodecInterface            *codec;
    CodecCallback             *codec_callback;
    ArchiveCallbackInterface  *archive_callback;
    HashingInterface          *hashing;
    FileList                  *file_list;
    ArchiveHeader              archive_header;

    // compress/decompress file
    bool compressFile  (ifstream &ifile, ofstream &ofile, FileInfo& file_info);
    bool decompressFile(ifstream &ifile, ofstream &ofile, FileInfo& file_info);
    
    // read/write archive header
    void writeArchiveHeader(ofstream& ofile);
    bool readArchiveHeader (ifstream& ifile);
    
    // update callback
    void updateCallbackInfo(FileInfo &file_info, wstring& archive_name, wstring& output_path);
    void updateCallbackInfo(FileInfo& file_info);

public:
    Archive();
    ~Archive();

    // set callback
    void setCallback(ArchiveCallbackInterface *archive_callback);

    // create archive from directory or file
    bool archiveCreate(vector<wstring> &files, wstring &archive_name);

    // archive extracting function
    bool archiveExtract(wstring &archive_name, wstring &extract_dir);

    // detect if input is a file, folder or archive 
    ArchiveDetectResult detectInput(wstring &input_name);
};
}

#endif // SCL_SCL_H
