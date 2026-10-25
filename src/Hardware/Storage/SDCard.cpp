#include "Hardware/Storage/SDCard.h"
#include "Hardware/Core/ISerializable.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <set>
#include <string_view>

#ifndef PROJECT_VERSION
#define PROJECT_VERSION "unknown"
#endif

namespace Hardware {

namespace {

// Paths are stored as UTF-8; on Windows std::string -> path would use the ANSI code page.
std::filesystem::path PathFromUtf8(std::string_view str) {
    return std::filesystem::path(std::u8string(str.begin(), str.end()));
}

}  // namespace

SDCard::SDCard() { Reset(); }

SDCard::~SDCard() { Unmount(); }

void SDCard::Reset() {
    state = State::IDLE;
    cmd_bytes_received = 0;
    response_index = 0;
    response_buffer.clear();
    is_acmd = false;
    is_initialized = false;
    is_sdhc = true;  // Assuming SDHC for 512-byte sectors and blocks addressing
    cs_active = false;
    crc_enabled = false;
    is_read_multiblock = false;
    is_write_multiblock = false;
    sd_status_byte = 0x00;
    last_miso = 0xFFU;
    last_mosi = 0xFFU;

    if (is_folder_mount && !currentPath.empty()) {
        SyncFromHost();
    }
}

bool SDCard::Mount(const std::string& imagePath) {
    const std::filesystem::path fsPath = PathFromUtf8(imagePath);
    if (std::filesystem::is_directory(fsPath)) {
        return MountFolder(imagePath);
    }

    if (mounted) {
        Unmount();
    }

    imageFile.open(fsPath, std::ios::in | std::ios::out | std::ios::binary);
    if (imageFile.is_open()) {
        is_read_only = false;
    } else {
        // Try opening read-only if read-write failed
        imageFile.clear();
        imageFile.open(fsPath, std::ios::in | std::ios::binary);
        if (imageFile.is_open()) {
            is_read_only = true;
        } else {
            // Try creating it if it doesn't exist
            imageFile.clear();
            imageFile.open(fsPath, std::ios::out | std::ios::binary);
            if (imageFile.is_open()) {
                imageFile.close();
                imageFile.open(fsPath, std::ios::in | std::ios::out | std::ios::binary);
                is_read_only = false;
            }
        }
    }

    if (imageFile.is_open()) {
        currentPath = imagePath;
        mounted = true;
        
        imageFile.seekg(0, std::ios::end);
        total_blocks = static_cast<uint32_t>(imageFile.tellg() / 512);
        imageFile.seekg(0, std::ios::beg);
        
        is_modified = false;
        Reset();
        return true;
    }
    return false;
}

void SDCard::Unmount() {
    sfnToHostMap.clear();
    if (is_folder_mount) {
        if (is_modified) {
            SyncToHost();
        }
        virtual_disk.clear();
        virtual_disk.shrink_to_fit();
        is_folder_mount = false;
        allow_host_deletion = false;
    }
    if (imageFile.is_open()) {
        imageFile.close();
    }
    mounted = false;
    is_modified = false;
}

namespace {

bool IsPathContained(const std::filesystem::path& target, const std::filesystem::path& base_dir) {
    try {
        std::error_code ec;
        std::filesystem::path norm_base = std::filesystem::canonical(base_dir, ec);
        if (ec) return false;
        std::filesystem::path norm_target = std::filesystem::weakly_canonical(target, ec);
        if (ec) return false;

        auto it_base = norm_base.begin();
        auto it_target = norm_target.begin();
#if defined(_WIN32)
        while (it_base != norm_base.end()) {
            if (it_target == norm_target.end()) {
                return false;
            }
            std::string s_base = it_base->string();
            std::string s_target = it_target->string();
            if (s_base.size() != s_target.size()) {
                return false;
            }
            for (size_t i = 0; i < s_base.size(); ++i) {
                if (std::tolower(static_cast<unsigned char>(s_base[i])) !=
                    std::tolower(static_cast<unsigned char>(s_target[i]))) {
                    return false;
                }
            }
            ++it_base;
            ++it_target;
        }
#else
        while (it_base != norm_base.end()) {
            if (it_target == norm_target.end() || *it_base != *it_target) {
                return false;
            }
            ++it_base;
            ++it_target;
        }
#endif
        return true;
    } catch (...) {
        return false;
    }
}

void GetDosDateTime(const std::filesystem::file_time_type& ft, uint16_t& dosDate, uint16_t& dosTime) {
    dosDate = static_cast<uint16_t>((((2026 - 1980) & 0x7F) << 9) | ((1 & 0x0F) << 5) | (1 & 0x1F));
    dosTime = 0;
    try {
        auto sctp = std::chrono::file_clock::to_sys(ft);
        std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);
        std::tm tm_buf{};
#if defined(_WIN32)
        localtime_s(&tm_buf, &cftime);
#else
        localtime_r(&cftime, &tm_buf);
#endif
        int year = tm_buf.tm_year + 1900;
        if (year < 1980) year = 1980;
        if (year > 2107) year = 2107;
        int month = tm_buf.tm_mon + 1;
        int day = tm_buf.tm_mday;
        int hour = tm_buf.tm_hour;
        int min = tm_buf.tm_min;
        int sec = tm_buf.tm_sec / 2;

        dosDate = static_cast<uint16_t>((((year - 1980) & 0x7F) << 9) | ((month & 0x0F) << 5) | (day & 0x1F));
        dosTime = static_cast<uint16_t>(((hour & 0x1F) << 11) | ((min & 0x3F) << 5) | (sec & 0x1F));
    } catch (...) {
    }
}

std::string GenerateSFN(const std::string& filename, std::set<std::string>& used_sfns) {
    std::string base;
    std::string ext;
    size_t last_dot = filename.find_last_of('.');
    if (last_dot != std::string::npos && last_dot > 0) {
        base = filename.substr(0, last_dot);
        ext = filename.substr(last_dot + 1);
    } else {
        base = filename;
    }

    auto cleanPart = [](const std::string& str) {
        std::string res;
        for (char c : str) {
            unsigned char uc = static_cast<unsigned char>(c);
            if (std::isalnum(uc) || c == '_' || c == '-' || c == '$' || c == '~') {
                res += static_cast<char>(std::toupper(uc));
            }
        }
        return res;
    };

    std::string clean_base = cleanPart(base);
    std::string clean_ext = cleanPart(ext);

    if (clean_base.empty()) {
        clean_base = "FILE";
    }
    if (clean_ext.size() > 3) {
        clean_ext = clean_ext.substr(0, 3);
    }

    bool fits = (clean_base.size() <= 8 && clean_ext.size() <= 3 && base.find('.') == std::string::npos);
    if (fits) {
        std::string candidate_base = clean_base;
        candidate_base.resize(8, ' ');
        std::string candidate_ext = clean_ext;
        candidate_ext.resize(3, ' ');
        std::string sfn = candidate_base + candidate_ext;
        if (used_sfns.find(sfn) == used_sfns.end()) {
            used_sfns.insert(sfn);
            return sfn;
        }
    }

    int counter = 1;
    while (counter < 1000) {
        std::string tag = "~" + std::to_string(counter);
        size_t max_base_len = 8 - tag.size();
        std::string candidate_base = clean_base.substr(0, std::min(clean_base.size(), max_base_len)) + tag;
        candidate_base.resize(8, ' ');
        std::string candidate_ext = clean_ext;
        candidate_ext.resize(3, ' ');
        std::string sfn = candidate_base + candidate_ext;
        if (used_sfns.find(sfn) == used_sfns.end()) {
            used_sfns.insert(sfn);
            return sfn;
        }
        counter++;
    }

    std::string fallback = clean_base.substr(0, 6) + "~9";
    fallback.resize(8, ' ');
    clean_ext.resize(3, ' ');
    std::string sfn = fallback + clean_ext;
    used_sfns.insert(sfn);
    return sfn;
}

}  // namespace

