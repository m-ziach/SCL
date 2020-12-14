////////////////////////////////////////////
// Small Compression Library              //
// author: Mariusz Ziach                  //
// www   : http://ziach.pl/               //
// date  : 2020                           //
////////////////////////////////////////////

#ifndef SCL_FILELIST_H
#define SCL_FILELIST_H

// C++
#include <vector>
#include <thread>
#include <filesystem>
#include <locale>
#include <future>

// Archive
#include "Types.h"
#include "Utils.h"


using namespace std;
using namespace std::filesystem;

namespace SCL {

enum FileFlags { F_ISDIR = 1 };

struct FileHeader {
    Byte  flags;
    Word  file_name_length;
    QWord file_size;
    QWord file_compressed_size;
    DWord file_attributes;
    QWord file_creation_time;
    QWord file_modification_time;
    QWord file_access_time;
    DWord file_hash;
    QWord file_ID;
};

struct FileInfo {
    FileHeader file_header;
    wstring    absolute_file_name;
    wstring    relative_file_name;
};

class FileList {
protected:
    mutex list_mutex;
    future<void> thread_future;
    vector<FileInfo> file_list;
    vector<wstring>  file_names;
    QWord number_of_files;
    QWord number_of_folders;
    QWord size_of_all_files;
    void createFileListThread();
    void insertFile(wstring& absolute_file_name, wstring& relative_file_name, QWord& file_ID);

public:
    FileList();
    ~FileList();
    void createFileList(vector<wstring>& file_names);
    QWord writeFileList(ofstream& ofs);
    QWord readFileList(ifstream& ifs);
    vector<FileInfo>* getFileList();
    bool  isCompleted();
    void  waitUntilCompleted();
    QWord getNumberOfFiles();
    QWord getNumberOfFolders();
    QWord getSizeOfAllFiles();
    void  updateFiles(wstring &dir);
    void lockList();
    void unlockList();
};
}

#endif // SCL_FILELIST_H
