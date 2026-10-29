#include "Hardware/Storage/VirtualFAT16Synchronizer.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iostream>

namespace Hardware {

std::filesystem::path VirtualFAT16Synchronizer::PathFromUtf8(std::string_view str) {
    return std::filesystem::path(std::u8string(str.begin(), str.end()));
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
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(std::chrono::file_clock::to_sys(ft));
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

}  // namespace

std::string VirtualFAT16Synchronizer::GenerateSFN(const std::string& filename, std::set<std::string>& used_sfns) {
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

bool VirtualFAT16Synchronizer::IsProtectedHostPath(const std::filesystem::path& canPath) {
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

void VirtualFAT16Synchronizer::InitializeDiskLayout(std::vector<uint8_t>& virtual_disk) {
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

    virtual_disk[2048] = 0xF8;
    virtual_disk[2049] = 0xFF;
    virtual_disk[2050] = 0xFF;
    virtual_disk[2051] = 0xFF;

    virtual_disk[18432] = 0xF8;
    virtual_disk[18433] = 0xFF;
    virtual_disk[18434] = 0xFF;
    virtual_disk[18435] = 0xFF;
}

bool VirtualFAT16Synchronizer::Build(std::vector<uint8_t>& virtual_disk,
                                    const std::filesystem::path& hostFolder,
                                    std::unordered_map<std::string, std::filesystem::path>& sfnToHostMap) {
    sfnToHostMap.clear();
    if (hostFolder.empty()) {
        return false;
    }

    InitializeDiskLayout(virtual_disk);

    auto setFatEntry = [&virtual_disk](uint16_t cluster, uint16_t val) {
        size_t f1 = 2048 + static_cast<size_t>(cluster) * 2;
        size_t f2 = 18432 + static_cast<size_t>(cluster) * 2;
        if (f2 + 1 < virtual_disk.size()) {
            virtual_disk[f1] = static_cast<uint8_t>(val & 0xFFU);
            virtual_disk[f1 + 1] = static_cast<uint8_t>((val >> 8) & 0xFFU);
            virtual_disk[f2] = static_cast<uint8_t>(val & 0xFFU);
            virtual_disk[f2 + 1] = static_cast<uint8_t>((val >> 8) & 0xFFU);
        }
    };

    uint16_t next_free_cluster = 2;

    auto writeDirEntryAt = [&virtual_disk](size_t offset, const std::string& sfn11, uint8_t attr,
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
                    if (!IsPathContained(entry.path(), hostFolder)) {
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
    std::filesystem::path root_host = std::filesystem::canonical(hostFolder, ec_root);
    if (ec_root) {
        root_host = hostFolder;
    }
    buildDir(buildDir, root_host, 0, 0);
    return true;
}

void VirtualFAT16Synchronizer::Extract(const std::vector<uint8_t>& virtual_disk,
                                       const std::filesystem::path& hostFolder,
                                       const std::unordered_map<std::string, std::filesystem::path>& sfnToHostMap,
                                       bool allowDeletion) {
    if (hostFolder.empty() || virtual_disk.size() < 65536 * 512) {
        return;
    }

    std::error_code ec;
    std::filesystem::path root_host = std::filesystem::canonical(hostFolder, ec);
    if (ec) {
        return;
    }

    auto getFatEntry = [&virtual_disk](uint16_t cluster) -> uint16_t {
        size_t offset = 2048 + static_cast<size_t>(cluster) * 2;
        if (offset + 1 >= virtual_disk.size()) {
            return 0xFFFF;
        }
        return static_cast<uint16_t>(virtual_disk[offset] | (virtual_disk[offset + 1] << 8));
    };

    auto readClusterChain = [&virtual_disk, &getFatEntry](uint16_t start_cluster) {
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

    if (allowDeletion) {
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

}  // namespace Hardware
