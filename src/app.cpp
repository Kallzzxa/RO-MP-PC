// ============================================================
// RO-MP PC — ConfigPath: folder config per-OS.
// ============================================================
#include "app.h"

#include <string>

#ifdef _WIN32
  #include <windows.h>
  #include <shlobj.h>
#else
  #include <sys/stat.h>
  #include <unistd.h>
#endif

namespace romppc {

const char* ConfigPath(const char* name) {
    static std::string base;
    static std::string full;
    if (base.empty()) {
#ifdef _WIN32
        char p[MAX_PATH] = "";
        // %APPDATA%/RO-MP-PC
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, p))) {
            base = std::string(p) + "\\RO-MP-PC";
        } else {
            base = ".";
        }
        CreateDirectoryA(base.c_str(), NULL);
#else
        const char* h = getenv("HOME");
        base = std::string(h ? h : ".") + "/.romp-pc";
        mkdir(base.c_str(), 0755);
#endif
    }
    full = base +
#ifdef _WIN32
        "\\" +
#else
        "/" +
#endif
        name;
    return full.c_str();
}

}  // namespace romppc
