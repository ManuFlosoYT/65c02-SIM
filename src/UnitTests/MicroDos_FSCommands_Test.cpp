#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <initializer_list>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

struct WcResult {
    size_t lines{0};
    size_t words{0};
    size_t bytes{0};

    auto operator<=>(const WcResult&) const = default;
};

struct WcFlags {
    bool lines{false};
    bool words{false};
    bool bytes{false};

    [[nodiscard]] constexpr bool any() const noexcept {
        return lines || words || bytes;
    }

    auto operator<=>(const WcFlags&) const = default;
};

[[nodiscard]] constexpr bool is_ascii_whitespace(char c) noexcept {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

[[nodiscard]] constexpr WcResult count_wc(std::string_view text) noexcept {
    WcResult result{};
    result.bytes = text.size();

    bool in_word = false;
    for (const char c : text) {
        if (c == '\n') {
            ++result.lines;
        }
        if (is_ascii_whitespace(c)) {
            in_word = false;
        } else if (!in_word) {
            in_word = true;
            ++result.words;
        }
    }
    return result;
}

[[nodiscard]] constexpr WcFlags parse_wc_flags(std::span<const std::string_view> args) noexcept {
    WcFlags flags{};
    for (const auto& arg : args) {
        if (arg == "-l") {
            flags.lines = true;
        } else if (arg == "-w") {
            flags.words = true;
        } else if (arg == "-c") {
            flags.bytes = true;
        }
    }
    return flags;
}

[[nodiscard]] std::string format_wc_output(const WcResult& result, const WcFlags& flags) {
    const bool show_all = !flags.any();
    const bool show_lines = show_all || flags.lines;
    const bool show_words = show_all || flags.words;
    const bool show_bytes = show_all || flags.bytes;

    std::vector<std::string> parts;
    if (show_lines) {
        parts.push_back(std::format("{}", result.lines));
    }
    if (show_words) {
        parts.push_back(std::format("{}", result.words));
    }
    if (show_bytes) {
        parts.push_back(std::format("{}", result.bytes));
    }

    std::string out;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) {
            out.push_back(' ');
        }
        out.append(parts[i]);
    }
    return out;
}

[[nodiscard]] constexpr bool should_wc_show_usage(size_t num_files) noexcept {
    return num_files == 0;
}

[[nodiscard]] std::string append_pipe_stdin_arg(std::string_view cmd, bool has_prev_pipe) {
    std::string out{cmd};
    if (has_prev_pipe) {
        if (!out.ends_with(" -") && out != "-") {
            out.append(" -");
        }
    }
    return out;
}

enum class RmdirStatus {
    Ok,
    DirectoryNotEmpty,
    DirectoryNotFound
};

struct DirEntry {
    std::string name;
    bool is_directory{false};

    auto operator<=>(const DirEntry&) const = default;
};

[[nodiscard]] constexpr bool is_directory_empty(std::span<const DirEntry> entries) noexcept {
    return std::ranges::all_of(entries, [](const DirEntry& entry) noexcept {
        return entry.name == "." || entry.name == "..";
    });
}

class MockFileSystem {
public:
    void create_directory(std::string path, std::vector<DirEntry> entries = {}) {
        directories_[std::move(path)] = std::move(entries);
    }

    [[nodiscard]] bool has_directory(const std::string& path) const {
        return directories_.contains(path);
    }

    [[nodiscard]] const std::vector<DirEntry>* get_entries(const std::string& path) const {
        const auto it = directories_.find(path);
        return it != directories_.end() ? &it->second : nullptr;
    }

    [[nodiscard]] RmdirStatus remove_directory(const std::string& path) {
        const auto it = directories_.find(path);
        if (it == directories_.end()) {
            return RmdirStatus::DirectoryNotFound;
        }
        if (!is_directory_empty(it->second)) {
            return RmdirStatus::DirectoryNotEmpty;
        }
        directories_.erase(it);
        return RmdirStatus::Ok;
    }

private:
    std::unordered_map<std::string, std::vector<DirEntry>> directories_;
};

constexpr uint64_t FAT16_CLUSTER_SIZE = 4096;
constexpr uint64_t DEFAULT_DISK_SIZE_32MB = 32ULL * 1024ULL * 1024ULL;

[[nodiscard]] constexpr uint64_t round_to_fat16_cluster(uint64_t bytes, uint64_t cluster_size = FAT16_CLUSTER_SIZE) noexcept {
    if (bytes == 0) {
        return 0;
    }
    return ((bytes + cluster_size - 1) / cluster_size) * cluster_size;
}

