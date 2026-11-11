#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr size_t VISIBLE_ROWS = 8;
constexpr size_t VISIBLE_COLS = 23;
constexpr size_t MAX_LINE_LEN = 120;
constexpr size_t MAX_LINES = 64;

constexpr uint8_t COLOR_TEXT_BODY = 0x00;
constexpr uint8_t COLOR_TEXT_HEADING = 0x03;
constexpr uint8_t COLOR_TEXT_RULE = 0x15;
constexpr uint8_t COLOR_TEXT_LIST = 0x0C;

struct ParsedUrl {
    std::string protocol;
    std::string host;
    uint16_t port{443};
    std::string path;

    auto operator<=>(const ParsedUrl&) const = default;
};

[[nodiscard]] constexpr char to_ascii_lower(char c) noexcept {
    if (c >= 'A' && c <= 'Z') {
        return static_cast<char>(c + ('a' - 'A'));
    }
    return c;
}

[[nodiscard]] constexpr bool istarts_with(std::string_view sv, std::string_view prefix) noexcept {
    if (sv.size() < prefix.size()) {
        return false;
    }
    for (size_t i = 0; i < prefix.size(); ++i) {
        if (to_ascii_lower(sv[i]) != to_ascii_lower(prefix[i])) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] ParsedUrl parse_browser_url(std::string_view raw_url) {
    ParsedUrl result{};
    result.protocol = "SSL";
    result.port = 443;

    std::string_view rem = raw_url;
    while (!rem.empty() && (rem.front() == ' ' || rem.front() == '\t')) {
        rem.remove_prefix(1);
    }
    while (!rem.empty() && (rem.back() == ' ' || rem.back() == '\t' || rem.back() == '\r' || rem.back() == '\n')) {
        rem.remove_suffix(1);
    }

    if (istarts_with(rem, "http://")) {
        result.protocol = "TCP";
        result.port = 80;
        rem.remove_prefix(7);
    } else if (istarts_with(rem, "https://")) {
        result.protocol = "SSL";
        result.port = 443;
        rem.remove_prefix(8);
    }

    const size_t slash_pos = rem.find('/');
    std::string_view host_port;
    if (slash_pos != std::string_view::npos) {
        host_port = rem.substr(0, slash_pos);
        result.path = std::string(rem.substr(slash_pos));
    } else {
        host_port = rem;
        result.path = "/";
    }

    const size_t colon_pos = host_port.find(':');
    if (colon_pos != std::string_view::npos) {
        result.host = std::string(host_port.substr(0, colon_pos));
        const std::string_view port_sv = host_port.substr(colon_pos + 1);
        uint32_t parsed_port = 0;
        bool has_digits = false;
        for (const char c : port_sv) {
            if (c >= '0' && c <= '9') {
                parsed_port = parsed_port * 10U + static_cast<uint32_t>(c - '0');
                has_digits = true;
            } else {
                break;
            }
        }
        if (has_digits && parsed_port > 0 && parsed_port <= 65535U) {
            result.port = static_cast<uint16_t>(parsed_port);
        }
    } else {
        result.host = std::string(host_port);
    }

    if (result.path.empty()) {
        result.path = "/";
    }

    return result;
}

struct RenderedLine {
    std::string text;
    uint8_t color{COLOR_TEXT_BODY};

    auto operator<=>(const RenderedLine&) const = default;
};

struct HtmlDocument {
    std::string title;
    std::vector<RenderedLine> lines;
    size_t max_line_len{0};
};

[[nodiscard]] constexpr char decode_html_entity(std::string_view entity, size_t& consumed) noexcept {
    struct EntityMapping {
        std::string_view token;
        char decoded;
    };
    constexpr std::array<EntityMapping, 7> mappings{{
        {"&amp;", '&'},
        {"&lt;", '<'},
        {"&gt;", '>'},
        {"&quot;", '"'},
        {"&#39;", '\''},
        {"&apos;", '\''},
        {"&nbsp;", ' '},
    }};

    for (const auto& m : mappings) {
        if (istarts_with(entity, m.token)) {
            consumed = m.token.size();
            return m.decoded;
        }
    }
    consumed = 1;
    return '&';
}

[[nodiscard]] HtmlDocument parse_html_document(std::string_view html) {
    HtmlDocument doc{};
    std::string current_line;
    uint8_t current_color = COLOR_TEXT_BODY;
    bool in_title = false;
    bool last_was_space = true;

    auto flush_line = [&]() {
        while (!current_line.empty() && current_line.back() == ' ') {
            current_line.pop_back();
        }
        if (!current_line.empty() && doc.lines.size() < MAX_LINES) {
            doc.max_line_len = std::max(doc.max_line_len, current_line.size());
            doc.lines.push_back(RenderedLine{.text = current_line, .color = current_color});
        }
        current_line.clear();
        current_color = COLOR_TEXT_BODY;
        last_was_space = true;
    };

    auto append_char = [&](char c) {
        if (in_title) {
            if (c == '\r' || c == '\n' || c == '\t') {
                c = ' ';
            }
            if (c == ' ' && (doc.title.empty() || doc.title.back() == ' ')) {
                return;
            }
            doc.title.push_back(c);
            return;
        }
        if (c == '\r' || c == '\n' || c == '\t') {
            c = ' ';
        }
        if (c == ' ') {
            if (last_was_space) {
                return;
            }
            last_was_space = true;
        } else {
            last_was_space = false;
        }
        if (current_line.size() < MAX_LINE_LEN) {
            current_line.push_back(c);
        }
    };

    size_t i = 0;
    while (i < html.size()) {
        if (istarts_with(html.substr(i), "<!--")) {
            const size_t end_comment = html.find("-->", i + 4);
            if (end_comment == std::string_view::npos) {
                break;
            }
            i = end_comment + 3;
            continue;
        }

        if (html[i] == '<') {
            const size_t close_pos = html.find('>', i + 1);
            if (close_pos == std::string_view::npos) {
                break;
            }
            std::string_view tag_content = html.substr(i + 1, close_pos - i - 1);
            while (!tag_content.empty() && tag_content.front() == ' ') {
                tag_content.remove_prefix(1);
            }

            std::string tag_name;
            for (const char c : tag_content) {
                if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '>') {
                    break;
                }
                if (c == '/' && !tag_name.empty()) {
                    break;
                }
                tag_name.push_back(to_ascii_lower(c));
            }

            i = close_pos + 1;

            if (tag_name == "script") {
                while (i < html.size()) {
                    if (istarts_with(html.substr(i), "</script")) {
                        const size_t end_script = html.find('>', i + 8);
                        i = (end_script == std::string_view::npos) ? html.size() : end_script + 1;
                        break;
                    }
                    ++i;
                }
                continue;
            }

            if (tag_name == "style") {
                while (i < html.size()) {
                    if (istarts_with(html.substr(i), "</style")) {
                        const size_t end_style = html.find('>', i + 7);
                        i = (end_style == std::string_view::npos) ? html.size() : end_style + 1;
                        break;
                    }
                    ++i;
                }
                continue;
            }

            if (tag_name == "title") {
                in_title = true;
            } else if (tag_name == "/title") {
                in_title = false;
                while (!doc.title.empty() && doc.title.back() == ' ') {
                    doc.title.pop_back();
                }
            } else if (tag_name == "h1" || tag_name == "h2" || tag_name == "h3") {
                flush_line();
                current_color = COLOR_TEXT_HEADING;
            } else if (tag_name == "/h1" || tag_name == "/h2" || tag_name == "/h3" ||
                       tag_name == "p" || tag_name == "/p" || tag_name == "div" || tag_name == "/div" ||
                       tag_name == "br") {
                flush_line();
            } else if (tag_name == "li") {
                flush_line();
                current_color = COLOR_TEXT_LIST;
                current_line = "* ";
                last_was_space = true;
            } else if (tag_name == "/li") {
                flush_line();
            } else if (tag_name == "hr") {
                flush_line();
                if (doc.lines.size() < MAX_LINES) {
                    const std::string rule(VISIBLE_COLS, '-');
                    doc.max_line_len = std::max(doc.max_line_len, rule.size());
                    doc.lines.push_back(RenderedLine{.text = rule, .color = COLOR_TEXT_RULE});
                }
            }
            continue;
        }

        if (html[i] == '&') {
            size_t consumed = 1;
            const char decoded = decode_html_entity(html.substr(i), consumed);
            append_char(decoded);
            i += consumed;
            continue;
        }

        append_char(html[i]);
        ++i;
    }

    flush_line();
    return doc;
}