static bool IsProtectedHostPath(const std::filesystem::path& canPath) {
    if (canPath.empty() || canPath == canPath.root_path()) {
        return true;
    }

    std::vector<std::filesystem::path> protectedDirs;
#if defined(_WIN32)
    for (const char* env : {"SystemRoot", "WINDIR", "ProgramFiles", "ProgramFiles(x86)", "ProgramData", "USERPROFILE"}) {
        if (const char* val = std::getenv(env)) {
            protectedDirs.push_back(PathFromUtf8(val));
        }
    }
#else
    for (const char* dir : {"/home", "/root", "/usr", "/bin", "/sbin", "/lib", "/etc", "/var", "/tmp", "/sys", "/proc",
                            "/dev", "/boot", "/opt", "/Users", "/System", "/Library", "/Applications"}) {
        protectedDirs.emplace_back(dir);
    }
    if (const char* home = std::getenv("HOME")) {
        protectedDirs.push_back(PathFromUtf8(home));
    }
#endif

    for (const auto& dir : protectedDirs) {
        std::error_code ec;
        std::filesystem::path canDir = std::filesystem::canonical(dir, ec);
        if (!ec && std::filesystem::equivalent(canPath, canDir, ec) && !ec) {
            return true;
        }
    }
    return false;
}

bool SDCard::MountFolder(const std::string& folderPath, bool allowDeletion) {
    const std::filesystem::path hostFolder = PathFromUtf8(folderPath);
    if (!std::filesystem::exists(hostFolder) || !std::filesystem::is_directory(hostFolder)) {
        return false;
    }

    std::error_code ec;
    std::filesystem::path canPath = std::filesystem::canonical(hostFolder, ec);
    if (ec || IsProtectedHostPath(canPath)) {
        return false;
    }

    if (std::filesystem::exists(hostFolder / ".65C02_SIM_mount")) {
        allow_host_deletion = true;
    } else {
        allow_host_deletion = allowDeletion;
    }

    if (mounted) {
        Unmount();
    }

    mounted = true;
    is_folder_mount = true;
    currentPath = folderPath;
    total_blocks = 65536;
    is_read_only = false;
    is_modified = false;

    if (!BuildVirtualFAT()) {
        Unmount();
        return false;
    }

    Reset();
    return true;
}

