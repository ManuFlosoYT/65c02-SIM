#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

[[nodiscard]] constexpr std::string_view trim(std::string_view sv) noexcept {
    const auto start = sv.find_first_not_of(" \t\r\n");
    if (start == std::string_view::npos) {
        return {};
    }
    const auto end = sv.find_last_not_of(" \t\r\n");
    return sv.substr(start, end - start + 1);
}

[[nodiscard]] constexpr char to_ascii_lower(char c) noexcept {
    if (c >= 'A' && c <= 'Z') {
        return static_cast<char>(c + ('a' - 'A'));
    }
    return c;
}

[[nodiscard]] constexpr bool iequals(std::string_view a, std::string_view b) noexcept {
    return std::ranges::equal(a, b, [](char c1, char c2) {
        return to_ascii_lower(c1) == to_ascii_lower(c2);
    });
}

[[nodiscard]] constexpr bool istarts_with(std::string_view sv, std::string_view prefix) noexcept {
    if (sv.size() < prefix.size()) {
        return false;
    }
    return iequals(sv.substr(0, prefix.size()), prefix);
}

[[nodiscard]] constexpr bool is_batch_file(std::string_view filename) noexcept {
    const auto last_slash = filename.find_last_of("/\\");
    const auto base_name = (last_slash == std::string_view::npos) ? filename : filename.substr(last_slash + 1);
    const auto dot_pos = base_name.rfind('.');
    if (dot_pos == std::string_view::npos || dot_pos == 0) {
        return false;
    }
    const auto ext = base_name.substr(dot_pos);
    return iequals(ext, ".bat") || iequals(ext, ".cmd");
}

[[nodiscard]] constexpr bool is_comment_line(std::string_view trimmed_line) noexcept {
    if (trimmed_line.empty()) {
        return false;
    }
    if (trimmed_line.front() == '#' || trimmed_line.front() == ';') {
        return true;
    }
    if (trimmed_line.size() >= 3 && istarts_with(trimmed_line, "rem")) {
        if (trimmed_line.size() == 3) {
            return true;
        }
        const char next_char = trimmed_line[3];
        return next_char == ' ' || next_char == '\t' || next_char == '\r' || next_char == '\n';
    }
    return false;
}

[[nodiscard]] std::vector<std::string> parse_batch_script(std::string_view script) {
    std::vector<std::string> executable_lines;
    size_t pos = 0;
    while (pos < script.size()) {
        const auto newline_pos = script.find('\n', pos);
        const auto line = (newline_pos == std::string_view::npos)
            ? script.substr(pos)
            : script.substr(pos, newline_pos - pos);
        pos = (newline_pos == std::string_view::npos) ? script.size() : newline_pos + 1;

        const auto trimmed = trim(line);
        if (trimmed.empty() || is_comment_line(trimmed)) {
            continue;
        }
        executable_lines.emplace_back(trimmed);
    }
    return executable_lines;
}

class BatchQueue {
public:
    constexpr BatchQueue() = default;

    void enqueue_script(std::string_view script) {
        auto lines = parse_batch_script(script);
        for (auto& line : lines) {
            lines_.push_back(std::move(line));
        }
    }

    void enqueue_line(std::string_view line) {
        const auto trimmed = trim(line);
        if (!trimmed.empty() && !is_comment_line(trimmed)) {
            lines_.emplace_back(trimmed);
        }
    }

    [[nodiscard]] bool empty() const noexcept {
        return lines_executed_ >= lines_.size();
    }

    [[nodiscard]] size_t size() const noexcept {
        return lines_.size() - lines_executed_;
    }

    [[nodiscard]] size_t total_enqueued() const noexcept {
        return lines_.size();
    }

    [[nodiscard]] bool is_active() const noexcept {
        return !empty();
    }

    [[nodiscard]] std::optional<std::string> pop_next() {
        if (empty()) {
            return std::nullopt;
        }
        return lines_[lines_executed_++];
    }

    [[nodiscard]] std::span<const std::string> remaining_lines() const noexcept {
        if (empty()) {
            return {};
        }
        return std::span<const std::string>(lines_.data() + lines_executed_, lines_.size() - lines_executed_);
    }

    void clear() noexcept {
        lines_.clear();
        lines_executed_ = 0;
    }

private:
    std::vector<std::string> lines_;
    size_t lines_executed_{0};
};

