// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include <arcana/version.hpp>

#include <cstdio>

int main()
{
    std::printf("%s\n", arcana::library_version().data());
    return arcana::library_version() == ARCANA_EXPECTED_VERSION ? 0 : 1;
}