bool SDCard::BuildVirtualFAT() {
    sfnToHostMap.clear();
    if (!is_folder_mount || currentPath.empty()) {
        return false;
    }

    virtual_disk.assign(65536 * 512, 0);

    virtual_disk[0] = 0xEB;
    virtual_disk[1] = 0x3C;
    virtual_disk[2] = 0x90;
    std::memcpy(&virtual_disk[3], "6502SIM ", 8);
    virtual_disk[11] = 0x00;
    virtual_disk[12] = 0x02;
    virtual_disk[13] = 0x08;
    virtual_disk[14] = 0x04;
    virtual_disk[15] = 0x00;
    virtual_disk[16] = 0x02;
    virtual_disk[17] = 0x00;
    virtual_disk[18] = 0x02;
    virtual_disk[19] = 0x00;
    virtual_disk[20] = 0x00;
    virtual_disk[21] = 0xF8;
    virtual_disk[22] = 0x20;
    virtual_disk[23] = 0x00;
    virtual_disk[24] = 0x3F;
    virtual_disk[25] = 0x00;
    virtual_disk[26] = 0xFF;
    virtual_disk[27] = 0x00;
    virtual_disk[28] = 0x00;
    virtual_disk[29] = 0x00;
    virtual_disk[30] = 0x00;
    virtual_disk[31] = 0x00;
    virtual_disk[32] = 0x00;
    virtual_disk[33] = 0x00;
    virtual_disk[34] = 0x01;
    virtual_disk[35] = 0x00;
    virtual_disk[36] = 0x80;
    virtual_disk[37] = 0x00;
    virtual_disk[38] = 0x29;
    virtual_disk[39] = 0x78;
    virtual_disk[40] = 0x56;
    virtual_disk[41] = 0x34;
    virtual_disk[42] = 0x12;
    std::memcpy(&virtual_disk[43], "VIRTUAL FAT", 11);
    std::memcpy(&virtual_disk[54], "FAT16   ", 8);
    virtual_disk[510] = 0x55;
    virtual_disk[511] = 0xAA;

    auto setFatEntry = [this](uint16_t cluster, uint16_t val) {
        size_t f1 = 2048 + static_cast<size_t>(cluster) * 2;
        size_t f2 = 18432 + static_cast<size_t>(cluster) * 2;
        if (f2 + 1 < virtual_disk.size()) {
            virtual_disk[f1] = static_cast<uint8_t>(val & 0xFFU);
            virtual_disk[f1 + 1] = static_cast<uint8_t>((val >> 8) & 0xFFU);
            virtual_disk[f2] = static_cast<uint8_t>(val & 0xFFU);
            virtual_disk[f2 + 1] = static_cast<uint8_t>((val >> 8) & 0xFFU);
        }
    };

    setFatEntry(0, 0xFFF8);
    setFatEntry(1, 0xFFFF);

    uint16_t next_free_cluster = 2;

    auto writeDirEntryAt = [this](size_t offset, const std::string& sfn11, uint8_t attr,
                                  uint16_t date, uint16_t time, uint16_t first_cluster, uint32_t size) {
        if (offset + 32 <= virtual_disk.size()) {
            std::memcpy(&virtual_disk[offset], sfn11.data(), 11);
            virtual_disk[offset + 11] = attr;
            virtual_disk[offset + 12] = 0;
            virtual_disk[offset + 13] = 0;
            virtual_disk[offset + 14] = static_cast<uint8_t>(time & 0xFFU);
            virtual_disk[offset + 15] = static_cast<uint8_t>((time >> 8) & 0xFFU);
            virtual_disk[offset + 16] = static_cast<uint8_t>(date & 0xFFU);
            virtual_disk[offset + 17] = static_cast<uint8_t>((date >> 8) & 0xFFU);
            virtual_disk[offset + 18] = static_cast<uint8_t>(date & 0xFFU);
            virtual_disk[offset + 19] = static_cast<uint8_t>((date >> 8) & 0xFFU);
            virtual_disk[offset + 20] = 0;
            virtual_disk[offset + 21] = 0;
            virtual_disk[offset + 22] = static_cast<uint8_t>(time & 0xFFU);
            virtual_disk[offset + 23] = static_cast<uint8_t>((time >> 8) & 0xFFU);
            virtual_disk[offset + 24] = static_cast<uint8_t>(date & 0xFFU);
            virtual_disk[offset + 25] = static_cast<uint8_t>((date >> 8) & 0xFFU);
            virtual_disk[offset + 26] = static_cast<uint8_t>(first_cluster & 0xFFU);
            virtual_disk[offset + 27] = static_cast<uint8_t>((first_cluster >> 8) & 0xFFU);
            virtual_disk[offset + 28] = static_cast<uint8_t>(size & 0xFFU);
            virtual_disk[offset + 29] = static_cast<uint8_t>((size >> 8) & 0xFFU);
            virtual_disk[offset + 30] = static_cast<uint8_t>((size >> 16) & 0xFFU);
            virtual_disk[offset + 31] = static_cast<uint8_t>((size >> 24) & 0xFFU);
        }
    };

    struct SubDirInfo {
        std::filesystem::path host_path;
        uint16_t cluster = 0;
        uint16_t parent_cluster = 0;
    };

    auto buildDir = [&](auto& self, const std::filesystem::path& dir_path, uint16_t dir_cluster, uint16_t parent_cluster) -> void {
        std::vector<std::filesystem::directory_entry> items;
        try {
            for (const auto& entry : std::filesystem::directory_iterator(
                     dir_path, std::filesystem::directory_options::skip_permission_denied)) {
                std::string fname = entry.path().filename().string();
                if (fname.empty() || fname[0] == '.') {
                    continue;
                }
                if (std::filesystem::is_symlink(entry.symlink_status())) {
                    if (!IsPathContained(entry.path(), PathFromUtf8(currentPath))) {
                        continue;
                    }
                }
                items.push_back(entry);
            }
        } catch (...) {
            return;
        }

        std::sort(items.begin(), items.end(), [](const auto& a, const auto& b) {
            return a.path().filename().string() < b.path().filename().string();
        });

        std::set<std::string> used_sfns;
        size_t entry_index = 0;
        uint16_t current_dir_cluster = dir_cluster;

        auto writeNextEntry = [&](const std::string& sfn11, uint8_t attr, uint16_t date, uint16_t time, uint16_t first_clus, uint32_t size) {
            if (dir_cluster == 0) {
                if (entry_index < 512) {
                    size_t offset = 34816 + entry_index * 32;
                    writeDirEntryAt(offset, sfn11, attr, date, time, first_clus, size);
                    entry_index++;
                }
            } else {
                if (entry_index > 0 && (entry_index % 128) == 0) {
                    if (next_free_cluster > 8180) {
                        return;
                    }
                    uint16_t new_cluster = next_free_cluster++;
                    setFatEntry(current_dir_cluster, new_cluster);
                    setFatEntry(new_cluster, 0xFFFF);
                    current_dir_cluster = new_cluster;
                }
                size_t offset = 51200 + static_cast<size_t>(current_dir_cluster - 2) * 4096 + (entry_index % 128) * 32;
                writeDirEntryAt(offset, sfn11, attr, date, time, first_clus, size);
                entry_index++;
            }
        };

        if (dir_cluster != 0) {
            uint16_t date = 0;
            uint16_t time = 0;
            GetDosDateTime(std::filesystem::file_time_type::clock::now(), date, time);
            writeNextEntry(".          ", 0x10, date, time, dir_cluster, 0);
            writeNextEntry("..         ", 0x10, date, time, parent_cluster, 0);
        }

        std::vector<SubDirInfo> subdirs_to_process;

        for (const auto& item : items) {
            std::string fname = item.path().filename().string();
            std::string sfn = GenerateSFN(fname, used_sfns);
            sfnToHostMap[dir_path.string() + "/" + sfn] = item.path();

            std::error_code ec;
            uint16_t date = 0;
            uint16_t time = 0;
            auto lwt = item.last_write_time(ec);
            if (!ec) {
                GetDosDateTime(lwt, date, time);
            } else {
                GetDosDateTime(std::filesystem::file_time_type::clock::now(), date, time);
            }

            if (item.is_directory(ec)) {
                if (next_free_cluster > 8180) {
                    continue;
                }
                uint16_t sub_cluster = next_free_cluster++;
                setFatEntry(sub_cluster, 0xFFFF);
                writeNextEntry(sfn, 0x10, date, time, sub_cluster, 0);
                subdirs_to_process.push_back({item.path(), sub_cluster, dir_cluster});
            } else if (item.is_regular_file(ec)) {
                uintmax_t fsize = item.file_size(ec);
                if (ec || fsize > 30 * 1024 * 1024) {
                    continue;
                }
                uint16_t first_clus = 0;
                if (fsize > 0) {
                    uint32_t clusters_needed = static_cast<uint32_t>((fsize + 4095) / 4096);
                    if (next_free_cluster + clusters_needed > 8180) {
                        continue;
                    }
                    first_clus = next_free_cluster;
                    std::ifstream fin(item.path(), std::ios::binary);
                    uint32_t remaining = static_cast<uint32_t>(fsize);
                    for (uint32_t c = 0; c < clusters_needed; ++c) {
                        uint16_t cur_clus = next_free_cluster++;
                        uint16_t next_clus = (c + 1 < clusters_needed) ? next_free_cluster : 0xFFFF;
                        setFatEntry(cur_clus, next_clus);
                        size_t disk_offset = 51200 + static_cast<size_t>(cur_clus - 2) * 4096;
                        size_t to_read = std::min<size_t>(remaining, 4096);
                        fin.read(reinterpret_cast<char*>(&virtual_disk[disk_offset]), static_cast<std::streamsize>(to_read));
                        remaining -= static_cast<uint32_t>(to_read);
                    }
                }
                writeNextEntry(sfn, 0x20, date, time, first_clus, static_cast<uint32_t>(fsize));
            }
        }

        for (const auto& sub : subdirs_to_process) {
            self(self, sub.host_path, sub.cluster, sub.parent_cluster);
        }
    };

    std::error_code ec_root;
    std::filesystem::path root_host = std::filesystem::canonical(PathFromUtf8(currentPath), ec_root);
    if (ec_root) {
        root_host = PathFromUtf8(currentPath);
    }
    buildDir(buildDir, root_host, 0, 0);
    return true;
}

