#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr size_t MAX_PIPELINE_STAGES = 4;

struct Redirection {
    std::string filename;
    bool append{false};

    auto operator<=>(const Redirection&) const = default;
};

struct CommandStage {
    std::vector<std::string> args;

    auto operator<=>(const CommandStage&) const = default;
};

enum class ParseError {
    None,
    EmptyCommand,
    LeadingPipe,
    TrailingPipe,
    ConsecutivePipes,
    ExceededMaxStages,
    MissingRedirectionFile,
    MultipleRedirections,
    RedirectionNotAtEnd,
    EmptyStage,
    UnclosedQuote
};

struct PipelineParseResult {
    std::vector<CommandStage> stages;
    std::optional<Redirection> redirection;
    ParseError error{ParseError::None};

    [[nodiscard]] constexpr bool has_error() const noexcept { return error != ParseError::None; }

    [[nodiscard]] constexpr bool is_success() const noexcept { return error == ParseError::None; }

    [[nodiscard]] std::string_view redir_out() const noexcept {
        return redirection ? std::string_view{redirection->filename} : std::string_view{};
    }

    [[nodiscard]] bool append() const noexcept { return redirection ? redirection->append : false; }
};

[[nodiscard]] constexpr std::string_view trim(std::string_view sv) noexcept {
    const auto start = sv.find_first_not_of(" \t\r\n");
    if (start == std::string_view::npos) {
        return {};
    }
    const auto end = sv.find_last_not_of(" \t\r\n");
    return sv.substr(start, end - start + 1);
}

struct TokenizeResult {
    std::vector<std::string> tokens;
    bool has_unclosed_quote{false};
};

[[nodiscard]] TokenizeResult tokenize_args(std::string_view sv) {
    TokenizeResult result;
    std::string current;
    bool in_single_quote = false;
    bool in_double_quote = false;
    bool token_active = false;

    for (const char c : sv) {
        if (c == '\'' && !in_double_quote) {
            in_single_quote = !in_single_quote;
            token_active = true;
        } else if (c == '"' && !in_single_quote) {
            in_double_quote = !in_double_quote;
            token_active = true;
        } else if ((c == ' ' || c == '\t') && !in_single_quote && !in_double_quote) {
            if (token_active) {
                result.tokens.push_back(std::move(current));
                current.clear();
                token_active = false;
            }
        } else {
            current.push_back(c);
            token_active = true;
        }
    }

    if (in_single_quote || in_double_quote) {
        result.has_unclosed_quote = true;
    }

    if (token_active) {
        result.tokens.push_back(std::move(current));
    }

    return result;
}