struct ScrollbarGeometry {
    int track_start{0};
    int track_end{0};
    int thumb_size{4};

    [[nodiscard]] constexpr int track_range() const noexcept {
        return std::max(0, (track_end - track_start + 1) - thumb_size);
    }

    [[nodiscard]] constexpr int compute_thumb_pos(size_t scroll_offset, size_t max_scroll) const noexcept {
        if (max_scroll == 0) {
            return track_start;
        }
        const size_t clamped = std::min(scroll_offset, max_scroll);
        return track_start + static_cast<int>((clamped * static_cast<size_t>(track_range())) / max_scroll);
    }

    [[nodiscard]] constexpr size_t mouse_to_scroll(int mouse_coord, size_t max_scroll) const noexcept {
        if (max_scroll == 0) {
            return 0;
        }
        const int range = track_range();
        if (range <= 0 || mouse_coord <= track_start) {
            return 0;
        }
        if (mouse_coord >= track_start + range) {
            return max_scroll;
        }
        const int rel = mouse_coord - track_start;
        return (static_cast<size_t>(rel) * max_scroll + static_cast<size_t>(range / 2)) / static_cast<size_t>(range);
    }
};

constexpr ScrollbarGeometry VERTICAL_SCROLLBAR{
    .track_start = 13,
    .track_end = 55,
    .thumb_size = 5
};