void SDCard::ExtractVirtualFAT() {
    if (!is_folder_mount || currentPath.empty() || virtual_disk.size() < 65536 * 512) {
        return;
    }

    std::error_code ec;
    std::filesystem::path root_host = std::filesystem::canonical(PathFromUtf8(currentPath), ec);
    if (ec) {
        return;
    }

    auto getFatEntry = [this](uint16_t cluster) -> uint16_t {
        size_t offset = 2048 + static_cast<size_t>(cluster) * 2;
        if (offset + 1 >= virtual_disk.size()) {
            return 0xFFFF;
        }
        return static_cast<uint16_t>(virtual_disk[offset] | (virtual_disk[offset + 1] << 8));
    };

    auto readClusterChain = [this, &getFatEntry](uint16_t start_cluster) {
        std::vector<size_t> entry_offsets;
        uint16_t clus = start_cluster;
        std::set<uint16_t> visited;
        while (clus >= 2 && clus < 0xFFF8) {
            if (!visited.insert(clus).second) {
                break;
            }
            size_t clus_offset = 51200 + static_cast<size_t>(clus - 2) * 4096;
            if (clus_offset + 4096 > virtual_disk.size()) {
                break;
            }
            for (size_t i = 0; i < 128; ++i) {
                entry_offsets.push_back(clus_offset + i * 32);
            }
            clus = getFatEntry(clus);
        }
        return entry_offsets;
    };

    std::set<std::filesystem::path> synced_paths;

    auto processEntries = [&](auto& self, const std::vector<size_t>& entry_offsets, const std::filesystem::path& host_dir) -> void {
        for (size_t offset : entry_offsets) {
            if (offset + 32 > virtual_disk.size()) {
                break;
            }
            uint8_t first_byte = virtual_disk[offset];
            if (first_byte == 0x00) {
                break;
            }
            if (first_byte == 0xE5 || first_byte == 0x2E) {
                continue;
            }
            uint8_t attr = virtual_disk[offset + 11];
            if (attr == 0x0F || (attr & 0x08) != 0) {
                continue;
            }

            std::string name;
            for (int i = 0; i < 8; ++i) {
                char c = static_cast<char>(virtual_disk[offset + i]);
                if (c != ' ') {
                    name += c;
                }
            }
            std::string ext;
            for (int i = 8; i < 11; ++i) {
                char c = static_cast<char>(virtual_disk[offset + i]);
                if (c != ' ') {
                    ext += c;
                }
            }
            if (name.empty()) {
                continue;
            }
            std::string sfn_name = ext.empty() ? name : (name + "." + ext);

            std::string sfn(reinterpret_cast<const char*>(&virtual_disk[offset]), 11);
            std::filesystem::path host_target;
            auto it = sfnToHostMap.find(host_dir.string() + "/" + sfn);
            if (it != sfnToHostMap.end() && std::filesystem::exists(it->second)) {
                host_target = it->second;
            } else {
                std::string target_name = sfn_name;
                if (std::filesystem::exists(host_dir)) {
                    for (const auto& entry : std::filesystem::directory_iterator(host_dir, std::filesystem::directory_options::skip_permission_denied)) {
                        std::string cur_name = entry.path().filename().string();
                        if (cur_name.size() == sfn_name.size()) {
                            bool match = true;
                            for (size_t i = 0; i < sfn_name.size(); ++i) {
                                if (std::tolower(static_cast<unsigned char>(cur_name[i])) !=
                                    std::tolower(static_cast<unsigned char>(sfn_name[i]))) {
                                    match = false;
                                    break;
                                }
                            }
                            if (match) {
                                target_name = cur_name;
                                break;
                            }
                        }
                    }
                }
                host_target = host_dir / target_name;
            }

            std::filesystem::path weakly_target = std::filesystem::weakly_canonical(host_target, ec);
            if (ec || !IsPathContained(weakly_target, root_host)) {
                continue;
            }

            uint16_t first_cluster = static_cast<uint16_t>(virtual_disk[offset + 26] | (virtual_disk[offset + 27] << 8));
            uint32_t file_size = static_cast<uint32_t>(virtual_disk[offset + 28] | (virtual_disk[offset + 29] << 8) |
                                                        (virtual_disk[offset + 30] << 16) | (virtual_disk[offset + 31] << 24));

            if ((attr & 0x10) != 0) {
                std::filesystem::create_directories(host_target, ec);
                synced_paths.insert(weakly_target);
                std::error_code ec_c;
                std::filesystem::path can_target = std::filesystem::canonical(host_target, ec_c);
                if (!ec_c) {
                    synced_paths.insert(can_target);
                }
                if (first_cluster >= 2) {
                    std::vector<size_t> sub_offsets = readClusterChain(first_cluster);
                    self(self, sub_offsets, host_target);
                }
            } else {
                synced_paths.insert(weakly_target);
                std::error_code ec_c;
                std::filesystem::path can_target = std::filesystem::canonical(host_target, ec_c);
                if (!ec_c) {
                    synced_paths.insert(can_target);
                }
                std::vector<uint8_t> file_data;
                file_data.reserve(file_size);
                uint16_t cur = first_cluster;
                uint32_t left = file_size;
                std::set<uint16_t> visited;
                while (cur >= 2 && cur < 0xFFF8 && left > 0) {
                    if (!visited.insert(cur).second) {
                        break;
                    }
                    size_t clus_offset = 51200 + static_cast<size_t>(cur - 2) * 4096;
                    if (clus_offset >= virtual_disk.size()) {
                        break;
                    }
                    size_t chunk = std::min<size_t>(left, 4096);
                    if (clus_offset + chunk > virtual_disk.size()) {
                        break;
                    }
                    file_data.insert(file_data.end(), virtual_disk.begin() + clus_offset,
                                     virtual_disk.begin() + clus_offset + chunk);
                    left -= static_cast<uint32_t>(chunk);
                    cur = getFatEntry(cur);
                }

                bool need_write = true;
                if (std::filesystem::exists(host_target, ec) && std::filesystem::is_regular_file(host_target, ec)) {
                    if (std::filesystem::file_size(host_target, ec) == file_data.size()) {
                        std::ifstream in_check(host_target, std::ios::binary);
                        std::vector<uint8_t> existing_data(file_data.size());
                        if (in_check.read(reinterpret_cast<char*>(existing_data.data()),
                                         static_cast<std::streamsize>(file_data.size()))) {
                            if (existing_data == file_data) {
                                need_write = false;
                            }
                        } else if (file_data.empty()) {
                            need_write = false;
                        }
                    }
                }

                if (need_write) {
                    std::filesystem::create_directories(host_target.parent_path(), ec);
                    std::ofstream out_file(host_target, std::ios::binary | std::ios::trunc);
                    if (out_file.is_open()) {
                        if (!file_data.empty()) {
                            out_file.write(reinterpret_cast<const char*>(file_data.data()),
                                           static_cast<std::streamsize>(file_data.size()));
                        }
                    }
                }
            }
        }
    };

    std::vector<size_t> root_offsets;
    root_offsets.reserve(512);
    for (size_t i = 0; i < 512; ++i) {
        root_offsets.push_back(34816 + i * 32);
    }
    processEntries(processEntries, root_offsets, root_host);

    if (allow_host_deletion) {
        std::vector<std::filesystem::path> host_items;
        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(
                     root_host, std::filesystem::directory_options::skip_permission_denied)) {
                std::string fname = entry.path().filename().string();
                if (!fname.empty() && fname[0] == '.') {
                    continue;
                }
                host_items.push_back(entry.path());
            }
        } catch (...) {
        }

        std::sort(host_items.begin(), host_items.end(), [](const auto& a, const auto& b) {
            return a.string().length() > b.string().length();
        });

        for (const auto& item : host_items) {
            std::error_code ec_del;
            std::filesystem::path can_item = std::filesystem::weakly_canonical(item, ec_del);
            if (ec_del) {
                continue;
            }
            if (synced_paths.find(can_item) == synced_paths.end()) {
                std::filesystem::remove(item, ec_del);
            }
        }
    }
}

