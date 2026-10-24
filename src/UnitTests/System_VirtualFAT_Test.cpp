#include "Hardware/Storage/SDCard.h"
#include <gtest/gtest.h>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {

void InitSDSPI(Hardware::SDCard& sd) {
    sd.SetCS(true);
    for (int i = 0; i < 15; ++i) {
        sd.TransferByte(0xFF);
    }
    sd.SetCS(false);

    std::array<uint8_t, 6> cmd0 = {0x40, 0x00, 0x00, 0x00, 0x00, 0x95};
    for (uint8_t b : cmd0) {
        sd.TransferByte(b);
    }
    for (int i = 0; i < 20; ++i) {
        if (sd.TransferByte(0xFF) == 0x01) {
            break;
        }
    }

    std::array<uint8_t, 6> cmd8 = {0x48, 0x00, 0x00, 0x01, 0xAA, 0x87};
    for (uint8_t b : cmd8) {
        sd.TransferByte(b);
    }
    for (int i = 0; i < 20; ++i) {
        if (sd.TransferByte(0xFF) == 0x01) {
            sd.TransferByte(0xFF);
            sd.TransferByte(0xFF);
            sd.TransferByte(0xFF);
            sd.TransferByte(0xFF);
            break;
        }
    }

    for (int attempt = 0; attempt < 200; ++attempt) {
        std::array<uint8_t, 6> cmd55 = {0x77, 0x00, 0x00, 0x00, 0x00, 0x01};
        for (uint8_t b : cmd55) {
            sd.TransferByte(b);
        }
        for (int i = 0; i < 20; ++i) {
            if (sd.TransferByte(0xFF) <= 0x01) {
                break;
            }
        }

        std::array<uint8_t, 6> acmd41 = {0x69, 0x40, 0x00, 0x00, 0x00, 0x01};
        for (uint8_t b : acmd41) {
            sd.TransferByte(b);
        }
        uint8_t res = 0xFF;
        for (int i = 0; i < 20; ++i) {
            res = sd.TransferByte(0xFF);
            if (res == 0x00 || res == 0x01) {
                break;
            }
        }
        if (res == 0x00) {
            break;
        }
    }
}

bool ReadSectorSPI(Hardware::SDCard& sd, uint32_t lba, std::array<uint8_t, 512>& outData) {
    std::array<uint8_t, 6> cmd17 = {
        0x51,
        static_cast<uint8_t>((lba >> 24) & 0xFF),
        static_cast<uint8_t>((lba >> 16) & 0xFF),
        static_cast<uint8_t>((lba >> 8) & 0xFF),
        static_cast<uint8_t>(lba & 0xFF),
        0x01
    };
    for (uint8_t b : cmd17) {
        sd.TransferByte(b);
    }

    bool ok = false;
    for (int i = 0; i < 30; ++i) {
        if (sd.TransferByte(0xFF) == 0x00) {
            ok = true;
            break;
        }
    }
    if (!ok) {
        return false;
    }

    bool tokenFound = false;
    for (int i = 0; i < 150; ++i) {
        if (sd.TransferByte(0xFF) == 0xFE) {
            tokenFound = true;
            break;
        }
    }
    if (!tokenFound) {
        return false;
    }

    for (size_t i = 0; i < 512; ++i) {
        outData.at(i) = sd.TransferByte(0xFF);
    }
    sd.TransferByte(0xFF);
    sd.TransferByte(0xFF);
    return true;
}

} // namespace

TEST(System_VirtualFAT_Test, Mount_ValidFolder_BPB_And_Files) {
    std::filesystem::path tempDir = std::filesystem::temp_directory_path() / "65c02_test_vfat_mount";
    std::filesystem::remove_all(tempDir);
    std::filesystem::create_directories(tempDir / "SUB");

    {
        std::ofstream file(tempDir / "HELLO.TXT");
        file << "Hello from 65C02 Virtual FAT!";
    }
    {
        std::ofstream file(tempDir / "SUB" / "DATA.BIN", std::ios::binary);
        std::vector<uint8_t> bytes = {0x01, 0x02, 0x03, 0x04};
        file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }
    {
        std::ofstream sentinel(tempDir / ".65C02_SIM_mount");
        sentinel << "sentinel";
    }

    Hardware::SDCard sd;
    ASSERT_TRUE(sd.Mount(tempDir.string()));
    EXPECT_TRUE(sd.IsMounted());
    EXPECT_TRUE(sd.IsFolderMount());
    EXPECT_TRUE(sd.IsDeletionAllowed());

    InitSDSPI(sd);

    std::array<uint8_t, 512> sector0{};
    ASSERT_TRUE(ReadSectorSPI(sd, 0, sector0));
    EXPECT_EQ(sector0.at(510), 0x55);
    EXPECT_EQ(sector0.at(511), 0xAA);
    EXPECT_EQ(sector0.at(11), 0x00);
    EXPECT_EQ(sector0.at(12), 0x02);
    EXPECT_EQ(sector0.at(13), 8);

    std::string fstype(reinterpret_cast<const char*>(&sector0.at(54)), 8);
    EXPECT_EQ(fstype, "FAT16   ");

    std::array<uint8_t, 512> rootDir{};
    ASSERT_TRUE(ReadSectorSPI(sd, 68, rootDir));

    std::string rootStr(reinterpret_cast<const char*>(rootDir.data()), rootDir.size());
    EXPECT_NE(rootStr.find("HELLO   TXT"), std::string::npos);
    EXPECT_NE(rootStr.find("SUB     "), std::string::npos);
    EXPECT_EQ(rootStr.find(".65C02"), std::string::npos);

    std::array<uint8_t, 512> dataSector{};
    ASSERT_TRUE(ReadSectorSPI(sd, 100, dataSector));
    std::string fileContent(reinterpret_cast<const char*>(dataSector.data()));
    EXPECT_NE(fileContent.find("Hello from 65C02 Virtual FAT!"), std::string::npos);

    sd.Unmount();
    EXPECT_FALSE(sd.IsMounted());
    std::filesystem::remove_all(tempDir);
}