constexpr ScrollbarGeometry HORIZONTAL_SCROLLBAR{
    .track_start = 4,
    .track_end = 91,
    .thumb_size = 6
};

[[nodiscard]] constexpr size_t compute_max_scroll_y(size_t total_lines) noexcept {
    return (total_lines > VISIBLE_ROWS) ? (total_lines - VISIBLE_ROWS) : 0;
}

[[nodiscard]] constexpr size_t compute_max_scroll_x(size_t max_line_len) noexcept {
    return (max_line_len > VISIBLE_COLS) ? (max_line_len - VISIBLE_COLS) : 0;
}

}  // namespace

TEST(MicroDosBrowserTest, UrlParserDefaultHttpsAndExplicitHttps) {
    const auto implicit_url = parse_browser_url("example.com");
    EXPECT_EQ(implicit_url.protocol, "SSL");
    EXPECT_EQ(implicit_url.host, "example.com");
    EXPECT_EQ(implicit_url.port, 443);
    EXPECT_EQ(implicit_url.path, "/");

    const auto explicit_https = parse_browser_url("https://example.com/index.html");
    EXPECT_EQ(explicit_https.protocol, "SSL");
    EXPECT_EQ(explicit_https.host, "example.com");
    EXPECT_EQ(explicit_https.port, 443);
    EXPECT_EQ(explicit_https.path, "/index.html");

    const auto uppercase_https = parse_browser_url("HTTPS://retro.org/docs/guide.htm");
    EXPECT_EQ(uppercase_https.protocol, "SSL");
    EXPECT_EQ(uppercase_https.host, "retro.org");
    EXPECT_EQ(uppercase_https.port, 443);
    EXPECT_EQ(uppercase_https.path, "/docs/guide.htm");
}

TEST(MicroDosBrowserTest, UrlParserHttpAndCustomPorts) {
    const auto http_url = parse_browser_url("http://info.cern.ch/hypertext/WWW/TheProject.html");
    EXPECT_EQ(http_url.protocol, "TCP");
    EXPECT_EQ(http_url.host, "info.cern.ch");
    EXPECT_EQ(http_url.port, 80);
    EXPECT_EQ(http_url.path, "/hypertext/WWW/TheProject.html");

    const auto custom_http_port = parse_browser_url("http://localhost:8080/test");
    EXPECT_EQ(custom_http_port.protocol, "TCP");
    EXPECT_EQ(custom_http_port.host, "localhost");
    EXPECT_EQ(custom_http_port.port, 8080);
    EXPECT_EQ(custom_http_port.path, "/test");

    const auto custom_https_port = parse_browser_url("https://api.local:8443");
    EXPECT_EQ(custom_https_port.protocol, "SSL");
    EXPECT_EQ(custom_https_port.host, "api.local");
    EXPECT_EQ(custom_https_port.port, 8443);
    EXPECT_EQ(custom_https_port.path, "/");
}