bool SDCard::SyncFromHost() {
    if (!is_folder_mount || currentPath.empty()) {
        return false;
    }
    if (is_modified) {
        SyncToHost();
    }
    return BuildVirtualFAT();
}

bool SDCard::SyncToHost() {
    if (!is_folder_mount || currentPath.empty()) {
        return false;
    }
    ExtractVirtualFAT();
    is_modified = false;
    return true;
}

void SDCard::SetCS(bool csVal) {
    // CS is active low
    cs_active = !csVal;

    if (!cs_active) {
        if (state != State::WRITE_BUSY && state != State::CMD12_BUSY) {
            state = State::IDLE;
            cmd_bytes_received = 0;
            response_buffer.clear();
        }
    }
}

Byte SDCard::Read(Word address) {
    switch (address & 0x03U) {
        case 0:  // CTRL
            return (mounted ? 0x01U : 0x00U);
        case 1:  // DATA
            return last_miso;
        default:
            return 0xFFU;
    }
}

void SDCard::Write(Word address, Byte data) {
    switch (address & 0x03U) {
        case 0:  // CTRL
            SetCS((data & 0x01U) == 0); // If 0, then high (inactive), if 1, then low (active)
            break;
        case 1:  // DATA
            last_mosi = data;
            last_miso = TransferByte(data);
            break;
        default:
            break;
    }
}

uint8_t SDCard::TransferByte(uint8_t mosi) {
    if (!mounted) {
        return 0xFFU;
    }
    if (!cs_active) {
        if (!is_initialized && warmup_bytes < 10) {
            warmup_bytes++;
        }
        return 0xFFU;
    }

    uint8_t miso = 0xFFU;
    switch (state) {
        case State::IDLE:
            HandleIdleState(mosi);
            break;
        case State::COMMAND_RECEIVE:
            HandleCommandReceiveState(mosi);
            break;
        case State::WAIT_RESPONSE:
            HandleWaitResponseState(miso);
            break;
        case State::SEND_RESPONSE:
            HandleSendResponseState(miso);
            break;
        case State::READ_PENDING:
            HandleReadPendingState(mosi, miso);
            break;
        case State::READ_DATA_DELAY:
            HandleReadDataDelayState(mosi, miso);
            break;
        case State::READ_DATA_TOKEN:
            HandleReadDataTokenState(mosi, miso);
            break;
        case State::READ_DATA_BLOCK:
            HandleReadDataBlockState(mosi, miso);
            break;
        case State::READ_DATA_CRC:
            HandleReadDataCrcState(mosi, miso);
            break;
        case State::WRITE_DATA_TOKEN:
            HandleWriteDataTokenState(mosi);
            break;
        case State::WRITE_DATA_BLOCK:
            HandleWriteDataBlockState(mosi);
            break;
        case State::WRITE_DATA_CRC:
            HandleWriteDataCrcState(mosi, miso);
            break;
        case State::WRITE_BUSY:
            HandleWriteBusyState(miso);
            break;
        case State::CMD12_BUSY:
            HandleCmd12BusyState(miso);
            break;
        default:
            state = State::IDLE;
            break;
    }
    return miso;
}

void SDCard::HandleIdleState(uint8_t mosi) {
    if ((mosi & 0xC0U) == 0x40U) {  // Command starts with 01
        if (!is_initialized && warmup_bytes < 10) {
            return; // Ignore command if warmup is not complete
        }
        cmd_buffer[0] = mosi;
        cmd_bytes_received = 1;
        state = State::COMMAND_RECEIVE;
    }
}

void SDCard::HandleCommandReceiveState(uint8_t mosi) {
    cmd_buffer.at(static_cast<size_t>(cmd_bytes_received++)) = mosi;
    if (cmd_bytes_received == 6) {
        ProcessCommand();
        // State is now WAIT_RESPONSE (from ProcessCommand's QueueResponse calls)
    }
}

void SDCard::HandleWaitResponseState(uint8_t& miso) {
    // Standard commands require a 1-8 byte delay before sending the response.
    // Returning 0xFF here provides a 1-byte gap, making the response
    // available in the very next byte transfer.
    miso = 0xFFU;
    state = State::SEND_RESPONSE;
}

void SDCard::HandleSendResponseState(uint8_t& miso) {
    if (static_cast<size_t>(response_index) < response_buffer.size()) {
        miso = response_buffer.at(static_cast<size_t>(response_index++));
        if (static_cast<size_t>(response_index) == response_buffer.size()) {
            response_buffer.clear();
            // Transition based on the last command
            uint8_t last_cmd = cmd_buffer[0] & 0x3FU;
            if ((last_cmd == 24U || last_cmd == 25U) && !is_acmd) {
                state = State::WRITE_DATA_TOKEN;
            } else if ((last_cmd == 17U || last_cmd == 18U) && !is_acmd) {
                state = State::READ_PENDING;
            } else if (last_cmd == 12U && !is_acmd) {
                state = State::CMD12_BUSY;
            } else {
                state = State::IDLE;
                sd_status_byte = 0x00; // Clear status on normal completion
            }
        }
    } else {
        state = State::IDLE;
        sd_status_byte = 0x00; // Clear status on normal completion
    }
}

