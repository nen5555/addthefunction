#pragma once
#include <optional>
#include <string>

// 上传书籍：文件名检查（纯逻辑，可单独测试）+ 保存到书库目录

// Kavita 能识别的格式（小写，带点）
bool isAllowedExtension(const std::string& filename);

// 清理浏览器传来的文件名：
//   - 去掉路径部分（防止 "../../etc/passwd" 这种路径穿越 path traversal）
//   - 去掉控制字符，把 Windows 不允许的字符 \ / : * ? " < > | 换成 _
//   - 去掉首尾空格
// 结果为空、"."、".."、以点开头、或超过 255 字节时返回 nullopt
std::optional<std::string> sanitizeFilename(const std::string& raw);

// 书的文件夹名：去掉扩展名的文件名（Kavita 的 Books 书库建议一本书一个文件夹）
std::string bookFolderName(const std::string& filename);

enum class SaveResult { Ok, AlreadyExists, IoError };

// 把内容写到 libraryDir/<文件夹>/<文件名>
// 先写到 .part 临时文件，写完再重命名（rename），避免 Kavita 扫描到写了一半的文件
// 同名文件已存在时不覆盖，返回 AlreadyExists；savedPath 返回最终路径
SaveResult saveBook(const std::string& libraryDir, const std::string& filename,
                    const std::string& content, std::string& savedPath, std::string& error);