enum class FileType {
    Regular,
    Directory,
    Special
};

struct DirEntry {
    std::string name;
    FileType type{FileType::Regular};
    bool is_hidden{false};

    auto operator<=>(const DirEntry&) const = default;
};

class StartupManager {
public:
    constexpr StartupManager() = default;

    [[nodiscard]] constexpr uint8_t os_startup_done() const noexcept {
        return os_startup_done_;
    }

    [[nodiscard]] std::vector<std::string> scan_startup_scripts(std::span<const DirEntry> entries) {
        if (os_startup_done_ == 0) {
            os_startup_done_ = 1;
        }

        std::vector<std::string> matched_scripts;
        for (const auto& entry : entries) {
            if (entry.type != FileType::Regular) {
                continue;
            }
            if (entry.is_hidden || entry.name.starts_with('.')) {
                continue;
            }
            if (is_batch_file(entry.name)) {
                matched_scripts.push_back(entry.name);
            }
        }
        return matched_scripts;
    }

    void on_shell_iteration() noexcept {
        if (os_startup_done_ == 0) {
            os_startup_done_ = 1;
        }
    }

    void reset() noexcept {
        os_startup_done_ = 0;
    }

private:
    uint8_t os_startup_done_{0};
};

enum class InvocationType {
    None,
    BatchCommand,
    DirectBatchFile,
    StandardCommand
};

struct CommandInvocation {
    InvocationType type{InvocationType::None};
    std::string target_file;
    std::vector<std::string> arguments;

    auto operator<=>(const CommandInvocation&) const = default;
};

[[nodiscard]] std::vector<std::string> tokenize_arguments(std::string_view args_sv) {
    std::vector<std::string> tokens;
    size_t i = 0;
    while (i < args_sv.size()) {
        while (i < args_sv.size() && (args_sv[i] == ' ' || args_sv[i] == '\t')) {
            ++i;
        }
        if (i >= args_sv.size()) {
            break;
        }
        size_t start = i;
        while (i < args_sv.size() && args_sv[i] != ' ' && args_sv[i] != '\t') {
            ++i;
        }
        tokens.emplace_back(args_sv.substr(start, i - start));
    }
    return tokens;
}

[[nodiscard]] CommandInvocation resolve_invocation(std::string_view command_line) {
    const auto trimmed = trim(command_line);
    if (trimmed.empty()) {
        return {.type = InvocationType::None};
    }

    const auto first_space = trimmed.find_first_of(" \t");
    const auto cmd_name = (first_space == std::string_view::npos)
        ? trimmed
        : trimmed.substr(0, first_space);
    const auto args_part = (first_space == std::string_view::npos)
        ? std::string_view{}
        : trim(trimmed.substr(first_space + 1));

    if (iequals(cmd_name, "batch")) {
        if (args_part.empty()) {
            return {.type = InvocationType::BatchCommand, .target_file = {}, .arguments = {}};
        }
        const auto next_space = args_part.find_first_of(" \t");
        const auto target = (next_space == std::string_view::npos)
            ? args_part
            : args_part.substr(0, next_space);
        const auto rest_args = (next_space == std::string_view::npos)
            ? std::string_view{}
            : trim(args_part.substr(next_space + 1));
        return {
            .type = InvocationType::BatchCommand,
            .target_file = std::string(target),
            .arguments = tokenize_arguments(rest_args)
        };
    }

    if (is_batch_file(cmd_name)) {
        return {
            .type = InvocationType::DirectBatchFile,
            .target_file = std::string(cmd_name),
            .arguments = tokenize_arguments(args_part)
        };
    }

    return {
        .type = InvocationType::StandardCommand,
        .target_file = std::string(cmd_name),
        .arguments = tokenize_arguments(args_part)
    };
}

}  // namespace

TEST(BatchParserTest, TrimsLeadingAndTrailingWhitespace) {
    EXPECT_EQ(trim("   echo hello \t \r\n"), "echo hello");
    EXPECT_EQ(trim("\t\r\n  cls  \t\n"), "cls");
    EXPECT_EQ(trim("echo   spaced   args"), "echo   spaced   args");

    const auto lines = parse_batch_script("   echo 1   \n\t  dir /w \r\n  cls  ");
    ASSERT_EQ(lines.size(), 3U);
    EXPECT_EQ(lines[0], "echo 1");
    EXPECT_EQ(lines[1], "dir /w");
    EXPECT_EQ(lines[2], "cls");
}

