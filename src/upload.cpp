#include "upload.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <system_error>

namespace fs = std::filesystem;

namespace {

std::string toLower(std::string s) {
    for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

std::string trim(const std::string& s) {
    const auto b = s.find_first_not_of(" \t");
    if (b == std::string::npos) return "";
    const auto e = s.find_last_not_of(" \t");
    return s.substr(b, e - b + 1);
}

}  // namespace

bool isAllowedExtension(const std::string& filename) {
    static const std::array<const char*, 9> kExts = {
        ".epub", ".pdf", ".cbz", ".cbr", ".cb7", ".zip", ".rar", ".7z", ".cbt"};
    const auto dot = filename.rfind('.');
    if (dot == std::string::npos) return false;
    const std::string ext = toLower(filename.substr(dot));
    return std::any_of(kExts.begin(), kExts.end(), [&](const char* e) { return ext == e; });
}

std::optional<std::string> sanitizeFilename(const std::string& raw) {
    // 只保留最后一个 / 或 \ 之后的部分
    const auto slash = raw.find_last_of("/\\");
    const std::string base = (slash == std::string::npos) ? raw : raw.substr(slash + 1);

    std::string out;
    for (unsigned char c : base) {
        if (c < 0x20 || c == 0x7F) continue;                    // 控制字符直接丢掉
        if (std::string(":*?\"<>|").find(static_cast<char>(c)) != std::string::npos) {
            out += '_';
        } else {
            out += static_cast<char>(c);                         // 中文等 UTF-8 字节原样保留
        }
    }
    out = trim(out);
    if (out.empty() || out.front() == '.' || out.size() > 255) return std::nullopt;
    return out;
}

std::string bookFolderName(const std::string& filename) {
    const auto dot = filename.rfind('.');
    std::string stem = trim(dot == std::string::npos ? filename : filename.substr(0, dot));
    return stem.empty() ? filename : stem;
}

SaveResult saveBook(const std::string& libraryDir, const std::string& filename,
                    const std::string& content, std::string& savedPath, std::string& error) {
    std::error_code ec;
    const fs::path dir = fs::path(libraryDir) / bookFolderName(filename);
    const fs::path target = dir / filename;
    savedPath = target.string();

    if (fs::exists(target, ec)) return SaveResult::AlreadyExists;

    fs::create_directories(dir, ec);
    if (ec) {
        error = "无法创建目录 " + dir.string() + "：" + ec.message();
        return SaveResult::IoError;
    }

    const fs::path tmp = target.string() + ".part";
    {
        std::ofstream ofs(tmp, std::ios::binary | std::ios::trunc);
        if (!ofs) {
            error = "无法写入 " + tmp.string() + "（检查目录权限）";
            return SaveResult::IoError;
        }
        ofs.write(content.data(), static_cast<std::streamsize>(content.size()));
        if (!ofs) {
            error = "写入失败（磁盘满了？）";
            fs::remove(tmp, ec);
            return SaveResult::IoError;
        }
    }   // 离开作用域时 ofstream 析构，文件被关闭（RAII）

    fs::rename(tmp, target, ec);
    if (ec) {
        error = "重命名失败：" + ec.message();
        fs::remove(tmp, ec);
        return SaveResult::IoError;
    }
    return SaveResult::Ok;
}