[[nodiscard]] PipelineParseResult parse_pipeline(std::string_view command_line) {
    const auto trimmed_input = trim(command_line);
    if (trimmed_input.empty()) {
        return {.error = ParseError::EmptyCommand};
    }

    std::vector<size_t> pipe_indices;
    bool in_single = false;
    bool in_double = false;

    for (size_t i = 0; i < command_line.size(); ++i) {
        const char c = command_line[i];
        if (c == '\'' && !in_double) {
            in_single = !in_single;
        } else if (c == '"' && !in_single) {
            in_double = !in_double;
        } else if (c == '|' && !in_single && !in_double) {
            pipe_indices.push_back(i);
        }
    }

    if (in_single || in_double) {
        return {.error = ParseError::UnclosedQuote};
    }

    if (!pipe_indices.empty()) {
        const size_t first_non_ws = command_line.find_first_not_of(" \t\r\n");
        if (first_non_ws == pipe_indices.front()) {
            return {.error = ParseError::LeadingPipe};
        }

        const size_t last_non_ws = command_line.find_last_not_of(" \t\r\n");
        if (last_non_ws == pipe_indices.back()) {
            return {.error = ParseError::TrailingPipe};
        }
    }

    std::vector<std::string_view> raw_stages;
    size_t prev_pos = 0;
    for (const size_t pipe_idx : pipe_indices) {
        raw_stages.push_back(command_line.substr(prev_pos, pipe_idx - prev_pos));
        prev_pos = pipe_idx + 1;
    }
    raw_stages.push_back(command_line.substr(prev_pos));

    if (raw_stages.size() > MAX_PIPELINE_STAGES) {
        return {.error = ParseError::ExceededMaxStages};
    }

    PipelineParseResult result;

    for (size_t stage_idx = 0; stage_idx < raw_stages.size(); ++stage_idx) {
        const auto stage_sv = trim(raw_stages[stage_idx]);
        if (stage_sv.empty()) {
            return {.error = ParseError::ConsecutivePipes};
        }

        std::vector<size_t> redir_indices;
        in_single = false;
        in_double = false;
        for (size_t i = 0; i < stage_sv.size(); ++i) {
            const char c = stage_sv[i];
            if (c == '\'' && !in_double) {
                in_single = !in_single;
            } else if (c == '"' && !in_single) {
                in_double = !in_double;
            } else if (c == '>' && !in_single && !in_double) {
                redir_indices.push_back(i);
            }
        }

        const bool is_last_stage = (stage_idx + 1 == raw_stages.size());

        if (!redir_indices.empty()) {
            if (!is_last_stage) {
                return {.error = ParseError::RedirectionNotAtEnd};
            }

            const bool is_double_chevron = (redir_indices.size() == 2 && redir_indices[1] == redir_indices[0] + 1);
            if (redir_indices.size() > 2 || (redir_indices.size() == 2 && !is_double_chevron)) {
                return {.error = ParseError::MultipleRedirections};
            }

            const size_t op_pos = redir_indices[0];
            const size_t op_len = is_double_chevron ? 2 : 1;

            const auto cmd_sv = trim(stage_sv.substr(0, op_pos));
            const auto target_sv = trim(stage_sv.substr(op_pos + op_len));

            if (target_sv.empty()) {
                return {.error = ParseError::MissingRedirectionFile};
            }

            auto target_result = tokenize_args(target_sv);
            if (target_result.has_unclosed_quote) {
                return {.error = ParseError::UnclosedQuote};
            }
            if (target_result.tokens.empty()) {
                return {.error = ParseError::MissingRedirectionFile};
            }

            result.redirection =
                Redirection{.filename = std::move(target_result.tokens[0]), .append = is_double_chevron};

            auto cmd_result = tokenize_args(cmd_sv);
            if (cmd_result.has_unclosed_quote) {
                return {.error = ParseError::UnclosedQuote};
            }

            auto stage_args = std::move(cmd_result.tokens);
            for (size_t t = 1; t < target_result.tokens.size(); ++t) {
                stage_args.push_back(std::move(target_result.tokens[t]));
            }

            if (stage_args.empty()) {
                return {.error = ParseError::EmptyStage};
            }

            result.stages.push_back(CommandStage{.args = std::move(stage_args)});
        } else {
            auto stage_result = tokenize_args(stage_sv);
            if (stage_result.has_unclosed_quote) {
                return {.error = ParseError::UnclosedQuote};
            }
            if (stage_result.tokens.empty()) {
                return {.error = ParseError::EmptyStage};
            }

            result.stages.push_back(CommandStage{.args = std::move(stage_result.tokens)});
        }
    }

    return result;
}

}  // namespace

TEST(ShellPipelineTest, SingleCommandNoRedirection) {
    const auto result = parse_pipeline("cat file.txt");
    EXPECT_FALSE(result.has_error());
    EXPECT_TRUE(result.is_success());
    ASSERT_EQ(result.stages.size(), 1U);
    EXPECT_EQ(result.stages[0].args, (std::vector<std::string>{"cat", "file.txt"}));
    EXPECT_FALSE(result.redirection.has_value());
    EXPECT_TRUE(result.redir_out().empty());
}

