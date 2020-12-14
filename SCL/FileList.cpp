////////////////////////////////////////////
// Small Compression Library              //
// author: Mariusz Ziach                  //
// www   : http://ziach.pl/               //
// date  : 2020                           //
////////////////////////////////////////////

#include "FileList.h"

namespace SCL {

FileList::FileList() {
    number_of_files   = 0;
    number_of_folders = 0;
    size_of_all_files = 0;
    setlocale(LC_ALL, "");
}

FileList::~FileList() {}

void FileList::insertFile(wstring &absolute_file_name, wstring &relative_file_name, QWord &file_ID) {

    if (!is_regular_file(absolute_file_name) && !is_directory(absolute_file_name)) return;

    FileInfo fi;
    fi.absolute_file_name = absolute_file_name;
    fi.relative_file_name = relative_file_name;

    memset(&fi.file_header, 0, sizeof(FileHeader));

    fi.file_header.flags = is_directory(absolute_file_name) ? F_ISDIR : 0;
    fi.file_header.file_size = file_size(fi.absolute_file_name);
    fi.file_header.file_name_length = (Word)fi.relative_file_name.length();
    fi.file_header.file_ID = file_ID++;

    getFileTime(fi.absolute_file_name.c_str(),
        &fi.file_header.file_creation_time,
        &fi.file_header.file_access_time,
        &fi.file_header.file_modification_time);

    fi.file_header.file_attributes = getFileAttributes(fi.absolute_file_name.c_str());

    if (is_regular_file(absolute_file_name)) {
        size_of_all_files += fi.file_header.file_size;
        number_of_files++;
    }

    if (is_directory(absolute_file_name)) number_of_folders++;

    file_list.push_back(fi);
}

void FileList::createFileListThread() {
    QWord file_ID(0);
    for (wstring& file_name : file_names) {
        if (is_directory(file_name)) {
            path item = file_name;
            path dir_name = item.filename();
            for (const auto& entry : recursive_directory_iterator(item)) {
                wstring absolute_file_name = absolute(entry.path());
                wstring relative_file_name = dir_name / entry.path().lexically_relative(item);
                insertFile(absolute_file_name, relative_file_name, file_ID);
            }
        } else if (is_regular_file(file_name)) {
            wstring absolute_file_name = absolute(file_name);
            wstring relative_file_name = path(absolute_file_name).filename();
            insertFile(absolute_file_name, relative_file_name, file_ID);
        }
    }
}

QWord FileList::writeFileList(ofstream &ofs) {
    streampos beg = ofs.tellp();
    ofs.write((char*)&number_of_files,   sizeof(QWord));
    ofs.write((char*)&number_of_folders, sizeof(QWord));
    ofs.write((char*)&size_of_all_files, sizeof(QWord));

    for (FileInfo& fi : file_list) {
        ofs.write((char*)&fi.file_header, sizeof(FileHeader));
        for (wchar_t wc : fi.relative_file_name)
            ofs.write((char*)&wc, sizeof(wchar_t));
    }

    streampos end = ofs.tellp();
    return end - beg;
}

QWord FileList::readFileList(ifstream &ifs) {
    streampos beg = ifs.tellg();
    file_list.clear();

    ifs.read((char*)&number_of_files,   sizeof(QWord));
    ifs.read((char*)&number_of_folders, sizeof(QWord));
    ifs.read((char*)&size_of_all_files, sizeof(QWord));

    for (int i = 0; (i < number_of_files + number_of_folders) && !ifs.eof() ; i++) {
        FileInfo fi;
        ifs.read((char*)&fi.file_header, sizeof(FileHeader));
        fi.relative_file_name.clear();
        for (int j = 0; j < fi.file_header.file_name_length; j++) {
            wchar_t wc;
            ifs.read((char*)&wc, sizeof(wchar_t));
            fi.relative_file_name += wc;
        }
        
        file_list.push_back(fi);

    }

    if (ifs.eof()) ifs.clear();
    streampos end = ifs.tellg();
    return end - beg;
}

void FileList::createFileList(vector<wstring>&file_names) {
    this->file_names = file_names;
    thread_future = async(std::launch::async, &FileList::createFileListThread, this);
}

vector<FileInfo>*FileList::getFileList() {
    return &this->file_list;
}

QWord FileList::getNumberOfFiles() {
    return this->number_of_files;
}

QWord FileList::getNumberOfFolders() {
    return this->number_of_folders;
}

QWord FileList::getSizeOfAllFiles() {
    return this->size_of_all_files;
}

bool FileList::isCompleted() {
    return thread_future.wait_for(std::chrono::seconds(0)) == future_status::ready;
}

void FileList::waitUntilCompleted() {
    while (!isCompleted()) { }
}

void FileList::updateFiles(wstring& dir) {
    for (FileInfo& fi : file_list) {
        fi.absolute_file_name = wstring(path(dir) / path(fi.relative_file_name));
        
        setFileTime(fi.absolute_file_name.c_str(),
            fi.file_header.file_creation_time,
            fi.file_header.file_access_time,
            fi.file_header.file_modification_time);

        setFileAttributes(fi.absolute_file_name.c_str(), fi.file_header.file_attributes);
    }
}

}