TEST(BatchParserTest, IgnoresEmptyLines) {
    const auto empty_script = parse_batch_script("\n\n\r\n   \t  \n");
    EXPECT_TRUE(empty_script.empty());

    const auto mixed_script = parse_batch_script("dir\n\n\ncls\n \t \nver\n");
    ASSERT_EQ(mixed_script.size(), 3U);
    EXPECT_EQ(mixed_script[0], "dir");
    EXPECT_EQ(mixed_script[1], "cls");
    EXPECT_EQ(mixed_script[2], "ver");
}

TEST(BatchParserTest, IgnoresCommentLinesHashAndSemicolon) {
    const std::string script =
        "# Top level comment\n"
        "echo active1\n"
        "  # Indented hash comment\n"
        "; Assembly style comment\n"
        " \t ; Indented semicolon comment\n"
        "echo active2\n";

    const auto lines = parse_batch_script(script);
    ASSERT_EQ(lines.size(), 2U);
    EXPECT_EQ(lines[0], "echo active1");
    EXPECT_EQ(lines[1], "echo active2");

    const auto non_comment_script = parse_batch_script("echo arg#1;arg2\n");
    ASSERT_EQ(non_comment_script.size(), 1U);
    EXPECT_EQ(non_comment_script[0], "echo arg#1;arg2");
}

TEST(BatchParserTest, IgnoresCommentLinesRemCaseInsensitive) {
    const std::string script =
        "REM uppercase comment\n"
        "rem lowercase comment\n"
        "Rem MixedCase comment\n"
        "rEm mixed case\n"
        "   REM indented comment\n"
        "REM\n"
        "rem\n"
        "echo executed\n"
        "remember_me\n"
        "remove file.txt\n";

    const auto lines = parse_batch_script(script);
    ASSERT_EQ(lines.size(), 3U);
    EXPECT_EQ(lines[0], "echo executed");
    EXPECT_EQ(lines[1], "remember_me");
    EXPECT_EQ(lines[2], "remove file.txt");
}

TEST(BatchParserTest, PreservesCommandLineWithArgumentsAndFlags) {
    const std::string script =
        "copy file1.txt file2.txt /y /v\n"
        "ls -la /usr/bin\n"
        "format c: /q\n";

    const auto lines = parse_batch_script(script);
    ASSERT_EQ(lines.size(), 3U);
    EXPECT_EQ(lines[0], "copy file1.txt file2.txt /y /v");
    EXPECT_EQ(lines[1], "ls -la /usr/bin");
    EXPECT_EQ(lines[2], "format c: /q");
}

TEST(BatchParserTest, PreservesPipesAndRedirections) {
    const std::string script =
        "cat input.txt | grep error | wc -l > output.txt\n"
        "echo status: ready >> /var/log/boot.log\n"
        "type data.csv | sort >> log.txt\n";

    const auto lines = parse_batch_script(script);
    ASSERT_EQ(lines.size(), 3U);
    EXPECT_EQ(lines[0], "cat input.txt | grep error | wc -l > output.txt");
    EXPECT_EQ(lines[1], "echo status: ready >> /var/log/boot.log");
    EXPECT_EQ(lines[2], "type data.csv | sort >> log.txt");
}

TEST(BatchParserTest, DetectsBatchExtensionsCaseInsensitive) {
    EXPECT_TRUE(is_batch_file("setup.bat"));
    EXPECT_TRUE(is_batch_file("SETUP.BAT"));
    EXPECT_TRUE(is_batch_file("Install.Bat"));
    EXPECT_TRUE(is_batch_file("build.cmd"));
    EXPECT_TRUE(is_batch_file("BUILD.CMD"));
    EXPECT_TRUE(is_batch_file("Deploy.Cmd"));
    EXPECT_TRUE(is_batch_file("/scripts/run.bat"));
    EXPECT_TRUE(is_batch_file("C:\\tools\\init.cmd"));

    EXPECT_FALSE(is_batch_file("setup.exe"));
    EXPECT_FALSE(is_batch_file("script.sh"));
    EXPECT_FALSE(is_batch_file("batch"));
    EXPECT_FALSE(is_batch_file(".bat"));
    EXPECT_FALSE(is_batch_file(".cmd"));
    EXPECT_FALSE(is_batch_file("archive.bat.tar"));
    EXPECT_FALSE(is_batch_file("file.txt"));
}

