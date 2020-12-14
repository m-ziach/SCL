////////////////////////////////////////////
// Small Compression Library              //
// author: Mariusz Ziach                  //
// www   : http://ziach.pl/               //
// date  : 2020                           //
////////////////////////////////////////////

#include "Utils.h"

namespace SCL {

// reading/write integers to byte mem
DWord write32To8Buf(Byte* buf, DWord i) {
    buf[0] = (Byte)((i) & 0xFF);
    buf[1] = (Byte)((i >> 8) & 0xFF);
    buf[2] = (Byte)((i >> 16) & 0xFF);
    buf[3] = (Byte)((i >> 24) & 0xFF);
    return sizeof(DWord);
}

DWord write16To8Buf(Byte* buf, Word  i) {
    buf[0] = (Byte)((i) & 0xFF);
    buf[1] = (Byte)((i >> 8) & 0xFF);
    return sizeof(Word);
}

DWord read32From8Buf(Byte* buf) {
    DWord i = (DWord)(buf[0]);
    i |= (DWord)(buf[1]) << 8;
    i |= (DWord)(buf[2]) << 16;
    i |= (DWord)(buf[3]) << 24;
    return i;
}

Word read16From8Buf(Byte* buf) {
    Word i = (Word)(buf[0]);
    i |= (Word)(buf[1]) << 8;
    return i;
}

// file attributes
DWord getFileAttributes(const wchar_t* f_name) {
    return GetFileAttributesW(f_name);
}
bool setFileAttributes(const wchar_t* f_name, DWord attr) {
    return (bool)SetFileAttributesW(f_name, attr);
}

// file time
void FileTime::set(QWord time) {
    FILETIME file_time;
    file_time.dwHighDateTime = (time >> 32);
    file_time.dwLowDateTime  = time & 0xFFFFFFFF;
    FileTimeToSystemTime(&file_time, &this->sys_time);
}

Word FileTime::getYear() {
    return this->sys_time.wYear;
}

Word FileTime::getMonth() {
    return this->sys_time.wMonth;
}

Word FileTime::getDayOfWeek() {
    return this->sys_time.wDayOfWeek;
}

Word FileTime::getDay() {
    return this->sys_time.wDay;
}

Word FileTime::getHour() {
    return this->sys_time.wHour;
}

Word FileTime::getMinute() {
    return this->sys_time.wMinute;
}

Word FileTime::getSecond() {
    return this->sys_time.wSecond;
}

Word FileTime::getMillisecond() {
    return this->sys_time.wMilliseconds;
}

void getFileTime(const wchar_t* f_name, QWord* fcr, QWord* fla, QWord* lwr, bool dir) {
    FILETIME ft1, ft2, ft3;
    HANDLE hf = CreateFileW(f_name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
        (dir ? (FILE_ATTRIBUTE_DIRECTORY | FILE_FLAG_BACKUP_SEMANTICS) : FILE_ATTRIBUTE_NORMAL), NULL);
    GetFileTime(hf, &ft1, &ft2, &ft3);
    CloseHandle(hf);
    *fcr = QWord(ft1.dwHighDateTime) << 32 | ft1.dwLowDateTime;
    *fla = QWord(ft2.dwHighDateTime) << 32 | ft2.dwLowDateTime;
    *lwr = QWord(ft3.dwHighDateTime) << 32 | ft3.dwLowDateTime;
}

void setFileTime(const wchar_t* f_name, QWord fcr, QWord fla, QWord lwr, bool dir) {
    FILETIME ft1, ft2, ft3;
    ft1.dwHighDateTime = (fcr >> 32); ft1.dwLowDateTime = fcr & 0xFFFFFFFF;
    ft2.dwHighDateTime = (fla >> 32); ft2.dwLowDateTime = fla & 0xFFFFFFFF;
    ft3.dwHighDateTime = (lwr >> 32); ft3.dwLowDateTime = lwr & 0xFFFFFFFF;
    HANDLE hf = CreateFileW(f_name, GENERIC_ALL, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
        (dir ? (FILE_FLAG_BACKUP_SEMANTICS | FILE_ATTRIBUTE_DIRECTORY) : FILE_ATTRIBUTE_NORMAL), NULL);
    SetFileTime(hf, &ft1, &ft2, &ft3);
    CloseHandle(hf);
}

void stringToWstring(string& in, wstring* out) {
    if (in.length() == 0) return;
    wchar_t* wstr = new wchar_t[MAX_PATH];
    size_t converted(0);
    mbstowcs_s(&converted, wstr, MAX_PATH, in.c_str(), in.length());
    *out = wstr;

    delete[] wstr;
}

QWord fileSize(ifstream& ifile) {
    streampos current_pos = ifile.tellg();
    ifile.seekg(0, ios::end);
    QWord in_file_size = ifile.tellg() - current_pos;
    ifile.seekg(current_pos);
    return in_file_size;
}

}