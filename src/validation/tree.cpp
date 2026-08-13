// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "tree.hpp"

#include "context.hpp"

#include <algorithm>
#include <filesystem>
#include <system_error>
#include <vector>

namespace arcana::validation
{

std::vector<deck_file> walk_deck(std::filesystem::path const& root)
{
    namespace fs = std::filesystem;

    std::vector<deck_file> files;

    std::error_code ec;
    if (!fs::is_directory(root, ec))
        return files;

    auto const canonical_root = fs::weakly_canonical(root, ec);
    if (ec)
        return files;

    fs::recursive_directory_iterator it{root, fs::directory_options::skip_permission_denied, ec};
    if (ec)
        return files;

    // True when what this path names resolves to the deck root itself or to
    // something under it.
    auto const stays_inside = [&canonical_root](fs::path const& candidate)
    {
        std::error_code resolve_ec;
        auto const resolved = fs::weakly_canonical(candidate, resolve_ec);
        if (resolve_ec)
            return false;

        auto const inside = resolved.lexically_relative(canonical_root);
        return !inside.empty() && *inside.begin() != "..";
    };

    for (fs::recursive_directory_iterator const end; it != end; it.increment(ec))
    {
        if (ec)
            break;

        auto const& path = it->path();

        if (it->is_symlink(ec) && !stays_inside(path))
        {
            it.disable_recursion_pending();
            continue;
        }

        if (!it->is_regular_file(ec))
            continue;

        files.push_back({.relative = path.lexically_relative(root), .absolute = path});
    }

    std::ranges::sort(files, {}, &deck_file::relative);
    return files;
}

}  // namespace arcana::validation