[[nodiscard]] constexpr uint64_t bytes_to_1k_blocks(uint64_t bytes) noexcept {
    return bytes / 1024;
}

struct DfInfo {
    uint64_t total_bytes{0};
    uint64_t used_bytes{0};
    uint64_t available_bytes{0};
    uint64_t total_1k_blocks{0};
    uint64_t used_1k_blocks{0};
    uint64_t available_1k_blocks{0};
    double used_percent{0.0};
    double available_percent{0.0};

    auto operator<=>(const DfInfo&) const = default;
};

[[nodiscard]] constexpr DfInfo calculate_df(uint64_t total_bytes, uint64_t used_bytes) noexcept {
    DfInfo info{};
    info.total_bytes = total_bytes;
    info.used_bytes = std::min(used_bytes, total_bytes);
    info.available_bytes = (total_bytes >= used_bytes) ? (total_bytes - used_bytes) : 0;
    info.total_1k_blocks = bytes_to_1k_blocks(info.total_bytes);
    info.used_1k_blocks = bytes_to_1k_blocks(info.used_bytes);
    info.available_1k_blocks = bytes_to_1k_blocks(info.available_bytes);

    if (total_bytes > 0) {
        info.used_percent = (static_cast<double>(info.used_bytes) / static_cast<double>(total_bytes)) * 100.0;
        info.available_percent = 100.0 - info.used_percent;
    }
    return info;
}

[[nodiscard]] inline std::string FormatSize(uint64_t bytes) {
    if (bytes < 1024) {
        return std::format("{} B", bytes);
    }
    const double kb = static_cast<double>(bytes) / 1024.0;
    if (kb < 1024.0) {
        return std::format("{:.1f} KB", kb);
    }
    const double mb = kb / 1024.0;
    return std::format("{:.1f} MB", mb);
}

[[nodiscard]] inline std::string execute_echo(std::span<const std::string_view> args) {
    bool no_newline = false;
    size_t start_idx = 0;

    if (!args.empty() && args[0] == "-n") {
        no_newline = true;
        start_idx = 1;
    }

    std::string result;
    for (size_t i = start_idx; i < args.size(); ++i) {
        if (i > start_idx) {
            result.push_back(' ');
        }
        result.append(args[i]);
    }

    if (!no_newline) {
        result.push_back('\n');
    }

    return result;
}

[[nodiscard]] inline std::string execute_echo(std::initializer_list<std::string_view> args) {
    return execute_echo(std::span<const std::string_view>(args.begin(), args.size()));
}

constexpr size_t HEXDUMP_BYTES_PER_LINE = 16;

[[nodiscard]] constexpr char to_printable_ascii(uint8_t byte) noexcept {
    return (byte >= 0x20 && byte <= 0x7E) ? static_cast<char>(byte) : '.';
}

struct HexdumpLine {
    size_t offset{0};
    std::vector<uint8_t> bytes;
    std::string hex_view;
    std::string ascii_view;

    auto operator<=>(const HexdumpLine&) const = default;
};

[[nodiscard]] inline std::string format_hex_byte(uint8_t b) {
    return std::format("{:02x}", b);
}

[[nodiscard]] inline std::vector<HexdumpLine> hexdump_chunks(std::span<const uint8_t> data, size_t initial_offset = 0) {
    std::vector<HexdumpLine> lines;
    if (data.empty()) {
        return lines;
    }

    size_t offset = initial_offset;
    for (size_t i = 0; i < data.size(); i += HEXDUMP_BYTES_PER_LINE) {
        const size_t chunk_len = std::min(HEXDUMP_BYTES_PER_LINE, data.size() - i);
        const auto chunk_span = data.subspan(i, chunk_len);

        HexdumpLine line{};
        line.offset = offset;
        line.bytes.assign(chunk_span.begin(), chunk_span.end());

        std::string hex_str;
        std::string ascii_str;
        ascii_str.reserve(chunk_len);

        for (size_t j = 0; j < chunk_len; ++j) {
            if (j > 0) {
                hex_str.push_back(' ');
            }
            hex_str.append(format_hex_byte(chunk_span[j]));
            ascii_str.push_back(to_printable_ascii(chunk_span[j]));
        }

        line.hex_view = std::move(hex_str);
        line.ascii_view = std::move(ascii_str);

        lines.push_back(std::move(line));
        offset += chunk_len;
    }

    return lines;
}

