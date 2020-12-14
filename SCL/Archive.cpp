////////////////////////////////////////////
// Small Compression Library              //
// author: Mariusz Ziach                  //
// www   : http://ziach.pl/               //
// date  : 2020                           //
////////////////////////////////////////////

#include "Archive.h"

// namespaces
using namespace std;
using namespace std::filesystem;

namespace SCL {

ArchiveCallbackInterface::~ArchiveCallbackInterface() {}

ArchiveCallbackInterface::ArchiveCallbackInterface() {
    info.callback_action       = CLA_NOACTION;
    info.number_of_files       = 0;
    info.file_read_bytes       = 0;
    info.file_written_bytes    = 0;
    info.archive_read_bytes    = 0;
    info.archive_written_bytes = 0;
    info.file_ratio            = 0;
    info.archive_ratio         = 0;
    info.file_precent          = 0;
    info.archive_precent       = 0;
    info.file_clock            = 0;
    info.archive_clock         = 0;
    info.file_ID               = 0;
    info.file_name   .clear();
    info.file_path   .clear();
    info.archive_path.clear();
    info.archive_file.clear();
    info.output_path .clear();
}

bool CodecCallback::callback(CallbackType callback_type) {
    if (!archive_callback) return true;
    archive_callback->info.file_read_bytes       = info.in_pos;
    archive_callback->info.file_written_bytes    = info.out_size;
    archive_callback->info.file_ratio            = info.ratio;
    archive_callback->info.file_precent          = info.progress;
    archive_callback->info.file_clock            = info.clock;
    archive_callback->info.archive_read_bytes    = total_bytes_read    + info.in_pos;
    archive_callback->info.archive_written_bytes = total_bytes_written + info.out_size;

    if (archive_callback->info.callback_action == CLA_COMPRESS) {
        archive_callback->info.archive_precent  = COUNTPRECENT(archive_callback->info.archive_read_bytes, total_bytes_to_process);
        archive_callback->info.archive_ratio    = COUNTPRECENT(archive_callback->info.archive_written_bytes,
            archive_callback->info.archive_read_bytes);
    } else if (archive_callback->info.callback_action == CLA_DECOMPRESS) {
        archive_callback->info.archive_precent  = COUNTPRECENT(archive_callback->info.archive_written_bytes, total_bytes_to_process);
        archive_callback->info.archive_ratio    = COUNTPRECENT(archive_callback->info.archive_read_bytes,
            archive_callback->info.archive_written_bytes);
    }

    archive_callback->info.archive_clock         = COUNTTIME(archive_clock_start);
    if (callback_type == CLT_STREAM_FINISH) {
        total_bytes_read    += info.in_size;
        total_bytes_written += info.out_size;
    }
    return archive_callback->callback(CLT_PROGRESS);
}

void CodecCallback::init() {
    this->total_bytes_read       = 0;
    this->total_bytes_written    = 0;
    this->total_bytes_to_process = 0;
    this->archive_clock_start    = clock();
}

void CodecCallback::setTotalBytesToProcess(QWord i) {
    this->total_bytes_to_process = i;
}

CodecCallback::CodecCallback(ArchiveCallbackInterface* archive_callback) {
    this->archive_callback       = archive_callback;
    this->total_bytes_read       = 0;
    this->total_bytes_written    = 0;
    this->total_bytes_to_process = 0;
}

// constructor
Archive::Archive() {
    codec               = new LZHuffman;
    hashing             = new Hashing;
    file_list           = new FileList;
    codec_callback      = nullptr;
}

// descrutor
Archive::~Archive() {
    if (codec)          delete codec;
    if (hashing)        delete hashing;
    if (codec_callback) delete codec_callback;
    if (file_list)      delete file_list;
}

// callback
void Archive::setCallback(ArchiveCallbackInterface *archive_callback) {
    if (this->codec_callback) delete this->codec_callback;
    this->codec_callback = new CodecCallback(archive_callback);
    this->codec->setCallback(this->codec_callback);
    this->archive_callback = archive_callback;
}

// compress file
bool Archive::compressFile(ifstream &ifile, ofstream &ofile, FileInfo& file_info) {
    // callback
    if (!archive_callback->callback(CLT_FILE_BEGIN)) return false;

    // hash -> compress
    FileInputStream      icodec(&ifile);
    FilePartOutputStream ocodec(&ofile, file_info.file_header.file_size);
    hashing->init();
    hashing->hashStream(&icodec);
    file_info.file_header.file_hash            = hashing->getHash();
    updateCallbackInfo(file_info);
    file_info.file_header.file_compressed_size = codec->compressStream(&icodec, &ocodec);
    
    // final callback
    if (!archive_callback->callback(CLT_FILE_FINISH)) return false;
    return true;
}

// decompress file
bool Archive::decompressFile(ifstream &ifile, ofstream &ofile, FileInfo& file_info) {
    if (!archive_callback->callback(CLT_FILE_BEGIN)) return false;

    // decompress -> hash
    FilePartInputStream icodec(&ifile, file_info.file_header.file_compressed_size);
    FileOutputStream    ocodec(&ofile);
    HashingOutputStream hashing_ocodec(hashing, &ocodec);
    hashing->init();
    codec->decompressStream(&icodec, &hashing_ocodec);

    // wrong hash
    if (file_info.file_header.file_hash != hashing->getHash()) return false;

    // final callback
    if (!archive_callback->callback(CLT_FILE_FINISH)) return false;
    return true;
}

// write archive header
void Archive::writeArchiveHeader(ofstream &ofile) {
    archive_header.signature1 = SCL_ARCHIVE_SIGNATURE1;
    memcpy(archive_header.signature0, SCL_ARCHIVE_SIGNATURE0, sizeof(SCL_ARCHIVE_SIGNATURE0));
    ofile.write((char*)(&archive_header), sizeof(ArchiveHeader));
}

// read archive header
bool Archive::readArchiveHeader(ifstream &ifile) {
    ifile.read((char*)(&archive_header), sizeof(ArchiveHeader));
    if (ifile.gcount() != sizeof(ArchiveHeader)) {
        return false;
    }
    // check signature
    return (memcmp(archive_header.signature0, SCL_ARCHIVE_SIGNATURE0, sizeof(SCL_ARCHIVE_SIGNATURE0)) == 0 &&
                   archive_header.signature1 == SCL_ARCHIVE_SIGNATURE1);
}

// update callback info
void Archive::updateCallbackInfo(FileInfo &file_info, wstring &archive_name, wstring &output_path) {
    path file_name_path(file_info.relative_file_name);
    path archive_name_path(archive_name);

    archive_callback->info.file_name    = file_name_path.filename();
    archive_callback->info.file_path    = file_name_path.parent_path();
    archive_callback->info.archive_file = archive_name_path.filename();
    archive_callback->info.archive_path = archive_name_path.parent_path();
    archive_callback->info.output_path  = output_path;
    updateCallbackInfo(file_info);
}
void Archive::updateCallbackInfo(FileInfo& file_info) {
    archive_callback->info.file_ID             = file_info.file_header.file_ID;
    archive_callback->info.file_hash           = file_info.file_header.file_hash;
    archive_callback->info.file_creation_time    .set(file_info.file_header.file_creation_time);
    archive_callback->info.file_access_time      .set(file_info.file_header.file_access_time);
    archive_callback->info.file_modification_time.set(file_info.file_header.file_modification_time);
}

// create archive from directory or file
bool Archive::archiveCreate(vector<wstring> &files, wstring &archive_name) {
    // init callback
    codec_callback->init();
    archive_callback->info.callback_action = CLA_COMPRESS;

    // create file lsit
    file_list->createFileList(files);

    while (!file_list->isCompleted()) {
        archive_callback->info.number_of_files = file_list->getFileList()->size();
        if (!archive_callback->callback(CLT_COUNTING_FILES)) return false;
    }

    // callbacks
    codec_callback->setTotalBytesToProcess(file_list->getSizeOfAllFiles());
    archive_callback->info.number_of_files = file_list->getNumberOfFiles() + file_list->getNumberOfFolders();
    if (!archive_callback->callback(CLT_ARCHIVE_BEGIN)) return false;

    // open archive file and write header
    ofstream archive_file(archive_name, ios::binary);
    memset(&archive_header, 0, sizeof(ArchiveHeader));
    writeArchiveHeader(archive_file);

    // process files
    vector<FileInfo> *list = file_list->getFileList();
    for (FileInfo& file_info : *list) {

        // skip directory
        if (file_info.file_header.flags & F_ISDIR) continue;

        // update callback info
        wstring out_dir = path(archive_name).parent_path();
        updateCallbackInfo(file_info, archive_name, out_dir);

        // compress files
        ifstream ifile(file_info.absolute_file_name, ios::binary);
        if (!compressFile(ifile, archive_file, file_info)) return false;
    }
    
    // write file list
    archive_header.file_list_pos = archive_file.tellp();
    file_list->writeFileList(archive_file);

    // write header again
    archive_file.seekp(0);
    writeArchiveHeader(archive_file);
    if (!archive_callback->callback(CLT_ARCHIVE_FINISH)) return false;
    return true;
}

// archive extracting function
bool Archive::archiveExtract(wstring &archive_name, wstring &extract_dir) {
    ifstream archive_file(archive_name, ios::binary);

    // read header
    if (!readArchiveHeader(archive_file)) return false;

    // read file list
    archive_file.seekg(archive_header.file_list_pos);
    file_list->readFileList(archive_file);
    vector<FileInfo>* list = file_list->getFileList();

    // return to data position
    archive_file.seekg(sizeof(ArchiveHeader));

    // callbacks
    archive_callback->info.callback_action = CLA_DECOMPRESS;
    archive_callback->info.number_of_files = file_list->getNumberOfFiles() + file_list->getNumberOfFolders();
    codec_callback->init();
    codec_callback->setTotalBytesToProcess(file_list->getSizeOfAllFiles());
    if (!archive_callback->callback(CLT_ARCHIVE_BEGIN)) return false;

    // process files
    create_directories(extract_dir);
    for (FileInfo& file_info : *list) {
        wstring absolute_file_name = (wstring)(path(extract_dir) / path(file_info.relative_file_name));

        // update callback info
        updateCallbackInfo(file_info, archive_name, extract_dir);

        if (file_info.file_header.flags & F_ISDIR) {
            create_directories(absolute_file_name);
        } else {
            create_directories(path(absolute_file_name).parent_path());
            ofstream ofile(absolute_file_name, ios::binary);
            if (!decompressFile(archive_file, ofile, file_info)) return false;
        }
    }
    file_list->updateFiles(extract_dir);
    if (!archive_callback->callback(CLT_ARCHIVE_FINISH)) return false;
    return true;
}

// detect if input is a file, folder or archive 
ArchiveDetectResult Archive::detectInput(wstring &input_name) {
    memset(&archive_header, 0, sizeof(ArchiveHeader));
    if (is_directory(input_name)) {
        // input is directory to compress
        return AD_CREATE;

    } else if (is_regular_file(input_name)) {
        // try to read header
        ifstream ifile(input_name, ios::binary);
        bool is_arch = false;
        if (ifile.is_open()) {
            is_arch = readArchiveHeader(ifile);
            ifile.close();
        }
       
        if (is_arch) {
            return AD_EXTRACT;
        } else {
            // input is file to compress
            return AD_CREATE;
        }
    }  else {
        return AD_UNKNOWN;
    }
}

}
