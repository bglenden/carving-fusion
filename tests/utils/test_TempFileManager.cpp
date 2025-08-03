/**
 * test_TempFileManager.cpp
 *
 * Non-fragile unit tests for TempFileManager utility class
 * Tests file path generation and directory creation logic
 */

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include "../../include/utils/TempFileManager.h"

namespace chip_carving {
namespace test {

class TempFileManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a test file in temp directory to verify it works
        testFilename_ = "test_temp_file.txt";
    }

    void TearDown() override {
        // Clean up any test files created
        std::string testPath = TempFileManager::getLogFilePath(testFilename_);
        if (std::filesystem::exists(testPath)) {
            std::filesystem::remove(testPath);
        }
    }

    std::string testFilename_;
};

TEST_F(TempFileManagerTest, GetTempDirectoryReturnsValidPath) {
    std::string tempDir = TempFileManager::getTempDirectory();
    
    EXPECT_FALSE(tempDir.empty());
    EXPECT_TRUE(tempDir.find("temp_output") != std::string::npos);
}

TEST_F(TempFileManagerTest, GetLogFilePathCreatesProperPath) {
    std::string logPath = TempFileManager::getLogFilePath("test.log");
    
    EXPECT_FALSE(logPath.empty());
    EXPECT_TRUE(logPath.find("temp_output") != std::string::npos);
    EXPECT_TRUE(logPath.find("logs") != std::string::npos);
    EXPECT_TRUE(logPath.find("test.log") != std::string::npos);
}

TEST_F(TempFileManagerTest, GetSVGFilePathCreatesProperPath) {
    std::string svgPath = TempFileManager::getSVGFilePath("test.svg");
    
    EXPECT_FALSE(svgPath.empty());
    EXPECT_TRUE(svgPath.find("temp_output") != std::string::npos);
    EXPECT_TRUE(svgPath.find("svg") != std::string::npos);
    EXPECT_TRUE(svgPath.find("test.svg") != std::string::npos);
}

TEST_F(TempFileManagerTest, EnsureTempDirectoryExistsCreatesDirectories) {
    // This should not throw and should create the directories
    EXPECT_NO_THROW(TempFileManager::ensureTempDirectoryExists());
    
    // Verify the temp directory exists
    std::string tempDir = TempFileManager::getTempDirectory();
    EXPECT_TRUE(std::filesystem::exists(tempDir));
    EXPECT_TRUE(std::filesystem::is_directory(tempDir));
}

TEST_F(TempFileManagerTest, LogDirectoryIsCreatedWhenGettingLogPath) {
    std::string logPath = TempFileManager::getLogFilePath("test.log");
    
    // Extract the directory part
    std::filesystem::path path(logPath);
    std::string logDir = path.parent_path().string();
    
    // Directory should exist after calling getLogFilePath
    EXPECT_TRUE(std::filesystem::exists(logDir));
    EXPECT_TRUE(std::filesystem::is_directory(logDir));
}

TEST_F(TempFileManagerTest, SVGDirectoryIsCreatedWhenGettingSVGPath) {
    std::string svgPath = TempFileManager::getSVGFilePath("test.svg");
    
    // Extract the directory part
    std::filesystem::path path(svgPath);
    std::string svgDir = path.parent_path().string();
    
    // Directory should exist after calling getSVGFilePath
    EXPECT_TRUE(std::filesystem::exists(svgDir));
    EXPECT_TRUE(std::filesystem::is_directory(svgDir));
}

TEST_F(TempFileManagerTest, FileCanBeCreatedAtLogPath) {
    std::string logPath = TempFileManager::getLogFilePath(testFilename_);
    
    // Try to create a file at this path
    std::ofstream testFile(logPath);
    EXPECT_TRUE(testFile.is_open());
    
    testFile << "Test content" << std::endl;
    testFile.close();
    
    // Verify file exists
    EXPECT_TRUE(std::filesystem::exists(logPath));
    
    // Verify we can read it back
    std::ifstream readFile(logPath);
    EXPECT_TRUE(readFile.is_open());
    std::string content;
    std::getline(readFile, content);
    EXPECT_EQ(content, "Test content");
}

TEST_F(TempFileManagerTest, DifferentFilenamesCreateDifferentPaths) {
    std::string path1 = TempFileManager::getLogFilePath("file1.log");
    std::string path2 = TempFileManager::getLogFilePath("file2.log");
    std::string path3 = TempFileManager::getSVGFilePath("file3.svg");
    
    EXPECT_NE(path1, path2);
    EXPECT_NE(path1, path3);
    EXPECT_NE(path2, path3);
    
    EXPECT_TRUE(path1.find("file1.log") != std::string::npos);
    EXPECT_TRUE(path2.find("file2.log") != std::string::npos);
    EXPECT_TRUE(path3.find("file3.svg") != std::string::npos);
}

}  // namespace test
}  // namespace chip_carving