[[nodiscard]] inline std::string normalize_path(std::string_view path) {
    if (path.empty()) {
        return "/";
    }

    std::vector<std::string_view> segments;
    size_t start = 0;

    while (start < path.size()) {
        const size_t slash_pos = path.find('/', start);
        const std::string_view segment = (slash_pos == std::string_view::npos)
                                             ? path.substr(start)
                                             : path.substr(start, slash_pos - start);

        if (!segment.empty() && segment != ".") {
            if (segment == "..") {
                if (!segments.empty()) {
                    segments.pop_back();
                }
            } else {
                segments.push_back(segment);
            }
        }

        if (slash_pos == std::string_view::npos) {
            break;
        }
        start = slash_pos + 1;
    }

    if (segments.empty()) {
        return "/";
    }

    std::string result;
    for (const auto& seg : segments) {
        result.push_back('/');
        result.append(seg);
    }

    return result;
}

[[nodiscard]] inline std::string execute_pwd(std::string_view current_dir = "") {
    return normalize_path(current_dir);
}

}  // namespace

TEST(WcTest, MixedSpacesAndTabsCount) {
    constexpr std::string_view input = "  hello \t world \t\n  foo \t\t bar\n";
    const auto result = count_wc(input);

    EXPECT_EQ(result.lines, 2U);
    EXPECT_EQ(result.words, 4U);
    EXPECT_EQ(result.bytes, input.size());
}

TEST(WcTest, EmptyStream) {
    const auto result = count_wc("");

    EXPECT_EQ(result.lines, 0U);
    EXPECT_EQ(result.words, 0U);
    EXPECT_EQ(result.bytes, 0U);
}

TEST(WcTest, SingleWordWithoutTrailingNewline) {
    constexpr std::string_view input = "antigravity";
    const auto result = count_wc(input);

    EXPECT_EQ(result.lines, 0U);
    EXPECT_EQ(result.words, 1U);
    EXPECT_EQ(result.bytes, 11U);
}

TEST(WcTest, MultiLineWithTrailingEmptyLines) {
    constexpr std::string_view input = "first\nsecond\n\n\n";
    const auto result = count_wc(input);

    EXPECT_EQ(result.lines, 4U);
    EXPECT_EQ(result.words, 2U);
    EXPECT_EQ(result.bytes, input.size());
}

TEST(WcTest, FlagFiltering) {
    constexpr std::string_view input = "hello world\nfoo bar\n";
    const auto result = count_wc(input);

    EXPECT_EQ(result.lines, 2U);
    EXPECT_EQ(result.words, 4U);
    EXPECT_EQ(result.bytes, 20U);

    EXPECT_EQ(format_wc_output(result, {.lines = true}), "2");
    EXPECT_EQ(format_wc_output(result, {.words = true}), "4");
    EXPECT_EQ(format_wc_output(result, {.bytes = true}), "20");
    EXPECT_EQ(format_wc_output(result, {.lines = true, .words = true}), "2 4");
    EXPECT_EQ(format_wc_output(result, {}), "2 4 20");

    constexpr std::array<std::string_view, 1> flag_l = {"-l"};
    constexpr std::array<std::string_view, 1> flag_w = {"-w"};
    constexpr std::array<std::string_view, 1> flag_c = {"-c"};
    EXPECT_EQ(format_wc_output(result, parse_wc_flags(flag_l)), "2");
    EXPECT_EQ(format_wc_output(result, parse_wc_flags(flag_w)), "4");
    EXPECT_EQ(format_wc_output(result, parse_wc_flags(flag_c)), "20");
}

TEST(WcTest, ZeroFilesWithoutArgsShowsUsage) {
    EXPECT_TRUE(should_wc_show_usage(0));
    EXPECT_FALSE(should_wc_show_usage(1));
}

TEST(PipelineTest, AppendStdinDashWhenPiped) {
    EXPECT_EQ(append_pipe_stdin_arg("wc", true), "wc -");
    EXPECT_EQ(append_pipe_stdin_arg("wc -l", true), "wc -l -");
    EXPECT_EQ(append_pipe_stdin_arg("grep app", true), "grep app -");
    EXPECT_EQ(append_pipe_stdin_arg("wc -", true), "wc -");
    EXPECT_EQ(append_pipe_stdin_arg("wc", false), "wc");
}