bool SDCard::CheckForStopCommand(uint8_t mosi) {
    if (is_read_multiblock && mosi == 0x4CU) {
        // Abort read
        is_read_multiblock = false;
        cmd_buffer[0] = mosi;
        cmd_bytes_received = 1;
        state = State::COMMAND_RECEIVE;
        return true;
    }
    return false;
}

void SDCard::HandleReadPendingState(uint8_t mosi, uint8_t& miso) {
    if (CheckForStopCommand(mosi)) {
        miso = 0xFFU;
        return;
    }
    miso = 0xFFU;
    current_crc = CalculateCrc16(data_buffer);
    if (read_latency_enabled) {
        read_delay_bytes = static_cast<uint8_t>((std::rand() % 20) + 5);
    } else {
        read_delay_bytes = 0;
    }
    state = State::READ_DATA_DELAY;
}

void SDCard::HandleReadDataDelayState(uint8_t mosi, uint8_t& miso) {
    if (CheckForStopCommand(mosi)) {
        miso = 0xFFU;
        return;
    }
    miso = 0xFFU;
    if (read_delay_bytes > 0) {
        read_delay_bytes--;
    } else {
        state = State::READ_DATA_TOKEN;
    }
}

void SDCard::HandleReadDataTokenState(uint8_t mosi, uint8_t& miso) {
    if (CheckForStopCommand(mosi)) {
        miso = 0xFFU;
        return;
    }
    // Output token 0xFE before data block
    miso = 0xFEU;
    data_index = 0;
    state = State::READ_DATA_BLOCK;
}

void SDCard::HandleReadDataBlockState(uint8_t mosi, uint8_t& miso) {
    if (CheckForStopCommand(mosi)) {
        miso = 0xFFU;
        return;
    }
    miso = data_buffer.at(static_cast<size_t>(data_index++));
    if (data_index == 512) {
        state = State::READ_DATA_CRC;
        data_index = 0;
    }
}

void SDCard::HandleReadDataCrcState(uint8_t mosi, uint8_t& miso) {
    if (CheckForStopCommand(mosi)) {
        miso = 0xFFU;
        return;
    }
    if (data_index == 0) {
        miso = static_cast<uint8_t>((current_crc >> 8) & 0xFFU);
    } else {
        miso = static_cast<uint8_t>(current_crc & 0xFFU);
    }
    data_index++;
    if (data_index == 2) {
        if (is_read_multiblock) {
            current_lba++;
            if (current_lba >= total_blocks) {
                state = State::IDLE;
            } else {
                ReadBlockFromImage();
                state = State::READ_PENDING;
            }
        } else {
            state = State::IDLE;
        }
    }
}

void SDCard::HandleWriteDataTokenState(uint8_t mosi) {
    if (mosi == 0xFEU || (is_write_multiblock && mosi == 0xFCU)) {  // Start block token
        data_index = 0;
        state = State::WRITE_DATA_BLOCK;
    } else if (is_write_multiblock && mosi == 0xFDU) { // Stop Tran token
        is_write_multiblock = false;
        write_busy_bytes = 5;
        state = State::WRITE_BUSY;
    } else if ((mosi & 0xF0U) == 0xF0U) {  // Dummy bytes before token
        // Do nothing
    } else {
        // Unexpected token
        state = State::IDLE;
    }
}

void SDCard::HandleWriteDataBlockState(uint8_t mosi) {
    data_buffer.at(static_cast<size_t>(data_index++)) = mosi;
    if (data_index == 512) {
        state = State::WRITE_DATA_CRC;
        data_index = 0;
    }
}

void SDCard::HandleWriteDataCrcState(uint8_t mosi, uint8_t& miso) {
    if (data_index == 0) {
        received_crc = static_cast<uint16_t>(mosi) << 8;
    } else if (data_index == 1) {
        received_crc |= static_cast<uint16_t>(mosi);
    }

    data_index++;
    if (data_index == 3) {
        if (crc_enabled && received_crc != CalculateCrc16(data_buffer)) {
            // CRC Error: xxx01011b = 0x0B -> CRC Error
            miso = 0x0BU;
            state = State::IDLE;
        } else {
            WriteBlockToImage();
            if (is_write_multiblock) {
                current_lba++;
            }
            // Send Data Response:
            // xxx00101b = 0x05 -> Accepted
            miso = 0x05U;
            write_busy_bytes = 5;
            state = State::WRITE_BUSY;
        }
    }
}

void SDCard::HandleWriteBusyState(uint8_t& miso) {
    miso = 0x00U;
    if (write_busy_bytes > 0) {
        write_busy_bytes--;
    } else {
        miso = 0xFFU;
        if (is_write_multiblock) {
            state = State::WRITE_DATA_TOKEN;
        } else {
            state = State::IDLE;
        }
    }
}

void SDCard::HandleCmd12BusyState(uint8_t& miso) {
    if (cmd12_busy_bytes > 0) {
        cmd12_busy_bytes--;
        miso = 0x00U;
    } else {
        miso = 0xFFU;
        state = State::IDLE;
        sd_status_byte = 0x00; // Clear status
    }
}

void SDCard::ProcessCommand() {
    uint8_t cmd = cmd_buffer.at(0) & 0x3FU;
    uint32_t arg = (static_cast<uint32_t>(cmd_buffer.at(1)) << 24) | (static_cast<uint32_t>(cmd_buffer.at(2)) << 16) |
                   (static_cast<uint32_t>(cmd_buffer.at(3)) << 8) | static_cast<uint32_t>(cmd_buffer.at(4));

    response_index = 0;
    response_buffer.clear();

    if (cmd == 0 || cmd == 8) {
        if (cmd_buffer.at(5) != CalculateCrc7(cmd_buffer)) {
            QueueResponse1(0x09U);  // R1: Idle + CRC error
            return;
        }
    }

    if (is_acmd) {
        HandleAcmd(cmd, arg);
    } else {
        HandleStandardCmd(cmd, arg);
    }
}

void SDCard::HandleAcmd(uint8_t cmd, uint32_t arg) {
    is_acmd = false;
    switch (cmd) {
        case 41:
            HandleAcmd41(arg);
            break;
        default:
            QueueResponse1(0x04U);  // R1: Illegal command
            break;
    }
}

void SDCard::HandleAcmd41(uint32_t arg) {
    if (acmd41_attempts < acmd41_target_attempts) {
        acmd41_attempts++;
        QueueResponse1(0x01U);  // R1: In Idle State
    } else {
        is_sdhc = ((arg >> 30) & 1U) != 0; // HCS bit
        QueueResponse1(0x00U);  // R1: Ready (no idle bit)
        is_initialized = true;
    }
}

