#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr size_t MAX_HISTORY_ENTRIES = 16;
constexpr size_t SHELL_MAX_LINE = 128;

[[nodiscard]] constexpr char to_ascii_lower(char c) noexcept {
    if (c >= 'A' && c <= 'Z') {
        return static_cast<char>(c + ('a' - 'A'));
    }
    return c;
}

[[nodiscard]] constexpr std::string_view trim(std::string_view sv) noexcept {
    const auto start = sv.find_first_not_of(" \t\r\n");
    if (start == std::string_view::npos) {
        return {};
    }
    const auto end = sv.find_last_not_of(" \t\r\n");
    return sv.substr(start, end - start + 1);
}

class HistoryManager {
public:
    static constexpr size_t MAX_CAPACITY = MAX_HISTORY_ENTRIES;

    constexpr HistoryManager() = default;

    [[nodiscard]] constexpr size_t size() const noexcept { return entries_.size(); }
    [[nodiscard]] constexpr bool empty() const noexcept { return entries_.empty(); }
    [[nodiscard]] constexpr size_t capacity() const noexcept { return MAX_CAPACITY; }
    [[nodiscard]] const std::vector<std::string>& entries() const noexcept { return entries_; }

    bool add(std::string_view command) {
        const auto trimmed = trim(command);
        if (trimmed.empty()) {
            return false;
        }
        if (!entries_.empty() && entries_.back() == trimmed) {
            return false;
        }
        if (entries_.size() >= MAX_CAPACITY) {
            entries_.erase(entries_.begin());
        }
        entries_.emplace_back(trimmed);
        reset_navigation();
        return true;
    }

    [[nodiscard]] std::optional<std::string_view> navigate_up(std::string_view current_line) {
        if (entries_.empty()) {
            return std::nullopt;
        }
        if (nav_index_ == NOT_NAVIGATING) {
            draft_ = current_line;
            nav_index_ = static_cast<int>(entries_.size()) - 1;
        } else if (nav_index_ > 0) {
            --nav_index_;
        }
        return entries_[static_cast<size_t>(nav_index_)];
    }

    [[nodiscard]] std::optional<std::string_view> navigate_down() {
        if (nav_index_ == NOT_NAVIGATING) {
            return std::nullopt;
        }
        if (nav_index_ + 1 < static_cast<int>(entries_.size())) {
            ++nav_index_;
            return entries_[static_cast<size_t>(nav_index_)];
        }
        nav_index_ = NOT_NAVIGATING;
        return draft_;
    }

    void reset_navigation() noexcept {
        nav_index_ = NOT_NAVIGATING;
        draft_.clear();
    }

    [[nodiscard]] bool is_navigating() const noexcept { return nav_index_ != NOT_NAVIGATING; }

    [[nodiscard]] std::string_view draft() const noexcept { return draft_; }

    [[nodiscard]] std::string serialize() const {
        std::string buffer;
        for (const auto& entry : entries_) {
            buffer.append(entry);
            buffer.push_back('\n');
        }
        return buffer;
    }

    void deserialize(std::string_view content) {
        clear();
        size_t start = 0;
        while (start < content.size()) {
            const size_t end = content.find('\n', start);
            std::string_view line = (end == std::string_view::npos)
                                        ? content.substr(start)
                                        : content.substr(start, end - start);
            if (!line.empty() && line.back() == '\r') {
                line.remove_suffix(1);
            }
            add(line);
            if (end == std::string_view::npos) {
                break;
            }
            start = end + 1;
        }
    }

    void clear() noexcept {
        entries_.clear();
        reset_navigation();
    }

private:
    static constexpr int NOT_NAVIGATING = -1;

    std::vector<std::string> entries_;
    std::string draft_;
    int nav_index_{NOT_NAVIGATING};
};

struct PathSplit {
    std::string dir;
    std::string prefix;

    auto operator<=>(const PathSplit&) const = default;
};

struct DirEntry {
    std::string name;
    bool is_directory{false};

    auto operator<=>(const DirEntry&) const = default;
};

[[nodiscard]] constexpr PathSplit split_path(std::string_view token) {
    if (token.empty()) {
        return PathSplit{.dir = ".", .prefix = ""};
    }
    const size_t slash_pos = token.rfind('/');
    if (slash_pos == std::string_view::npos) {
        return PathSplit{.dir = ".", .prefix = std::string(token)};
    }
    if (slash_pos == 0) {
        return PathSplit{.dir = "/", .prefix = std::string(token.substr(1))};
    }
    return PathSplit{
        .dir = std::string(token.substr(0, slash_pos)),
        .prefix = std::string(token.substr(slash_pos + 1))
    };
}

