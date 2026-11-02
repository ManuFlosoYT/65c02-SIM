#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr size_t DISPLAY_WIDTH = 100;
constexpr size_t DISPLAY_HEIGHT = 64;
constexpr size_t ICON_DIM = 16;
constexpr size_t APPS_PER_PAGE = 12;

constexpr uint8_t APP_FLAG_EXEC = 0x01;
constexpr uint8_t APP_FLAG_HAS_ICON = 0x02;
constexpr uint8_t APP_FLAG_NO_DESKTOP = 0x04;

constexpr std::array<uint8_t, 4> COL_X{6, 30, 54, 78};
constexpr std::array<uint8_t, 3> ROW_Y{3, 21, 39};

constexpr uint8_t COLOR_BLACK = 0x00;
constexpr uint8_t COLOR_PINK = 0x33;
constexpr uint8_t COLOR_WHITE = 0x3F;

uint8_t generate_placeholder_pixel(size_t col, size_t row) noexcept {
    return (((col >> 2) ^ (row >> 2)) & 1) ? COLOR_PINK : COLOR_BLACK;
}

struct UdOSHeader {
    std::array<char, 4> magic{'u', 'D', 'O', 'S'};
    uint8_t version{3};
    uint8_t flags{1};
    uint16_t entry_point{0x0210};
    uint16_t core_size{0};
    uint16_t page_size{0x2000};
    uint16_t window_addr{0x2800};
    uint16_t reserved{0};
};

} // namespace

TEST(MicroDosDesktopTest, PlaceholderCheckerboardPattern) {
    std::array<std::array<uint8_t, ICON_DIM>, ICON_DIM> icon{};

    for (size_t r = 0; r < ICON_DIM; ++r) {
        for (size_t c = 0; c < ICON_DIM; ++c) {
            icon[r][c] = generate_placeholder_pixel(c, r);
        }
    }

    for (size_t r = 0; r < 4; ++r) {
        for (size_t c = 0; c < 4; ++c) {
            EXPECT_EQ(icon[r][c], COLOR_BLACK);
        }
    }

    for (size_t r = 0; r < 4; ++r) {
        for (size_t c = 4; c < 8; ++c) {
            EXPECT_EQ(icon[r][c], COLOR_PINK);
        }
    }

    for (size_t r = 4; r < 8; ++r) {
        for (size_t c = 0; c < 4; ++c) {
            EXPECT_EQ(icon[r][c], COLOR_PINK);
        }
    }

    for (size_t r = 4; r < 8; ++r) {
        for (size_t c = 4; c < 8; ++c) {
            EXPECT_EQ(icon[r][c], COLOR_BLACK);
        }
    }
}

TEST(MicroDosDesktopTest, GridLayoutGeometryWithinScreenBounds) {
    constexpr size_t BOTTOM_BAR_Y = 56;

    for (size_t row = 0; row < ROW_Y.size(); ++row) {
        for (size_t col = 0; col < COL_X.size(); ++col) {
            size_t left = COL_X[col];
            size_t right = left + ICON_DIM;
            size_t top = ROW_Y[row];
            size_t bottom = top + ICON_DIM;

            EXPECT_GE(left, 0U);
            EXPECT_LE(right, DISPLAY_WIDTH);
            EXPECT_GE(top, 0U);
            EXPECT_LE(bottom, BOTTOM_BAR_Y);
        }
    }
}

TEST(MicroDosDesktopTest, PaginationCalculationAndWrapping) {
    auto calc_total_pages = [](size_t total_apps) -> size_t {
        if (total_apps == 0) return 1;
        return (total_apps + APPS_PER_PAGE - 1) / APPS_PER_PAGE;
    };

    EXPECT_EQ(calc_total_pages(0), 1U);
    EXPECT_EQ(calc_total_pages(1), 1U);
    EXPECT_EQ(calc_total_pages(12), 1U);
    EXPECT_EQ(calc_total_pages(13), 2U);
    EXPECT_EQ(calc_total_pages(24), 2U);
    EXPECT_EQ(calc_total_pages(25), 3U);
    EXPECT_EQ(calc_total_pages(36), 3U);

    size_t total_pages = calc_total_pages(25);
    size_t current_page = 0;

    current_page = (current_page > 0) ? (current_page - 1) : (total_pages - 1);
    EXPECT_EQ(current_page, 2U);

    current_page = (current_page + 1 < total_pages) ? (current_page + 1) : 0;
    EXPECT_EQ(current_page, 0U);
}

TEST(MicroDosDesktopTest, HeaderFlagsAndIconOffsets) {
    UdOSHeader standardHdr{};
    EXPECT_EQ(sizeof(UdOSHeader), 16U);
    EXPECT_EQ(standardHdr.flags & 0x02, 0);
    EXPECT_EQ(standardHdr.entry_point, 0x0210);

    UdOSHeader iconHdr{};
    iconHdr.flags = 0x03;
    iconHdr.entry_point = 0x0310;
    EXPECT_NE(iconHdr.flags & 0x02, 0);
    EXPECT_EQ(iconHdr.entry_point, 0x0310);
    EXPECT_EQ(iconHdr.entry_point - 0x0210, 256);
}