TEST(ShellPipelineTest, SingleCommandOutputRedirection) {
    const auto result = parse_pipeline("cat file.txt > out.txt");
    EXPECT_FALSE(result.has_error());
    EXPECT_TRUE(result.is_success());
    ASSERT_EQ(result.stages.size(), 1U);
    EXPECT_EQ(result.stages[0].args, (std::vector<std::string>{"cat", "file.txt"}));
    ASSERT_TRUE(result.redirection.has_value());
    EXPECT_EQ(result.redirection->filename, "out.txt");
    EXPECT_FALSE(result.redirection->append);
    EXPECT_EQ(result.redir_out(), "out.txt");
    EXPECT_FALSE(result.append());
}

TEST(ShellPipelineTest, SingleCommandAppendRedirection) {
    const auto result = parse_pipeline("echo hello >> log.txt");
    EXPECT_FALSE(result.has_error());
    EXPECT_TRUE(result.is_success());
    ASSERT_EQ(result.stages.size(), 1U);
    EXPECT_EQ(result.stages[0].args, (std::vector<std::string>{"echo", "hello"}));
    ASSERT_TRUE(result.redirection.has_value());
    EXPECT_EQ(result.redirection->filename, "log.txt");
    EXPECT_TRUE(result.redirection->append);
    EXPECT_EQ(result.redir_out(), "log.txt");
    EXPECT_TRUE(result.append());
}

TEST(ShellPipelineTest, TwoStagePipeline) {
    const auto result = parse_pipeline("cat file.txt | grep foo");
    EXPECT_FALSE(result.has_error());
    EXPECT_TRUE(result.is_success());
    ASSERT_EQ(result.stages.size(), 2U);
    EXPECT_EQ(result.stages[0].args, (std::vector<std::string>{"cat", "file.txt"}));
    EXPECT_EQ(result.stages[1].args, (std::vector<std::string>{"grep", "foo"}));
    EXPECT_FALSE(result.redirection.has_value());
}

TEST(ShellPipelineTest, ThreeStagePipelineWithRedirection) {
    const auto result = parse_pipeline("cat file.txt | grep error | more > result.txt");
    EXPECT_FALSE(result.has_error());
    EXPECT_TRUE(result.is_success());
    ASSERT_EQ(result.stages.size(), 3U);
    EXPECT_EQ(result.stages[0].args, (std::vector<std::string>{"cat", "file.txt"}));
    EXPECT_EQ(result.stages[1].args, (std::vector<std::string>{"grep", "error"}));
    EXPECT_EQ(result.stages[2].args, (std::vector<std::string>{"more"}));
    ASSERT_TRUE(result.redirection.has_value());
    EXPECT_EQ(result.redirection->filename, "result.txt");
    EXPECT_FALSE(result.redirection->append);
    EXPECT_EQ(result.redir_out(), "result.txt");
    EXPECT_FALSE(result.append());
}

TEST(ShellPipelineTest, WhitespaceResilience) {
    {
        const auto result = parse_pipeline("cat   file.txt|grep   foo>out.txt");
        EXPECT_FALSE(result.has_error());
        EXPECT_TRUE(result.is_success());
        ASSERT_EQ(result.stages.size(), 2U);
        EXPECT_EQ(result.stages[0].args, (std::vector<std::string>{"cat", "file.txt"}));
        EXPECT_EQ(result.stages[1].args, (std::vector<std::string>{"grep", "foo"}));
        ASSERT_TRUE(result.redirection.has_value());
        EXPECT_EQ(result.redirection->filename, "out.txt");
        EXPECT_FALSE(result.redirection->append);
    }
    {
        const auto result = parse_pipeline("   cat  file.txt   |   grep  foo  >>  out.txt   ");
        EXPECT_FALSE(result.has_error());
        EXPECT_TRUE(result.is_success());
        ASSERT_EQ(result.stages.size(), 2U);
        EXPECT_EQ(result.stages[0].args, (std::vector<std::string>{"cat", "file.txt"}));
        EXPECT_EQ(result.stages[1].args, (std::vector<std::string>{"grep", "foo"}));
        ASSERT_TRUE(result.redirection.has_value());
        EXPECT_EQ(result.redirection->filename, "out.txt");
        EXPECT_TRUE(result.redirection->append);
    }
}