TEST(System_VirtualFAT_Test, SentinelFile_HiddenFromVirtualFAT) {
    std::filesystem::path tempDir = std::filesystem::temp_directory_path() / "65c02_test_vfat_sentinel";
    std::filesystem::remove_all(tempDir);
    std::filesystem::create_directories(tempDir);

    {
        std::ofstream sentinel(tempDir / ".65C02_SIM_mount");
        sentinel << "ok";
    }
    {
        std::ofstream dummy(tempDir / "DUMMY.DAT");
        dummy << "dummy";
    }

    Hardware::SDCard sd;
    ASSERT_TRUE(sd.Mount(tempDir.string()));
    InitSDSPI(sd);

    for (uint32_t sec = 68; sec < 100; ++sec) {
        std::array<uint8_t, 512> sector{};
        ASSERT_TRUE(ReadSectorSPI(sd, sec, sector));
        std::string s(reinterpret_cast<const char*>(sector.data()), sector.size());
        EXPECT_EQ(s.find(".65C02"), std::string::npos);
        EXPECT_EQ(s.find("SIM_MOUNT"), std::string::npos);
    }

    sd.Unmount();
    std::filesystem::remove_all(tempDir);
}

TEST(System_VirtualFAT_Test, HostSync_PicksUpNewContent) {
    std::filesystem::path tempDir = std::filesystem::temp_directory_path() / "65c02_test_vfat_sync";
    std::filesystem::remove_all(tempDir);
    std::filesystem::create_directories(tempDir);

    std::filesystem::path txtFile = tempDir / "DYNAMIC.TXT";
    {
        std::ofstream file(txtFile);
        file << "Version 1";
    }

    Hardware::SDCard sd;
    ASSERT_TRUE(sd.Mount(tempDir.string()));
    InitSDSPI(sd);

    std::array<uint8_t, 512> sectorBefore{};
    ASSERT_TRUE(ReadSectorSPI(sd, 100, sectorBefore));
    std::string contentBefore(reinterpret_cast<const char*>(sectorBefore.data()));
    EXPECT_NE(contentBefore.find("Version 1"), std::string::npos);

    {
        std::ofstream file(txtFile, std::ios::trunc);
        file << "Version 2 Modified";
    }

    EXPECT_TRUE(sd.SyncFromHost());

    std::array<uint8_t, 512> sectorAfter{};
    ASSERT_TRUE(ReadSectorSPI(sd, 100, sectorAfter));
    std::string contentAfter(reinterpret_cast<const char*>(sectorAfter.data()));
    EXPECT_NE(contentAfter.find("Version 2 Modified"), std::string::npos);

    sd.Unmount();
    std::filesystem::remove_all(tempDir);
}

TEST(System_VirtualFAT_Test, Security_Blacklist) {
    Hardware::SDCard sd;
    EXPECT_FALSE(sd.MountFolder("/"));
    EXPECT_FALSE(sd.MountFolder("/home"));
    EXPECT_FALSE(sd.MountFolder("/usr"));
    EXPECT_FALSE(sd.MountFolder("/bin"));
    EXPECT_FALSE(sd.MountFolder("/etc"));
    EXPECT_FALSE(sd.MountFolder("/var"));
    EXPECT_FALSE(sd.MountFolder("/tmp"));
}

TEST(System_VirtualFAT_Test, RestrictedDeletion_WhenSentinelMissing) {
    std::filesystem::path tempDir = std::filesystem::temp_directory_path() / "65c02_test_vfat_nodel";
    std::filesystem::remove_all(tempDir);
    std::filesystem::create_directories(tempDir);

    {
        std::ofstream file(tempDir / "TEST.TXT");
        file << "Protected file";
    }

    Hardware::SDCard sd;
    ASSERT_TRUE(sd.Mount(tempDir.string()));
    EXPECT_FALSE(sd.IsDeletionAllowed());

    sd.Unmount();
    std::filesystem::remove_all(tempDir);
}

