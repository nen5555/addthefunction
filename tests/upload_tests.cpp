#include "upload.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

namespace fs = std::filesystem;

static int failures = 0;

static void check(bool cond, const std::string& name) {
    if (!cond) {
        std::cerr << "FAIL: " << name << "\n";
        ++failures;
    }
}

int main() {
    // ---- sanitizeFilename ----
    check(sanitizeFilename("book.epub") == std::string("book.epub"), "plain name");
    check(sanitizeFilename("三体.epub") == std::string("三体.epub"), "chinese name kept");
    check(sanitizeFilename("../../etc/passwd") == std::string("passwd"), "unix path traversal stripped");
    check(sanitizeFilename("C:\\Users\\a\\b.pdf") == std::string("b.pdf"), "windows path stripped");
    check(sanitizeFilename("a:b?.pdf") == std::string("a_b_.pdf"), "illegal chars replaced");
    check(sanitizeFilename("  spaced.epub  ") == std::string("spaced.epub"), "trimmed");
    check(sanitizeFilename("bad\nname.epub") == std::string("badname.epub"), "control chars dropped");
    check(!sanitizeFilename(""), "empty rejected");
    check(!sanitizeFilename(".."), "dotdot rejected");
    check(!sanitizeFilename("dir/"), "trailing slash rejected");
    check(!sanitizeFilename(".hidden.epub"), "dotfile rejected");
    check(!sanitizeFilename(std::string(300, 'a') + ".epub"), "too long rejected");

    // ---- isAllowedExtension ----
    check(isAllowedExtension("a.epub"), "epub ok");
    check(isAllowedExtension("a.PDF"), "uppercase ok");
    check(isAllowedExtension("a.cbz"), "cbz ok");
    check(!isAllowedExtension("a.exe"), "exe rejected");
    check(!isAllowedExtension("noext"), "no extension rejected");
    check(!isAllowedExtension("a.epub.part"), "part rejected");

    // ---- bookFolderName ----
    check(bookFolderName("三体.epub") == "三体", "folder from stem");
    check(bookFolderName("a.b.pdf") == "a.b", "only last extension removed");

    // ---- saveBook ----
    const fs::path dir = fs::temp_directory_path() / ("upload_tests_" + std::to_string(getpid()));
    fs::remove_all(dir);
    std::string path, error;
    check(saveBook(dir.string(), "书.epub", "hello", path, error) == SaveResult::Ok, "save ok");
    check(fs::exists(dir / "书" / "书.epub"), "saved into own folder");
    check(!fs::exists(dir / "书" / "书.epub.part"), "no part file left");
    {
        std::ifstream in(dir / "书" / "书.epub", std::ios::binary);
        std::string content((std::istreambuf_iterator<char>(in)), {});
        check(content == "hello", "content written");
    }
    check(saveBook(dir.string(), "书.epub", "again", path, error) == SaveResult::AlreadyExists,
          "existing file not overwritten");
    fs::remove_all(dir);

    if (failures == 0) std::cout << "all upload tests passed\n";
    return failures == 0 ? 0 : 1;
}