[[nodiscard]] constexpr bool iequals_prefix(std::string_view candidate, std::string_view prefix) noexcept {
    if (candidate.size() < prefix.size()) {
        return false;
    }
    return std::ranges::equal(
        prefix,
        candidate.substr(0, prefix.size()),
        [](char a, char b) noexcept {
            return to_ascii_lower(a) == to_ascii_lower(b);
        }
    );
}

[[nodiscard]] std::optional<std::string> generate_suffix(const DirEntry& entry, std::string_view prefix) {
    if (!iequals_prefix(entry.name, prefix)) {
        return std::nullopt;
    }
    std::string suffix = entry.name.substr(prefix.size());
    suffix.push_back(entry.is_directory ? '/' : ' ');
    return suffix;
}

[[nodiscard]] bool append_completion_suffix(
    std::span<char> buffer,
    size_t& current_len,
    std::string_view suffix) noexcept {
    if (buffer.empty() || current_len >= buffer.size() - 1) {
        return false;
    }
    const size_t available = buffer.size() - 1 - current_len;
    const size_t to_copy = std::min(available, suffix.size());
    if (to_copy == 0) {
        return false;
    }
    std::memcpy(buffer.data() + current_len, suffix.data(), to_copy);
    current_len += to_copy;
    buffer[current_len] = '\0';
    return to_copy == suffix.size();
}

}  // namespace

TEST(HistoryManagerTest, EmptyOrWhitespaceCommandsIgnored) {
    HistoryManager history;
    EXPECT_FALSE(history.add(""));
    EXPECT_FALSE(history.add("   "));
    EXPECT_FALSE(history.add("\t\t"));
    EXPECT_FALSE(history.add("\n\r "));
    EXPECT_TRUE(history.empty());
    EXPECT_EQ(history.size(), 0U);
}

TEST(HistoryManagerTest, ConsecutiveDuplicatesIgnored) {
    HistoryManager history;
    EXPECT_TRUE(history.add("help"));
    EXPECT_EQ(history.size(), 1U);

    EXPECT_FALSE(history.add("help"));
    EXPECT_EQ(history.size(), 1U);

    EXPECT_FALSE(history.add("  help  "));
    EXPECT_EQ(history.size(), 1U);

    EXPECT_TRUE(history.add("ls"));
    EXPECT_EQ(history.size(), 2U);

    EXPECT_FALSE(history.add("ls"));
    EXPECT_EQ(history.size(), 2U);
}

TEST(HistoryManagerTest, NonConsecutiveDuplicatesAllowed) {
    HistoryManager history;
    EXPECT_TRUE(history.add("cat file.txt"));
    EXPECT_TRUE(history.add("clear"));
    EXPECT_TRUE(history.add("cat file.txt"));
    EXPECT_EQ(history.size(), 3U);
    EXPECT_EQ(history.entries()[0], "cat file.txt");
    EXPECT_EQ(history.entries()[1], "clear");
    EXPECT_EQ(history.entries()[2], "cat file.txt");
}

TEST(HistoryManagerTest, MaximumCapacityFifoShift) {
    HistoryManager history;
    for (size_t i = 0; i < 16; ++i) {
        EXPECT_TRUE(history.add("cmd_" + std::to_string(i)));
    }
    EXPECT_EQ(history.size(), 16U);
    EXPECT_EQ(history.entries().front(), "cmd_0");
    EXPECT_EQ(history.entries().back(), "cmd_15");

    EXPECT_TRUE(history.add("cmd_16"));
    EXPECT_EQ(history.size(), 16U);
    EXPECT_EQ(history.entries().front(), "cmd_1");
    EXPECT_EQ(history.entries().back(), "cmd_16");

    EXPECT_TRUE(history.add("cmd_17"));
    EXPECT_EQ(history.size(), 16U);
    EXPECT_EQ(history.entries().front(), "cmd_2");
    EXPECT_EQ(history.entries().back(), "cmd_17");
}