TEST(RmdirTest, EmptyDirectoryValidationPasses) {
    MockFileSystem fs;
    fs.create_directory("/tmp/empty_dir", {});

    EXPECT_TRUE(fs.has_directory("/tmp/empty_dir"));
    EXPECT_EQ(fs.remove_directory("/tmp/empty_dir"), RmdirStatus::Ok);
    EXPECT_FALSE(fs.has_directory("/tmp/empty_dir"));
}

TEST(RmdirTest, NonEmptyDirectoryValidationFails) {
    MockFileSystem fs;
    fs.create_directory("/tmp/non_empty", {
        DirEntry{.name = "test.txt", .is_directory = false}
    });

    EXPECT_TRUE(fs.has_directory("/tmp/non_empty"));
    EXPECT_EQ(fs.remove_directory("/tmp/non_empty"), RmdirStatus::DirectoryNotEmpty);
    EXPECT_TRUE(fs.has_directory("/tmp/non_empty"));
}

TEST(RmdirTest, DotAndDotDotEntriesFiltering) {
    MockFileSystem fs;
    fs.create_directory("/tmp/only_dots", {
        DirEntry{.name = ".", .is_directory = true},
        DirEntry{.name = "..", .is_directory = true}
    });

    EXPECT_EQ(fs.remove_directory("/tmp/only_dots"), RmdirStatus::Ok);
    EXPECT_FALSE(fs.has_directory("/tmp/only_dots"));

    fs.create_directory("/tmp/dots_and_file", {
        DirEntry{.name = ".", .is_directory = true},
        DirEntry{.name = "..", .is_directory = true},
        DirEntry{.name = "child.bin", .is_directory = false}
    });

    EXPECT_EQ(fs.remove_directory("/tmp/dots_and_file"), RmdirStatus::DirectoryNotEmpty);
    EXPECT_TRUE(fs.has_directory("/tmp/dots_and_file"));
}

TEST(RmdirTest, MissingDirectoryHandling) {
    MockFileSystem fs;
    EXPECT_EQ(fs.remove_directory("/does/not/exist"), RmdirStatus::DirectoryNotFound);
}

TEST(DfTest, FAT16ClusterAllocationRounding) {
    EXPECT_EQ(round_to_fat16_cluster(0), 0U);
    EXPECT_EQ(round_to_fat16_cluster(1), 4096U);
    EXPECT_EQ(round_to_fat16_cluster(4096), 4096U);
    EXPECT_EQ(round_to_fat16_cluster(4097), 8192U);
    EXPECT_EQ(round_to_fat16_cluster(8192), 8192U);
    EXPECT_EQ(round_to_fat16_cluster(8193), 12288U);
}

TEST(DfTest, OneKBlocksCalculationFromTotalDiskSize) {
    EXPECT_EQ(bytes_to_1k_blocks(DEFAULT_DISK_SIZE_32MB), 32768U);
}

TEST(DfTest, AvailableSpaceAndPercentageCalculation) {
    constexpr uint64_t used_bytes = 8ULL * 1024ULL * 1024ULL;
    const auto df = calculate_df(DEFAULT_DISK_SIZE_32MB, used_bytes);

    EXPECT_EQ(df.total_bytes, DEFAULT_DISK_SIZE_32MB);
    EXPECT_EQ(df.used_bytes, used_bytes);
    EXPECT_EQ(df.available_bytes, 24ULL * 1024ULL * 1024ULL);
    EXPECT_EQ(df.total_1k_blocks, 32768U);
    EXPECT_EQ(df.used_1k_blocks, 8192U);
    EXPECT_EQ(df.available_1k_blocks, 24576U);
    EXPECT_DOUBLE_EQ(df.used_percent, 25.0);
    EXPECT_DOUBLE_EQ(df.available_percent, 75.0);
}

TEST(DfTest, HumanReadableFormatSize) {
    EXPECT_EQ(FormatSize(0), "0 B");
    EXPECT_EQ(FormatSize(512), "512 B");
    EXPECT_EQ(FormatSize(1024), "1.0 KB");
    EXPECT_EQ(FormatSize(1536), "1.5 KB");
    EXPECT_EQ(FormatSize(4096), "4.0 KB");
    EXPECT_EQ(FormatSize(1048576), "1.0 MB");
    EXPECT_EQ(FormatSize(DEFAULT_DISK_SIZE_32MB), "32.0 MB");
}

