////////////////////////////////////////////
// Small Compression Library              //
// author: Mariusz Ziach                  //
// www   : http://ziach.pl/               //
// date  : 2020                           //
////////////////////////////////////////////

#ifdef _WIN32

// Windows
#include <windows.h>

#endif

// C
#include <cstdio>

// C++
#include <string>
#include <vector>
#include <filesystem>

// Archive
#include "../SCL/Archive.h"

using namespace std;
using namespace std::filesystem;

// Callback class
class CodecCallback : public SCL::ArchiveCallbackInterface {
private:
    int sec;
    SCL::QWord id;
public:
    CodecCallback() {
        id  = -1;
        sec = 0;
    }
    bool callback(SCL::CallbackType ct) {
        if (ct == SCL::CLT_COUNTING_FILES) {
            wprintf(L"\r  Counting files: %lli", info.number_of_files);
        } else if (ct == SCL::CLT_COUNTING_FILES_FINISH) {
            wprintf(L"\r");
        } else if (ct == SCL::CLT_ARCHIVE_BEGIN) {
            // Output header
            wprintf(L"\r  \u001b[4m\u001b[32m%7s"
                     "\u001b[4m\u001b[31m%36s"
                     "\u001b[0m\u001b[4m %15s %14s"
                     "\u001b[0m\u001b[4m%12s"
                     "\u001b[0m\u001b[4m%11s"
                     "\u001b[4m\u001b[33m%12s"
                     "\u001b[0m\n\n",
                     L"%  ",
                     L"File",
                     L"In size",
                     L"Out size",
                     L"Date",
                     L"Hash",
                     L"Time");

        } else if (ct == SCL::CLT_PROGRESS || ct == SCL::CLT_FILE_FINISH) {

            // Output every second or every file
            if (info.file_clock != sec || info.file_ID != id || ct == SCL::CLT_FILE_FINISH) {
                sec = info.file_clock;
                id  = info.file_ID;

                wprintf(L"\r  \u001b[32;1m%4i%%   "
                         "\u001b[31;1m%35.35ws"
                         "\u001b[30;1m%10i kB"
                         "\u001b[0m"
                         " -> "
                         "\u001b[30;1m%11i kB"
                         "\u001b[30;1m  %02i/%02i/%4i"
                         "\u001b[30;1m  #%08x"
                         "\u001b[33;1m%8i sec"
                         "\u001b[0m",
                    info.archive_precent,
                    info.file_name.c_str(),
                    (int)(info.file_read_bytes    / 1024),
                    (int)(info.file_written_bytes / 1024),
                    info.file_modification_time.getDay(),
                        info.file_modification_time.getMonth(),
                        info.file_modification_time.getYear(),
                    info.file_hash,
                    info.archive_clock);
            }
            if (ct == SCL::CLT_FILE_FINISH) {
                wprintf(L"\n");
            }
        } else if (ct == SCL::CLT_ARCHIVE_FINISH) {
            wprintf(L"\n");

            int comp_size    = int(info.callback_action == SCL::CLA_COMPRESS ? info.archive_written_bytes : info.archive_read_bytes) / 1024 / 1024;
            int decomp_size  = int(info.callback_action == SCL::CLA_COMPRESS ? info.archive_read_bytes : info.archive_written_bytes) / 1024 / 1024;

            // Summary
            wprintf(L"    \u001b[30;1m%67s%40ws\n"
                     "    \u001b[30;1m%67s%40ws\n"
                     "    \u001b[30;1m%67s%40ws\n"
                     "    \u001b[30;1m%67s%37i MB\n"
                     "    \u001b[30;1m%67s%37i MB\n"
                     "    \u001b[30;1m%67s%39i%%\n"
                     "    \u001b[30;1m%67s%36i sec\n"
                     "    \u001b[0m",
                     L"Archive path:", info.archive_path.c_str(),
                     L"Archive file:", info.archive_file.c_str(),
                     L"Output path:", info.output_path.c_str(),
                     L"Compressed size:", comp_size,
                     L"Decompressed size:", decomp_size,
                     L"Ratio:", info.archive_ratio,
                     L"Time:", info.archive_clock);

        }

        return true;
    }
};

// Header
void printHeader() {
    wprintf(L"\u001b[31;1m\n  File Archiver based on SCL (Small Compression Library)\n"
             "  (c) 2020 Mariusz Ziach\n"
             "  http://ziach.pl\u001b[0m\n");
}

// Usage info
void printUsageInfo() {
    wprintf(L"\u001b[31;1m\n  SCL.exe <input> \n\n"
             "\u001b[30;1m"
             "  <input> - Program will automatically recognize whether the dropped file is an archive for decompression or\n"
             "            file/folder for compression. It will also prevent overwriting files by creating unique names for\n"
             "            outputed files and folders if needed.\n"
             "\u001b[0m");
}


// Wait for input
void waitForInput(const wchar_t * msg) {
    wprintf(L"\n  Press enter to %s...\n\t", msg);
    cin.ignore();
}

// suffix gen
wstring suffixGen(int* i) {
    wstringstream ss;
    ss << (*i)++;
    return ss.str();
}

// Create unique name from given input name
void createUniqueName(wstring& name, wstring* out, wchar_t const* next) {
    int suf(0);
    path pn(name), base(name);
    base.replace_extension(L"");
    pn.replace_extension(next);
    while (exists(pn)) {
        pn = wstring(wstring(base) + suffixGen(&suf));
        pn.replace_extension(next);
    }
    *out = pn;
}

// Main 
int wmain(int argc, wchar_t* argv[]) {

    // setup console for Windows
    #ifdef _WIN32
        // Enable ANSI escape code
        HANDLE hout = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode = 0;
        GetConsoleMode(hout, &mode);
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hout, mode);

        // Title
        SetConsoleTitleW(L"SCL");
    #endif

    printHeader();
    
    if (argc > 1) {
        // Variables
        SCL::Archive SCL;

        wstring first_arg = wstring(argv[1]);
        SCL::ArchiveDetectResult det = SCL.detectInput(first_arg);
        CodecCallback callback;
        SCL.setCallback(&callback);

        // Input, output file/folder names
        wstring archive;
        wstring decom_folder;
        vector<wstring> files;

        // Copy command parameters
        for (int i = 1; i < argc; i++) {
            files.push_back(wstring(argv[i]));
        }

        switch (det) {
            case SCL::AD_CREATE:
                // Create archive file name
                createUniqueName(first_arg, &archive, L".scla");
                waitForInput(L"create archive");
                // Create archive
                SCL.archiveCreate(files, archive);

                break;

            case SCL::AD_EXTRACT:
                archive = argv[1];
                // Create output folder
                createUniqueName(first_arg, &decom_folder, L"");
                // Wait
                waitForInput(L"extract archive");
                SCL.archiveExtract(archive, decom_folder);

                break;
            case SCL::AD_UNKNOWN:
            default:
                printUsageInfo();
                break;
        }
    } else {
        printUsageInfo();
    }
    
    waitForInput(L"quit");
    return 0;
}
