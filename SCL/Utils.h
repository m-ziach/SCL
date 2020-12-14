////////////////////////////////////////////
// Small Compression Library              //
// author: Mariusz Ziach                  //
// www   : http://ziach.pl/               //
// date  : 2020                           //
////////////////////////////////////////////

#ifndef SCL_UTILS_H
#define SCL_UTILS_H

// c
#include <cassert>
#include <cstdlib>

// stl
#include <fstream>
#include <cstdlib>
#include <string>

// c
//#include <ctime>

// Archive
#include "Types.h"

#define COUNTPRECENT(x,y) (DWord)(float(x) / float(y) * 100.0);
#define COUNTTIME(start)  (DWord)((double)(clock() - start) / CLOCKS_PER_SEC);

using namespace std;

namespace SCL {

// reading/writing integers from/to byte stream
DWord write32To8Buf (Byte *buf, DWord i);
DWord write16To8Buf (Byte *buf, Word  i);
DWord read32From8Buf(Byte *buf);
Word  read16From8Buf(Byte *buf);

// file attributes
DWord getFileAttributes (const wchar_t *f_name);
bool  setFileAttributes (const wchar_t *f_name, DWord attr);

// file time
class FileTime {
private:
    SYSTEMTIME sys_time;

public:
    void set(QWord time);
    Word getYear();
    Word getMonth();
    Word getDay();
    Word getDayOfWeek();
    Word getHour();
    Word getMinute();
    Word getSecond();
    Word getMillisecond();
};

void getFileTime(const wchar_t *f_name, QWord *fcr, QWord *fla, QWord *lwr, bool dir = false);
void setFileTime(const wchar_t *f_name, QWord  fcr, QWord  fla, QWord  lwr, bool dir = false);

// file size
QWord fileSize(ifstream& ifile);

} // namespace

#endif // SCL_UTILS_H
