// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include <arcana/version.hpp>

#include <cstdio>

int main()
{
    std::printf("%s\n", arcana::library_version().data());
    return arcana::library_version() == "0.1.0" ? 0 : 1;
}
