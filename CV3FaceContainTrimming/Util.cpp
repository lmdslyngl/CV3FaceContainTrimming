
#include "Util.h"
#include <Windows.h>

// 自分のディレクトリを取得する
std::string GetMyDir() {
    char buf[256];
    GetModuleFileName(nullptr, buf, 256);
    return buf;
}