void SDCard::HandleStandardCmd(uint8_t cmd, uint32_t arg) {
    switch (cmd) {
        case 0:
            HandleCmd0();
            break;
        case 8:
            HandleCmd8(arg);
            break;
        case 12:
            HandleCmd12();
            break;
        case 13:
            HandleCmd13();
            break;
        case 16:
            HandleCmd16();
            break;
        case 17:
            HandleCmd17(arg);
            break;
        case 18:
            HandleCmd18(arg);
            break;
        case 24:
            HandleCmd24(arg);
            break;
        case 25:
            HandleCmd25(arg);
            break;
        case 55:
            HandleCmd55();
            break;
        case 58:
            HandleCmd58();
            break;
        case 59:
            HandleCmd59(arg);
            break;
        default:
            QueueResponse1(is_initialized ? 0x04U : 0x05U);  // R1: Illegal command
            break;
    }
}

void SDCard::HandleCmd0() {
    QueueResponse1(0x01U);  // R1: In Idle State
    is_initialized = false;
    acmd41_attempts = 0;
    acmd41_target_attempts = static_cast<uint8_t>((std::rand() % 6) + 1);
    if (is_folder_mount && !currentPath.empty()) {
        SyncFromHost();
    }
}

void SDCard::HandleCmd8(uint32_t arg) {
    QueueResponse7(0x01U, arg & 0xFFFU);  // Echo back voltage pattern
}

void SDCard::HandleCmd12() {
    is_read_multiblock = false;
    cmd12_busy_bytes = 2; // Busy for a couple of bytes
    QueueResponse1(0x00U);
}

void SDCard::HandleCmd13() {
    QueueResponse2(is_initialized ? 0x00U : 0x01U, sd_status_byte); // R2 response
}

void SDCard::HandleCmd16() {
    // We always read/write 512 bytes
    QueueResponse1(is_initialized ? 0x00U : 0x01U);
}

bool SDCard::PrepareBlockTransfer(uint32_t arg, State nextState, bool isMultiblock, bool isWrite) {
    (void)nextState;
    if (!is_initialized) {
        QueueResponse1(0x05U);  // R1: Illegal command + In Idle State
        return false;
    }
    if (arg >= total_blocks && is_sdhc) {
        sd_status_byte |= 0x40U;
        QueueResponse1(0x40U); // R1: Parameter Error
        return false;
    }
    current_lba = arg;
    if (!is_sdhc) {
        current_lba /= 512U;
        if (current_lba >= total_blocks) {
            sd_status_byte |= 0x40U;
            QueueResponse1(0x40U);
            return false;
        }
    }
    if (isWrite) {
        is_write_multiblock = isMultiblock;
    } else {
        is_read_multiblock = isMultiblock;
        ReadBlockFromImage();
    }
    QueueResponse1(0x00U);  // R1: Success
    return true;
}

void SDCard::HandleCmd17(uint32_t arg) {
    PrepareBlockTransfer(arg, State::READ_PENDING, false, false);
}

void SDCard::HandleCmd18(uint32_t arg) {
    PrepareBlockTransfer(arg, State::READ_PENDING, true, false);
}

void SDCard::HandleCmd24(uint32_t arg) {
    PrepareBlockTransfer(arg, State::WRITE_DATA_TOKEN, false, true);
}

void SDCard::HandleCmd25(uint32_t arg) {
    PrepareBlockTransfer(arg, State::WRITE_DATA_TOKEN, true, true);
}

void SDCard::HandleCmd55() {
    QueueResponse1(is_initialized ? 0x00U : 0x01U);
    is_acmd = true;
}

void SDCard::HandleCmd58() {
    // Return OCR. Bit 30 (CCS) = is_sdhc ? 1 : 0
    uint32_t ocr = is_sdhc ? 0xC0FF8000U : 0x80FF8000U;
    QueueResponse3(is_initialized ? 0x00U : 0x01U, ocr);
}

void SDCard::HandleCmd59(uint32_t arg) {
    crc_enabled = (arg & 1U) != 0;
    QueueResponse1(is_initialized ? 0x00U : 0x01U);
}

void SDCard::QueueResponse1(uint8_t response1) {
    response_buffer.clear();
    response_buffer.push_back(response1);
    response_index = 0;
    state = State::WAIT_RESPONSE;
}

void SDCard::QueueResponse2(uint8_t response1, uint8_t response2) {
    response_buffer.clear();
    response_buffer.push_back(response1);
    response_buffer.push_back(response2);
    response_index = 0;
    state = State::WAIT_RESPONSE;
}

void SDCard::QueueResponse3(uint8_t response1, uint32_t response3) {
    response_buffer.clear();
    response_buffer.push_back(response1);
    response_buffer.push_back(static_cast<uint8_t>((response3 >> 24) & 0xFFU));
    response_buffer.push_back(static_cast<uint8_t>((response3 >> 16) & 0xFFU));
    response_buffer.push_back(static_cast<uint8_t>((response3 >> 8) & 0xFFU));
    response_buffer.push_back(static_cast<uint8_t>(response3 & 0xFFU));
    response_index = 0;
    state = State::WAIT_RESPONSE;
}

void SDCard::QueueResponse7(uint8_t response1, uint32_t response7) {
    QueueResponse3(response1, response7);
}

void SDCard::ReadBlockFromImage() {
    if (is_folder_mount) {
        if (current_lba < total_blocks && (static_cast<size_t>(current_lba) + 1) * 512 <= virtual_disk.size()) {
            std::memcpy(data_buffer.data(), virtual_disk.data() + (static_cast<size_t>(current_lba) * 512), 512);
        } else {
            data_buffer.fill(0xFFU);
        }
        return;
    }

    if (!mounted || !imageFile.is_open()) {
        data_buffer.fill(0xFFU);
        return;
    }

    imageFile.clear();  // Clear any EOF flags
    imageFile.seekg(static_cast<std::streamoff>(current_lba) * 512, std::ios::beg);
    ISerializable::Deserialize(imageFile, data_buffer);
}

void SDCard::WriteBlockToImage() {
    if (is_folder_mount) {
        if (is_read_only) {
            return;
        }
        if (current_lba < total_blocks && (static_cast<size_t>(current_lba) + 1) * 512 <= virtual_disk.size()) {
            std::memcpy(virtual_disk.data() + (static_cast<size_t>(current_lba) * 512), data_buffer.data(), 512);
            is_modified = true;
        }
        return;
    }

    if (!mounted || !imageFile.is_open() || is_read_only) {
        return;
    }

    imageFile.clear();
    imageFile.seekp(static_cast<std::streamoff>(current_lba) * 512, std::ios::beg);
    ISerializable::Serialize(imageFile, data_buffer);
    imageFile.flush();
    is_modified = true;
}