TEST(MicroDosDesktopTest, DesktopSysVectorsAndPaths) {
    constexpr uint16_t VECTOR_BASE = 0x0200;
    constexpr uint16_t VECTOR_INIT = VECTOR_BASE + 0;
    constexpr uint16_t VECTOR_TICK = VECTOR_BASE + 3;
    constexpr uint16_t VECTOR_REDRAW = VECTOR_BASE + 6;

    EXPECT_EQ(VECTOR_INIT, 0x0200);
    EXPECT_EQ(VECTOR_TICK, 0x0203);
    EXPECT_EQ(VECTOR_REDRAW, 0x0206);

    constexpr std::string_view LAUNCH_FILE = "/SYSTEM/DESKTOP/launch.dat";
    constexpr std::string_view DESKTOP_SYS = "/SYSTEM/DESKTOP/DESKTOP.SYS";
    constexpr std::string_view BLISS_PATH = "/SYSTEM/DESKTOP/bliss.bin";

    EXPECT_EQ(LAUNCH_FILE, "/SYSTEM/DESKTOP/launch.dat");
    EXPECT_EQ(DESKTOP_SYS, "/SYSTEM/DESKTOP/DESKTOP.SYS");
    EXPECT_EQ(BLISS_PATH, "/SYSTEM/DESKTOP/bliss.bin");
}

TEST(MicroDosDesktopTest, HeaderFlagConstants) {
    static_assert(APP_FLAG_EXEC == 0x01);
    static_assert(APP_FLAG_HAS_ICON == 0x02);
    static_assert(APP_FLAG_NO_DESKTOP == 0x04);

    EXPECT_EQ(APP_FLAG_EXEC, 0x01);
    EXPECT_EQ(APP_FLAG_HAS_ICON, 0x02);
    EXPECT_EQ(APP_FLAG_NO_DESKTOP, 0x04);
}

TEST(MicroDosDesktopTest, HeaderFlagCombinations) {
    constexpr uint8_t FLAG_NORMAL_APP = APP_FLAG_EXEC;
    constexpr uint8_t FLAG_APP_WITH_ICON = APP_FLAG_EXEC | APP_FLAG_HAS_ICON;
    constexpr uint8_t FLAG_CLI_NO_DESKTOP = APP_FLAG_EXEC | APP_FLAG_NO_DESKTOP;
    constexpr uint8_t FLAG_ICON_NO_DESKTOP = APP_FLAG_EXEC | APP_FLAG_HAS_ICON | APP_FLAG_NO_DESKTOP;

    EXPECT_EQ(FLAG_NORMAL_APP, 0x01);
    EXPECT_EQ(FLAG_APP_WITH_ICON, 0x03);
    EXPECT_EQ(FLAG_CLI_NO_DESKTOP, 0x05);
    EXPECT_EQ(FLAG_ICON_NO_DESKTOP, 0x07);

    auto is_desktop_app = [](uint8_t flags) noexcept -> bool {
        return (flags & APP_FLAG_NO_DESKTOP) == 0;
    };

    EXPECT_TRUE(is_desktop_app(FLAG_NORMAL_APP));
    EXPECT_TRUE(is_desktop_app(FLAG_APP_WITH_ICON));
    EXPECT_FALSE(is_desktop_app(FLAG_CLI_NO_DESKTOP));
    EXPECT_FALSE(is_desktop_app(FLAG_ICON_NO_DESKTOP));
}

TEST(MicroDosDesktopTest, DesktopAppFilteringScan) {
    struct MockAppFile {
        std::string filename;
        UdOSHeader header;
    };

    const std::vector<MockAppFile> mock_bin_files = {
        {"calc.app", UdOSHeader{.flags = APP_FLAG_EXEC}},
        {"notes.app", UdOSHeader{.flags = APP_FLAG_EXEC | APP_FLAG_HAS_ICON}},
        {"daemon.app", UdOSHeader{.flags = APP_FLAG_EXEC | APP_FLAG_NO_DESKTOP}},
        {"service.app", UdOSHeader{.flags = APP_FLAG_EXEC | APP_FLAG_HAS_ICON | APP_FLAG_NO_DESKTOP}},
        {"readme.txt", UdOSHeader{.flags = 0}},
        {"term.app", UdOSHeader{.flags = APP_FLAG_EXEC}},
    };

    auto is_app_file = [](std::string_view name) noexcept -> bool {
        return name.ends_with(".app") || name.ends_with(".APP");
    };

    std::vector<std::string> scanned_apps;
    for (const auto& file : mock_bin_files) {
        if (!is_app_file(file.filename)) {
            continue;
        }

        std::array<uint8_t, 16> hdr{};
        hdr[5] = file.header.flags;

        if (hdr[5] & APP_FLAG_NO_DESKTOP) {
            continue;
        }

        std::string base_name = file.filename.substr(0, file.filename.size() - 4);
        if (base_name.size() > 8) {
            base_name.resize(8);
        }
        scanned_apps.push_back(base_name);
    }

    const std::vector<std::string> expected = {"calc", "notes", "term"};
    EXPECT_EQ(scanned_apps, expected);

    auto ranges_filtered_apps = mock_bin_files
        | std::views::filter([&](const MockAppFile& f) {
            return is_app_file(f.filename) && ((f.header.flags & APP_FLAG_NO_DESKTOP) == 0);
        })
        | std::views::transform([](const MockAppFile& f) {
            std::string base = f.filename.substr(0, f.filename.size() - 4);
            if (base.size() > 8) {
                base.resize(8);
            }
            return base;
        })
        | std::ranges::to<std::vector>();

    EXPECT_EQ(ranges_filtered_apps, expected);
}