TEST(MicroDosBrowserTest, HtmlParserSkipsScriptStyleAndComments) {
    constexpr std::string_view html =
        "<html><head><title>Test Page</title>"
        "<style>body { color: red; }</style>"
        "<script type=\"text/javascript\">alert('ignored');</script>"
        "</head><body>"
        "<!-- hidden comment <p>do not show</p> -->"
        "<p>Visible Content</p>"
        "</body></html>";

    const auto doc = parse_html_document(html);
    EXPECT_EQ(doc.title, "Test Page");
    ASSERT_EQ(doc.lines.size(), 1U);
    EXPECT_EQ(doc.lines[0].text, "Visible Content");
    EXPECT_EQ(doc.lines[0].color, COLOR_TEXT_BODY);
}

TEST(MicroDosBrowserTest, HtmlParserBlockTagsAndLineColors) {
    constexpr std::string_view html =
        "<title>65C02 Portal</title>"
        "<h1>Welcome</h1>"
        "<hr>"
        "<p>First paragraph<br>Second line</p>"
        "<ul><li>Item One</li><li>Item Two</li></ul>";

    const auto doc = parse_html_document(html);
    EXPECT_EQ(doc.title, "65C02 Portal");
    ASSERT_EQ(doc.lines.size(), 6U);

    EXPECT_EQ(doc.lines[0].text, "Welcome");
    EXPECT_EQ(doc.lines[0].color, COLOR_TEXT_HEADING);

    EXPECT_EQ(doc.lines[1].text, std::string(VISIBLE_COLS, '-'));
    EXPECT_EQ(doc.lines[1].color, COLOR_TEXT_RULE);

    EXPECT_EQ(doc.lines[2].text, "First paragraph");
    EXPECT_EQ(doc.lines[2].color, COLOR_TEXT_BODY);

    EXPECT_EQ(doc.lines[3].text, "Second line");
    EXPECT_EQ(doc.lines[3].color, COLOR_TEXT_BODY);

    EXPECT_EQ(doc.lines[4].text, "* Item One");
    EXPECT_EQ(doc.lines[4].color, COLOR_TEXT_LIST);

    EXPECT_EQ(doc.lines[5].text, "* Item Two");
    EXPECT_EQ(doc.lines[5].color, COLOR_TEXT_LIST);
}

TEST(MicroDosBrowserTest, HtmlParserDecodesEntities) {
    constexpr std::string_view html =
        "<p>AT&amp;T &lt;65C02&gt; &quot;CPU&quot; &#39;8-bit&#39; &apos;MOS&apos;&nbsp;OK</p>";

    const auto doc = parse_html_document(html);
    ASSERT_EQ(doc.lines.size(), 1U);
    EXPECT_EQ(doc.lines[0].text, "AT&T <65C02> \"CPU\" '8-bit' 'MOS' OK");
}

TEST(MicroDosBrowserTest, HtmlParserPreservesLongLinesForHorizontalScroll) {
    constexpr std::string_view html =
        "<p>This line is significantly longer than twenty-three characters so horizontal scrolling engages.</p>";

    const auto doc = parse_html_document(html);
    ASSERT_EQ(doc.lines.size(), 1U);
    EXPECT_GT(doc.lines[0].text.size(), VISIBLE_COLS);
    EXPECT_EQ(doc.max_line_len, doc.lines[0].text.size());
    EXPECT_GT(compute_max_scroll_x(doc.max_line_len), 0U);
}