TEST(EchoTest, MultipleArgumentsJoinedWithSpaces) {
    EXPECT_EQ(execute_echo({"hello", "world"}), "hello world\n");
    EXPECT_EQ(execute_echo({"alpha", "beta", "gamma"}), "alpha beta gamma\n");
}

TEST(EchoTest, HandlingNFlagNoTrailingNewline) {
    EXPECT_EQ(execute_echo({"-n", "hello", "world"}), "hello world");
    EXPECT_EQ(execute_echo({"-n", "single"}), "single");
    EXPECT_EQ(execute_echo({"hello", "-n"}), "hello -n\n");
}

TEST(EchoTest, EmptyArgumentsListBehavior) {
    EXPECT_EQ(execute_echo({}), "\n");
    EXPECT_EQ(execute_echo({"-n"}), "");
}

TEST(HexdumpTest, StreamChunkingInto16ByteLines) {
    std::array<uint8_t, 32> buffer{};
    for (size_t i = 0; i < buffer.size(); ++i) {
        buffer[i] = static_cast<uint8_t>(i);
    }

    const auto lines = hexdump_chunks(buffer, 0);
    ASSERT_EQ(lines.size(), 2U);

    EXPECT_EQ(lines[0].offset, 0U);
    EXPECT_EQ(lines[0].bytes.size(), 16U);
    EXPECT_EQ(lines[1].offset, 16U);
    EXPECT_EQ(lines[1].bytes.size(), 16U);
}

TEST(HexdumpTest, PartialLastChunk) {
    std::array<uint8_t, 20> buffer{};
    for (size_t i = 0; i < buffer.size(); ++i) {
        buffer[i] = static_cast<uint8_t>('A' + (i % 26));
    }

    const auto lines = hexdump_chunks(buffer, 0);
    ASSERT_EQ(lines.size(), 2U);

    EXPECT_EQ(lines[0].offset, 0U);
    EXPECT_EQ(lines[0].bytes.size(), 16U);
    EXPECT_EQ(lines[1].offset, 16U);
    EXPECT_EQ(lines[1].bytes.size(), 4U);
    EXPECT_EQ(lines[1].ascii_view, "QRST");
}

TEST(HexdumpTest, AsciiColumnConversion) {
    const std::array<uint8_t, 13> data = {
        'H', 'e', 'l', 'l', 'o',
        0x00, 0x0A, 0x09, 0x1F,
        '!', '~',
        0x7F, 0x80
    };

    const auto lines = hexdump_chunks(data, 0);
    ASSERT_EQ(lines.size(), 1U);
    EXPECT_EQ(lines[0].ascii_view, "Hello....!~..");
}

TEST(HexdumpTest, OffsetTrackingAcrossChunks) {
    const std::array<uint8_t, 16> chunk1{};
    const std::array<uint8_t, 16> chunk2{};

    const auto lines1 = hexdump_chunks(chunk1, 0);
    const auto lines2 = hexdump_chunks(chunk2, 16);

    ASSERT_EQ(lines1.size(), 1U);
    ASSERT_EQ(lines2.size(), 1U);
    EXPECT_EQ(lines1[0].offset, 0U);
    EXPECT_EQ(lines2[0].offset, 16U);
}

TEST(PwdTest, FallbackToRootOnEmptyOrRootPath) {
    EXPECT_EQ(execute_pwd(""), "/");
    EXPECT_EQ(execute_pwd(), "/");
    EXPECT_EQ(execute_pwd("/"), "/");
    EXPECT_EQ(execute_pwd("///"), "/");
}

TEST(PwdTest, PathNormalizationWithRelativeSegments) {
    EXPECT_EQ(execute_pwd("/a/b/../c"), "/a/c");
    EXPECT_EQ(execute_pwd("/a/./b/./c"), "/a/b/c");
    EXPECT_EQ(execute_pwd("/a/b/c/.."), "/a/b");
    EXPECT_EQ(execute_pwd("/usr/bin/../../"), "/");
    EXPECT_EQ(execute_pwd("documents/reports"), "/documents/reports");
}

TEST(PwdTest, AboveRootClampingAndMultipleSlashes) {
    EXPECT_EQ(execute_pwd("/.."), "/");
    EXPECT_EQ(execute_pwd("/../../../"), "/");
    EXPECT_EQ(execute_pwd("/a/../../.."), "/");
    EXPECT_EQ(execute_pwd("/var/log/"), "/var/log");
}