TEST(HistoryManagerTest, UpArrowNavigationSavesDraftAndClampsAtOldest) {
    HistoryManager history;
    history.add("first");
    history.add("second");
    history.add("third");

    EXPECT_FALSE(history.is_navigating());

    const auto nav1 = history.navigate_up("user_typing");
    ASSERT_TRUE(nav1.has_value());
    EXPECT_EQ(*nav1, "third");
    EXPECT_TRUE(history.is_navigating());
    EXPECT_EQ(history.draft(), "user_typing");

    const auto nav2 = history.navigate_up("user_typing");
    ASSERT_TRUE(nav2.has_value());
    EXPECT_EQ(*nav2, "second");

    const auto nav3 = history.navigate_up("user_typing");
    ASSERT_TRUE(nav3.has_value());
    EXPECT_EQ(*nav3, "first");

    const auto nav4 = history.navigate_up("user_typing");
    ASSERT_TRUE(nav4.has_value());
    EXPECT_EQ(*nav4, "first");

    const auto nav5 = history.navigate_up("user_typing");
    ASSERT_TRUE(nav5.has_value());
    EXPECT_EQ(*nav5, "first");
}

TEST(HistoryManagerTest, DownArrowNavigationRestoresNewerAndDraft) {
    HistoryManager history;
    history.add("cmd_a");
    history.add("cmd_b");
    history.add("cmd_c");

    static_cast<void>(history.navigate_up("current_draft"));
    static_cast<void>(history.navigate_up("current_draft"));
    static_cast<void>(history.navigate_up("current_draft"));

    const auto down1 = history.navigate_down();
    ASSERT_TRUE(down1.has_value());
    EXPECT_EQ(*down1, "cmd_b");
    EXPECT_TRUE(history.is_navigating());

    const auto down2 = history.navigate_down();
    ASSERT_TRUE(down2.has_value());
    EXPECT_EQ(*down2, "cmd_c");
    EXPECT_TRUE(history.is_navigating());

    const auto down3 = history.navigate_down();
    ASSERT_TRUE(down3.has_value());
    EXPECT_EQ(*down3, "current_draft");
    EXPECT_FALSE(history.is_navigating());

    const auto down4 = history.navigate_down();
    EXPECT_FALSE(down4.has_value());
}

TEST(HistoryManagerTest, DownArrowWithoutPriorNavigationDoesNothing) {
    HistoryManager history;
    history.add("cmd");
    EXPECT_FALSE(history.navigate_down().has_value());
}

TEST(HistoryManagerTest, EmptyHistoryNavigation) {
    HistoryManager history;
    EXPECT_FALSE(history.navigate_up("draft").has_value());
    EXPECT_FALSE(history.navigate_down().has_value());
}

TEST(HistoryManagerTest, SerializationFormatSeparatedByNewline) {
    HistoryManager history;
    history.add("echo hello");
    history.add("cat /SYSTEM/HIST.SYS");
    history.add("ls -l");

    const std::string serialized = history.serialize();
    EXPECT_EQ(serialized, "echo hello\ncat /SYSTEM/HIST.SYS\nls -l\n");
}

TEST(HistoryManagerTest, DeserializationFormat) {
    HistoryManager history;
    history.deserialize("echo hello\ncat /SYSTEM/HIST.SYS\nls -l\n");
    ASSERT_EQ(history.size(), 3U);
    EXPECT_EQ(history.entries()[0], "echo hello");
    EXPECT_EQ(history.entries()[1], "cat /SYSTEM/HIST.SYS");
    EXPECT_EQ(history.entries()[2], "ls -l");
}

TEST(HistoryManagerTest, DeserializationWithWindowsLineEndings) {
    HistoryManager history;
    history.deserialize("first\r\nsecond\r\nthird\r\n");
    ASSERT_EQ(history.size(), 3U);
    EXPECT_EQ(history.entries()[0], "first");
    EXPECT_EQ(history.entries()[1], "second");
    EXPECT_EQ(history.entries()[2], "third");
}

TEST(HistoryManagerTest, SerializationDeserializationRoundTrip) {
    HistoryManager original;
    original.add("sysinfo");
    original.add("cd /BIN");
    original.add("more README.TXT");

    HistoryManager restored;
    restored.deserialize(original.serialize());
    EXPECT_EQ(restored.entries(), original.entries());
}