TEST(BatchQueueTest, FifoPoppingExtractsLineAndYieldsRemaining) {
    BatchQueue queue;
    queue.enqueue_script(
        "echo step1\n"
        "run setup.bin\n"
        "echo step3\n"
    );

    EXPECT_EQ(queue.size(), 3U);
    EXPECT_EQ(queue.remaining_lines().size(), 3U);

    const auto pop1 = queue.pop_next();
    ASSERT_TRUE(pop1.has_value());
    EXPECT_EQ(*pop1, "echo step1");
    EXPECT_EQ(queue.size(), 2U);
    ASSERT_EQ(queue.remaining_lines().size(), 2U);
    EXPECT_EQ(queue.remaining_lines()[0], "run setup.bin");
    EXPECT_EQ(queue.remaining_lines()[1], "echo step3");

    const auto pop2 = queue.pop_next();
    ASSERT_TRUE(pop2.has_value());
    EXPECT_EQ(*pop2, "run setup.bin");
    EXPECT_EQ(queue.size(), 1U);
    ASSERT_EQ(queue.remaining_lines().size(), 1U);
    EXPECT_EQ(queue.remaining_lines()[0], "echo step3");

    const auto pop3 = queue.pop_next();
    ASSERT_TRUE(pop3.has_value());
    EXPECT_EQ(*pop3, "echo step3");
    EXPECT_EQ(queue.size(), 0U);
    EXPECT_TRUE(queue.remaining_lines().empty());
}

TEST(BatchQueueTest, StatePersistenceAcrossPopsUntilEmpty) {
    BatchQueue queue;
    queue.enqueue_script(
        "cmd_a\n"
        "cmd_b\n"
        "cmd_c\n"
    );

    EXPECT_TRUE(queue.is_active());
    EXPECT_FALSE(queue.empty());

    std::vector<std::string> popped;
    while (!queue.empty()) {
        EXPECT_TRUE(queue.is_active());
        auto line = queue.pop_next();
        ASSERT_TRUE(line.has_value());
        popped.push_back(std::move(*line));
    }

    EXPECT_EQ(popped, (std::vector<std::string>{"cmd_a", "cmd_b", "cmd_c"}));
    EXPECT_FALSE(queue.is_active());
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0U);
}

TEST(BatchQueueTest, DeletionAndCompletionStateWhenLastLinePopped) {
    BatchQueue queue;
    queue.enqueue_script("single_command\n");

    EXPECT_TRUE(queue.is_active());
    EXPECT_EQ(queue.size(), 1U);

    const auto pop = queue.pop_next();
    ASSERT_TRUE(pop.has_value());
    EXPECT_EQ(*pop, "single_command");

    EXPECT_FALSE(queue.is_active());
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0U);
    EXPECT_TRUE(queue.remaining_lines().empty());

    EXPECT_FALSE(queue.pop_next().has_value());

    queue.enqueue_line("new_cmd");
    EXPECT_TRUE(queue.is_active());
    queue.clear();
    EXPECT_FALSE(queue.is_active());
    EXPECT_TRUE(queue.empty());
}

TEST(BatchQueueTest, MultiFileConcatenationSequentialExecution) {
    BatchQueue queue;
    queue.enqueue_script(
        "script1_line1\n"
        "script1_line2\n"
    );
    queue.enqueue_script(
        "script2_line1\n"
        "script2_line2\n"
    );

    EXPECT_EQ(queue.size(), 4U);

    EXPECT_EQ(queue.pop_next(), "script1_line1");
    EXPECT_EQ(queue.pop_next(), "script1_line2");
    EXPECT_EQ(queue.pop_next(), "script2_line1");
    EXPECT_EQ(queue.pop_next(), "script2_line2");
    EXPECT_TRUE(queue.empty());

    queue.enqueue_script("part1_a\npart1_b\n");
    EXPECT_EQ(queue.pop_next(), "part1_a");

    queue.enqueue_script("part2_a\n");
    EXPECT_EQ(queue.size(), 2U);
    EXPECT_EQ(queue.pop_next(), "part1_b");
    EXPECT_EQ(queue.pop_next(), "part2_a");
    EXPECT_TRUE(queue.empty());
}

