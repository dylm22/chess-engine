#include "utils.h"
#include "direct.h"


std::string command_line::get_binary_directory(std::string argv0) { //i aint write this
    std::string pathSeparator;

    pathSeparator = "\\";
    // Under windows argv[0] may not have the extension. Also _get_pgmptr() had
    // issues in some Windows 10 versions, so check returned values carefully.
    char* pgmptr = nullptr;
    if (!_get_pgmptr(&pgmptr) && pgmptr != nullptr && *pgmptr)
        argv0 = pgmptr;

    // Extract the working directory
    auto workingDirectory = command_line::get_working_directory();

    // Extract the binary directory path from argv0
    auto   binaryDirectory = argv0;
    size_t pos = binaryDirectory.find_last_of("\\/");
    if (pos == std::string::npos)
        binaryDirectory = "." + pathSeparator;
    else
        binaryDirectory.resize(pos + 1);

    // Pattern replacement: "./" at the start of path is replaced by the working directory
    if (binaryDirectory.find("." + pathSeparator) == 0)
        binaryDirectory.replace(0, 1, workingDirectory);

    return binaryDirectory;
}

std::string command_line::get_working_directory() {
    std::string workingDirectory = "";
    char        buff[40000];
    char* cwd = _getcwd(buff, 40000);
    if (cwd)
        workingDirectory = cwd;

    return workingDirectory;
}
void prefetch(const void* addr) {
    _mm_prefetch((char const*)addr, _MM_HINT_T0); //yep
}