TEST(HistoryManagerTest, DeserializationFifoCapacityLimit) {
    std::string text;
    for (size_t i = 0; i < 20; ++i) {
        text += "entry_" + std::to_string(i) + "\n";
    }

    HistoryManager history;
    history.deserialize(text);
    EXPECT_EQ(history.size(), 16U);
    EXPECT_EQ(history.entries().front(), "entry_4");
    EXPECT_EQ(history.entries().back(), "entry_19");
}

TEST(PathCompletionTest, SplitTokenEmpty) {
    const auto split = split_path("");
    EXPECT_EQ(split.dir, ".");
    EXPECT_EQ(split.prefix, "");
}

TEST(PathCompletionTest, SplitTokenSimpleName) {
    const auto split = split_path("foo");
    EXPECT_EQ(split.dir, ".");
    EXPECT_EQ(split.prefix, "foo");
}

TEST(PathCompletionTest, SplitTokenRelativePathWithDirectory) {
    const auto split = split_path("bin/ca");
    EXPECT_EQ(split.dir, "bin");
    EXPECT_EQ(split.prefix, "ca");
}

TEST(PathCompletionTest, SplitTokenAbsolutePath) {
    const auto split = split_path("/SYSTEM/HI");
    EXPECT_EQ(split.dir, "/SYSTEM");
    EXPECT_EQ(split.prefix, "HI");
}

TEST(PathCompletionTest, SplitTokenRootSlash) {
    const auto split = split_path("/");
    EXPECT_EQ(split.dir, "/");
    EXPECT_EQ(split.prefix, "");
}

TEST(PathCompletionTest, SplitTokenTrailingSlash) {
    const auto split = split_path("bin/");
    EXPECT_EQ(split.dir, "bin");
    EXPECT_EQ(split.prefix, "");
}

TEST(PathCompletionTest, CaseInsensitivePrefixMatchingFAT83) {
    EXPECT_TRUE(iequals_prefix("CAT.APP", "cat"));
    EXPECT_TRUE(iequals_prefix("CAT.APP", "CAT"));
    EXPECT_TRUE(iequals_prefix("CAT.APP", "CaT"));
    EXPECT_TRUE(iequals_prefix("CAT.APP", "c"));
    EXPECT_TRUE(iequals_prefix("CAT.APP", "CAT.APP"));
    EXPECT_TRUE(iequals_prefix("CAT.APP", ""));

    EXPECT_FALSE(iequals_prefix("CAT.APP", "dog"));
    EXPECT_FALSE(iequals_prefix("CAT.APP", "CAT.APPS"));
    EXPECT_FALSE(iequals_prefix("C", "CAT.APP"));

    EXPECT_TRUE(iequals_prefix("SYSTEM", "sys"));
    EXPECT_TRUE(iequals_prefix("README.TXT", "readme.txt"));
}

TEST(PathCompletionTest, SuffixGenerationDirectoryAddsTrailingSlash) {
    const DirEntry dir_entry{.name = "SYSTEM", .is_directory = true};
    const auto suffix1 = generate_suffix(dir_entry, "sys");
    ASSERT_TRUE(suffix1.has_value());
    EXPECT_EQ(*suffix1, "TEM/");

    const auto suffix2 = generate_suffix(dir_entry, "SYSTEM");
    ASSERT_TRUE(suffix2.has_value());
    EXPECT_EQ(*suffix2, "/");

    const auto suffix3 = generate_suffix(dir_entry, "");
    ASSERT_TRUE(suffix3.has_value());
    EXPECT_EQ(*suffix3, "SYSTEM/");
}

TEST(PathCompletionTest, SuffixGenerationFileAddsTrailingSpace) {
    const DirEntry file_entry{.name = "README.TXT", .is_directory = false};
    const auto suffix1 = generate_suffix(file_entry, "read");
    ASSERT_TRUE(suffix1.has_value());
    EXPECT_EQ(*suffix1, "ME.TXT ");

    const auto suffix2 = generate_suffix(file_entry, "README.TXT");
    ASSERT_TRUE(suffix2.has_value());
    EXPECT_EQ(*suffix2, " ");

    const auto suffix3 = generate_suffix(file_entry, "");
    ASSERT_TRUE(suffix3.has_value());
    EXPECT_EQ(*suffix3, "README.TXT ");
}