TEST(MicroDosBrowserTest, VerticalScrollbarGeometryAndMouseMapping) {
    constexpr int V_BAR_X_MIN = 96;
    constexpr int V_BAR_X_MAX = 99;
    constexpr int V_BAR_Y_MIN = 9;
    constexpr int V_BAR_Y_MAX = 59;

    EXPECT_EQ(V_BAR_X_MAX - V_BAR_X_MIN + 1, 4);
    EXPECT_GT(VERTICAL_SCROLLBAR.track_start, V_BAR_Y_MIN);
    EXPECT_LT(VERTICAL_SCROLLBAR.track_end, V_BAR_Y_MAX);

    const size_t max_scroll_y = compute_max_scroll_y(24);
    EXPECT_EQ(max_scroll_y, 16U);

    EXPECT_EQ(VERTICAL_SCROLLBAR.compute_thumb_pos(0, max_scroll_y), VERTICAL_SCROLLBAR.track_start);
    EXPECT_EQ(VERTICAL_SCROLLBAR.compute_thumb_pos(max_scroll_y, max_scroll_y),
              VERTICAL_SCROLLBAR.track_start + VERTICAL_SCROLLBAR.track_range());

    EXPECT_EQ(VERTICAL_SCROLLBAR.mouse_to_scroll(0, max_scroll_y), 0U);
    EXPECT_EQ(VERTICAL_SCROLLBAR.mouse_to_scroll(VERTICAL_SCROLLBAR.track_start, max_scroll_y), 0U);
    EXPECT_EQ(VERTICAL_SCROLLBAR.mouse_to_scroll(VERTICAL_SCROLLBAR.track_end, max_scroll_y), max_scroll_y);
    EXPECT_EQ(VERTICAL_SCROLLBAR.mouse_to_scroll(63, max_scroll_y), max_scroll_y);

    const int mid_y = VERTICAL_SCROLLBAR.track_start + VERTICAL_SCROLLBAR.track_range() / 2;
    const size_t mid_scroll = VERTICAL_SCROLLBAR.mouse_to_scroll(mid_y, max_scroll_y);
    EXPECT_EQ(mid_scroll, 8U);
}

TEST(MicroDosBrowserTest, HorizontalScrollbarGeometryAndMouseMapping) {
    constexpr int H_BAR_X_MIN = 0;
    constexpr int H_BAR_X_MAX = 95;
    constexpr int H_BAR_Y_MIN = 60;
    constexpr int H_BAR_Y_MAX = 63;

    EXPECT_EQ(H_BAR_Y_MAX - H_BAR_Y_MIN + 1, 4);
    EXPECT_GT(HORIZONTAL_SCROLLBAR.track_start, H_BAR_X_MIN);
    EXPECT_LT(HORIZONTAL_SCROLLBAR.track_end, H_BAR_X_MAX);

    EXPECT_EQ(compute_max_scroll_x(20), 0U);
    EXPECT_EQ(compute_max_scroll_x(23), 0U);

    const size_t max_scroll_x = compute_max_scroll_x(63);
    EXPECT_EQ(max_scroll_x, 40U);

    EXPECT_EQ(HORIZONTAL_SCROLLBAR.compute_thumb_pos(0, max_scroll_x), HORIZONTAL_SCROLLBAR.track_start);
    EXPECT_EQ(HORIZONTAL_SCROLLBAR.compute_thumb_pos(max_scroll_x, max_scroll_x),
              HORIZONTAL_SCROLLBAR.track_start + HORIZONTAL_SCROLLBAR.track_range());

    EXPECT_EQ(HORIZONTAL_SCROLLBAR.mouse_to_scroll(0, max_scroll_x), 0U);
    EXPECT_EQ(HORIZONTAL_SCROLLBAR.mouse_to_scroll(HORIZONTAL_SCROLLBAR.track_start, max_scroll_x), 0U);
    EXPECT_EQ(HORIZONTAL_SCROLLBAR.mouse_to_scroll(HORIZONTAL_SCROLLBAR.track_end, max_scroll_x), max_scroll_x);
    EXPECT_EQ(HORIZONTAL_SCROLLBAR.mouse_to_scroll(99, max_scroll_x), max_scroll_x);

    const int mid_x = HORIZONTAL_SCROLLBAR.track_start + HORIZONTAL_SCROLLBAR.track_range() / 2;
    const size_t mid_scroll = HORIZONTAL_SCROLLBAR.mouse_to_scroll(mid_x, max_scroll_x);
    EXPECT_EQ(mid_scroll, 20U);
}
