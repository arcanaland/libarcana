// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <atomic>
#include <filesystem>
#include <fstream>
#include <string_view>
#include <system_error>

namespace arcana_test
{

// An RAII directory under the system temp dir
class temp_dir
{
  public:
    temp_dir()
    {
        static std::atomic<int> counter{0};
        path_ = std::filesystem::temp_directory_path() /
                ("arcana-loader-test-" + std::to_string(counter.fetch_add(1)));

        std::error_code ec;
        std::filesystem::remove_all(path_, ec);
        std::filesystem::create_directories(path_, ec);
    }

    ~temp_dir()
    {
        std::error_code ec;
        std::filesystem::remove_all(path_, ec);
    }

    temp_dir(temp_dir const&) = delete;
    temp_dir& operator=(temp_dir const&) = delete;

    [[nodiscard]] std::filesystem::path const& path() const noexcept
    {
        return path_;
    }

    // Writes a file at a deck-relative path, creating parent directories
    void write(std::string_view relative, std::string_view contents = "") const
    {
        auto const target = path_ / relative;
        std::error_code ec;
        std::filesystem::create_directories(target.parent_path(), ec);

        std::ofstream out{target, std::ios::binary};
        out << contents;
    }

  private:
    std::filesystem::path path_;
};

}  // namespace arcana_test