TEST(ShellPipelineTest, SyntaxErrorDetection) {
    EXPECT_TRUE(parse_pipeline("cat |").has_error());
    EXPECT_EQ(parse_pipeline("cat |").error, ParseError::TrailingPipe);

    EXPECT_TRUE(parse_pipeline("cat || grep").has_error());
    EXPECT_EQ(parse_pipeline("cat || grep").error, ParseError::ConsecutivePipes);

    EXPECT_TRUE(parse_pipeline("cat >").has_error());
    EXPECT_EQ(parse_pipeline("cat >").error, ParseError::MissingRedirectionFile);

    EXPECT_TRUE(parse_pipeline("| grep").has_error());
    EXPECT_EQ(parse_pipeline("| grep").error, ParseError::LeadingPipe);

    EXPECT_TRUE(parse_pipeline("cat >>").has_error());
    EXPECT_EQ(parse_pipeline("cat >>").error, ParseError::MissingRedirectionFile);

    EXPECT_TRUE(parse_pipeline("cat > | grep").has_error());
    EXPECT_EQ(parse_pipeline("cat > | grep").error, ParseError::RedirectionNotAtEnd);

    EXPECT_TRUE(parse_pipeline("cat | | grep").has_error());
    EXPECT_EQ(parse_pipeline("cat | | grep").error, ParseError::ConsecutivePipes);

    EXPECT_TRUE(parse_pipeline("a | b | c | d | e").has_error());
    EXPECT_EQ(parse_pipeline("a | b | c | d | e").error, ParseError::ExceededMaxStages);

    EXPECT_TRUE(parse_pipeline("cat > f1.txt > f2.txt").has_error());
    EXPECT_EQ(parse_pipeline("cat > f1.txt > f2.txt").error, ParseError::MultipleRedirections);

    EXPECT_TRUE(parse_pipeline("").has_error());
    EXPECT_EQ(parse_pipeline("").error, ParseError::EmptyCommand);

    EXPECT_TRUE(parse_pipeline("   ").has_error());
    EXPECT_EQ(parse_pipeline("   ").error, ParseError::EmptyCommand);
}

TEST(ShellPipelineTest, FourStagePipelineBoundary) {
    const auto four_stages = parse_pipeline("cmd1 | cmd2 | cmd3 | cmd4");
    EXPECT_TRUE(four_stages.is_success());
    EXPECT_EQ(four_stages.stages.size(), 4U);

    const auto four_stages_redir = parse_pipeline("cmd1 | cmd2 | cmd3 | cmd4 >> out.log");
    EXPECT_TRUE(four_stages_redir.is_success());
    EXPECT_EQ(four_stages_redir.stages.size(), 4U);
    ASSERT_TRUE(four_stages_redir.redirection.has_value());
    EXPECT_EQ(four_stages_redir.redirection->filename, "out.log");
    EXPECT_TRUE(four_stages_redir.redirection->append);

    const auto five_stages = parse_pipeline("cmd1 | cmd2 | cmd3 | cmd4 | cmd5");
    EXPECT_TRUE(five_stages.has_error());
    EXPECT_EQ(five_stages.error, ParseError::ExceededMaxStages);
}

TEST(ShellPipelineTest, QuotedArgumentsAndSpaces) {
    const auto result = parse_pipeline("echo \"hello world\" | grep 'world' > \"out file.txt\"");
    EXPECT_TRUE(result.is_success());
    ASSERT_EQ(result.stages.size(), 2U);
    EXPECT_EQ(result.stages[0].args, (std::vector<std::string>{"echo", "hello world"}));
    EXPECT_EQ(result.stages[1].args, (std::vector<std::string>{"grep", "world"}));
    ASSERT_TRUE(result.redirection.has_value());
    EXPECT_EQ(result.redirection->filename, "out file.txt");
    EXPECT_FALSE(result.redirection->append);
}