TEST(PathCompletionTest, SuffixCalculationCorrectnessCatApp) {
    const DirEntry cat_entry{.name = "CAT.APP", .is_directory = false};
    const auto suffix = generate_suffix(cat_entry, "cat");
    ASSERT_TRUE(suffix.has_value());
    EXPECT_EQ(*suffix, ".APP ");
}

TEST(PathCompletionTest, BufferOverflowPreventionNormalAppend) {
    std::array<char, SHELL_MAX_LINE> buffer{};
    const std::string_view initial = "cat ";
    std::memcpy(buffer.data(), initial.data(), initial.size());
    size_t current_len = initial.size();
    buffer[current_len] = '\0';

    const bool ok = append_completion_suffix(buffer, current_len, ".APP ");
    EXPECT_TRUE(ok);
    EXPECT_EQ(current_len, 9U);
    EXPECT_STREQ(buffer.data(), "cat .APP ");
    EXPECT_EQ(buffer[current_len], '\0');
}

TEST(PathCompletionTest, BufferOverflowPreventionTruncatesAtLimit) {
    std::array<char, 16> buffer{};
    buffer.fill('X');

    const std::string_view initial = "0123456789ab";
    std::memcpy(buffer.data(), initial.data(), initial.size());
    size_t current_len = initial.size();
    buffer[current_len] = '\0';

    const bool ok = append_completion_suffix(buffer, current_len, ".APP ");
    EXPECT_FALSE(ok);
    EXPECT_EQ(current_len, 15U);
    EXPECT_STREQ(buffer.data(), "0123456789ab.AP");
    EXPECT_EQ(buffer[15], '\0');
}

TEST(PathCompletionTest, BufferOverflowPreventionExactCapacity) {
    std::array<char, 16> buffer{};
    const std::string_view initial = "0123456789";
    std::memcpy(buffer.data(), initial.data(), initial.size());
    size_t current_len = initial.size();
    buffer[current_len] = '\0';

    const bool ok = append_completion_suffix(buffer, current_len, ".APP ");
    EXPECT_TRUE(ok);
    EXPECT_EQ(current_len, 15U);
    EXPECT_STREQ(buffer.data(), "0123456789.APP ");
    EXPECT_EQ(buffer[15], '\0');
}

TEST(PathCompletionTest, BufferOverflowPreventionFullBufferRejection) {
    std::array<char, 16> buffer{};
    const std::string_view initial = "0123456789abcde";
    std::memcpy(buffer.data(), initial.data(), initial.size());
    size_t current_len = initial.size();
    buffer[current_len] = '\0';

    const bool ok = append_completion_suffix(buffer, current_len, "extra");
    EXPECT_FALSE(ok);
    EXPECT_EQ(current_len, 15U);
    EXPECT_STREQ(buffer.data(), "0123456789abcde");
    EXPECT_EQ(buffer[15], '\0');
}

TEST(PathCompletionTest, EndToEndLineCompletionSimulation) {
    const std::vector<DirEntry> system_dir = {
        {.name = "DESKTOP.SYS", .is_directory = false},
        {.name = "HIST.SYS", .is_directory = false},
        {.name = "DRIVERS", .is_directory = true}
    };

    std::array<char, SHELL_MAX_LINE> line_buffer{};
    const std::string_view line_input = "cat /SYSTEM/HI";
    std::memcpy(line_buffer.data(), line_input.data(), line_input.size());
    size_t current_len = line_input.size();
    line_buffer[current_len] = '\0';

    const size_t token_start = line_input.rfind(' ') + 1;
    const std::string_view token = line_input.substr(token_start);
    const auto split = split_path(token);

    EXPECT_EQ(split.dir, "/SYSTEM");
    EXPECT_EQ(split.prefix, "HI");

    std::optional<DirEntry> matched;
    for (const auto& entry : system_dir) {
        if (iequals_prefix(entry.name, split.prefix)) {
            matched = entry;
            break;
        }
    }

    ASSERT_TRUE(matched.has_value());
    EXPECT_EQ(matched->name, "HIST.SYS");

    const auto suffix = generate_suffix(*matched, split.prefix);
    ASSERT_TRUE(suffix.has_value());
    EXPECT_EQ(*suffix, "ST.SYS ");

    const bool ok = append_completion_suffix(line_buffer, current_len, *suffix);
    EXPECT_TRUE(ok);
    EXPECT_STREQ(line_buffer.data(), "cat /SYSTEM/HIST.SYS ");
}