bool SDCard::SaveState(std::ostream& out) const {
    std::string verStr = "SD_VER:" + std::string(PROJECT_VERSION);
    ISerializable::Serialize(out, verStr);

    ISerializable::Serialize(out, currentPath);

    ISerializable::Serialize(out, mounted);
    ISerializable::Serialize(out, cs_active);
    ISerializable::Serialize(out, state);
    ISerializable::Serialize(out, last_miso);
    ISerializable::Serialize(out, cmd_buffer);
    ISerializable::Serialize(out, cmd_bytes_received);

    ISerializable::Serialize(out, response_buffer);

    ISerializable::Serialize(out, response_index);
    ISerializable::Serialize(out, data_buffer);
    ISerializable::Serialize(out, data_index);
    ISerializable::Serialize(out, current_lba);
    ISerializable::Serialize(out, is_acmd);
    ISerializable::Serialize(out, is_initialized);
    ISerializable::Serialize(out, is_sdhc);
    ISerializable::Serialize(out, is_read_multiblock);
    ISerializable::Serialize(out, is_write_multiblock);
    ISerializable::Serialize(out, sd_status_byte);

    ISerializable::Serialize(out, is_read_only);
    ISerializable::Serialize(out, warmup_bytes);
    ISerializable::Serialize(out, acmd41_attempts);
    ISerializable::Serialize(out, acmd41_target_attempts);
    ISerializable::Serialize(out, write_busy_bytes);
    ISerializable::Serialize(out, cmd12_busy_bytes);
    
    ISerializable::Serialize(out, read_delay_bytes);
    ISerializable::Serialize(out, crc_enabled);
    ISerializable::Serialize(out, read_latency_enabled);
    ISerializable::Serialize(out, current_crc);
    ISerializable::Serialize(out, received_crc);
    ISerializable::Serialize(out, total_blocks);

    return out.good();
}

bool SDCard::LoadState(std::istream& inStream) {
    std::string firstStr;
    ISerializable::Deserialize(inStream, firstStr);
    
    bool is_old_format = false;
    if (firstStr.find("SD_VER:") == 0) {
        ISerializable::Deserialize(inStream, currentPath);
    } else {
        is_old_format = true;
        currentPath = firstStr;
    }

    ISerializable::Deserialize(inStream, mounted);
    ISerializable::Deserialize(inStream, cs_active);
    ISerializable::Deserialize(inStream, state);
    ISerializable::Deserialize(inStream, last_miso);
    ISerializable::Deserialize(inStream, cmd_buffer);
    ISerializable::Deserialize(inStream, cmd_bytes_received);

    ISerializable::Deserialize(inStream, response_buffer);

    ISerializable::Deserialize(inStream, response_index);
    ISerializable::Deserialize(inStream, data_buffer);
    ISerializable::Deserialize(inStream, data_index);
    ISerializable::Deserialize(inStream, current_lba);
    ISerializable::Deserialize(inStream, is_acmd);
    ISerializable::Deserialize(inStream, is_initialized);
    ISerializable::Deserialize(inStream, is_sdhc);

    if (is_old_format) {
        is_read_only = false;
        warmup_bytes = 10;
        acmd41_attempts = 0;
        acmd41_target_attempts = 1;
        write_busy_bytes = 0;
        cmd12_busy_bytes = 0;
        read_delay_bytes = 0;
        crc_enabled = false;
        read_latency_enabled = false;
        current_crc = 0;
        received_crc = 0;
        is_read_multiblock = false;
        is_write_multiblock = false;
        total_blocks = 0;
        sd_status_byte = 0x00;
    } else {
        ISerializable::Deserialize(inStream, is_read_multiblock);
        ISerializable::Deserialize(inStream, is_write_multiblock);
        ISerializable::Deserialize(inStream, sd_status_byte);
        ISerializable::Deserialize(inStream, is_read_only);
        ISerializable::Deserialize(inStream, warmup_bytes);
        ISerializable::Deserialize(inStream, acmd41_attempts);
        ISerializable::Deserialize(inStream, acmd41_target_attempts);
        ISerializable::Deserialize(inStream, write_busy_bytes);
        ISerializable::Deserialize(inStream, cmd12_busy_bytes);
        ISerializable::Deserialize(inStream, read_delay_bytes);
        ISerializable::Deserialize(inStream, crc_enabled);
        ISerializable::Deserialize(inStream, read_latency_enabled);
        ISerializable::Deserialize(inStream, current_crc);
        ISerializable::Deserialize(inStream, received_crc);
        ISerializable::Deserialize(inStream, total_blocks);
    }

    if (imageFile.is_open()) {
        imageFile.close();
    }
    if (mounted && !currentPath.empty()) {
        if (std::filesystem::is_directory(PathFromUtf8(currentPath))) {
            MountFolder(currentPath);
        } else {
            imageFile.open(PathFromUtf8(currentPath), std::ios::in | std::ios::out | std::ios::binary);
            if (!imageFile.is_open()) {
                imageFile.clear();
                imageFile.open(PathFromUtf8(currentPath), std::ios::in | std::ios::binary);
                if (imageFile.is_open()) {
                    is_read_only = true;
                }
            } else {
                is_read_only = false;
            }
            if (imageFile.is_open()) {
                imageFile.seekg(0, std::ios::end);
                total_blocks = static_cast<uint32_t>(imageFile.tellg() / 512);
                imageFile.seekg(0, std::ios::beg);
            } else {
                mounted = false;
            }
        }
    }

    return inStream.good();
}

uint8_t SDCard::CalculateCrc7(const std::array<std::uint8_t, 6>& buffer) const {
    uint8_t crc = 0;
    for (int i = 0; i < 5; i++) {
        crc ^= buffer.at(static_cast<size_t>(i));
        for (int j = 0; j < 8; j++) {
            if ((crc & 0x80U) != 0) {
                crc ^= 0x89U;
            }
            crc = static_cast<uint8_t>(crc << 1);
        }
    }
    return static_cast<uint8_t>(crc | 1U);
}

uint16_t SDCard::CalculateCrc16(const std::array<std::uint8_t, 512>& buffer) const {
    uint16_t crc = 0;
    for (size_t i = 0; i < buffer.size(); i++) {
        crc ^= static_cast<uint16_t>(buffer.at(i)) << 8;
        for (int j = 0; j < 8; j++) {
            if ((crc & 0x8000U) != 0) {
                crc = static_cast<uint16_t>((crc << 1) ^ 0x1021U);
            } else {
                crc = static_cast<uint16_t>(crc << 1);
            }
        }
    }
    return crc;
}

}  // namespace Hardware
