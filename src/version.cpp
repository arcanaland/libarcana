#include <arcana/version.hpp>

namespace arcana
{

std::string_view library_version() noexcept
{
    return version;
}

}  // namespace arcana