TEST(StartupLogicTest, BootLifecycleOsStartupDoneTransitions) {
    StartupManager startup;
    EXPECT_EQ(startup.os_startup_done(), 0U);

    const std::vector<DirEntry> entries = {
        {"autoexec.bat", FileType::Regular, false}
    };

    const auto matched = startup.scan_startup_scripts(entries);
    EXPECT_EQ(startup.os_startup_done(), 1U);
    ASSERT_EQ(matched.size(), 1U);
    EXPECT_EQ(matched[0], "autoexec.bat");

    startup.on_shell_iteration();
    EXPECT_EQ(startup.os_startup_done(), 1U);

    startup.on_shell_iteration();
    EXPECT_EQ(startup.os_startup_done(), 1U);

    const auto next_matched = startup.scan_startup_scripts(entries);
    EXPECT_EQ(startup.os_startup_done(), 1U);
    EXPECT_EQ(next_matched.size(), 1U);

    startup.reset();
    EXPECT_EQ(startup.os_startup_done(), 0U);
}

TEST(StartupLogicTest, DirectoryScanningFiltersRegularNonHiddenCriteria) {
    StartupManager startup;

    const std::vector<DirEntry> entries = {
        {"AUTOEXEC.BAT", FileType::Regular, false},
        {"config.cmd", FileType::Regular, false},
        {".hidden.bat", FileType::Regular, true},
        {".startup.cmd", FileType::Regular, false},
        {"system.sys", FileType::Regular, false},
        {"readme.txt", FileType::Regular, false},
        {"subfolder.bat", FileType::Directory, false},
        {"device.cmd", FileType::Special, false},
        {"INSTALL.BAT", FileType::Regular, false}
    };

    const auto matched = startup.scan_startup_scripts(entries);
    ASSERT_EQ(matched.size(), 3U);
    EXPECT_EQ(matched[0], "AUTOEXEC.BAT");
    EXPECT_EQ(matched[1], "config.cmd");
    EXPECT_EQ(matched[2], "INSTALL.BAT");
}

TEST(StartupLogicTest, InvocationWithBatchCommandVsDirect) {
    const auto inv1 = resolve_invocation("batch autoexec.bat");
    EXPECT_EQ(inv1.type, InvocationType::BatchCommand);
    EXPECT_EQ(inv1.target_file, "autoexec.bat");
    EXPECT_TRUE(inv1.arguments.empty());

    const auto inv2 = resolve_invocation("BATCH run.cmd arg1 /v");
    EXPECT_EQ(inv2.type, InvocationType::BatchCommand);
    EXPECT_EQ(inv2.target_file, "run.cmd");
    ASSERT_EQ(inv2.arguments.size(), 2U);
    EXPECT_EQ(inv2.arguments[0], "arg1");
    EXPECT_EQ(inv2.arguments[1], "/v");

    const auto inv3 = resolve_invocation("batch");
    EXPECT_EQ(inv3.type, InvocationType::BatchCommand);
    EXPECT_TRUE(inv3.target_file.empty());
    EXPECT_TRUE(inv3.arguments.empty());

    const auto inv4 = resolve_invocation("autoexec.bat");
    EXPECT_EQ(inv4.type, InvocationType::DirectBatchFile);
    EXPECT_EQ(inv4.target_file, "autoexec.bat");
    EXPECT_TRUE(inv4.arguments.empty());

    const auto inv5 = resolve_invocation("SETUP.CMD --quiet --force");
    EXPECT_EQ(inv5.type, InvocationType::DirectBatchFile);
    EXPECT_EQ(inv5.target_file, "SETUP.CMD");
    ASSERT_EQ(inv5.arguments.size(), 2U);
    EXPECT_EQ(inv5.arguments[0], "--quiet");
    EXPECT_EQ(inv5.arguments[1], "--force");

    const auto inv6 = resolve_invocation("cat autoexec.bat | grep echo");
    EXPECT_EQ(inv6.type, InvocationType::StandardCommand);
    EXPECT_EQ(inv6.target_file, "cat");

    const auto inv7 = resolve_invocation("   ");
    EXPECT_EQ(inv7.type, InvocationType::None);
}
