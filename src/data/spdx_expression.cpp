// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "spdx_expression.hpp"

#include "ascii.hpp"
#include "spdx_licenses.hpp"

#include <algorithm>
#include <cstddef>
#include <string_view>
#include <vector>

namespace arcana::data
{

namespace
{

constexpr std::string_view license_ref_prefix{"LicenseRef-"};
constexpr std::string_view document_ref_prefix{"DocumentRef-"};

bool is_spdx_operator(std::string_view token) noexcept
{
    return token == "AND" || token == "OR" || token == "WITH";
}

// idstring = 1*(ALPHA / DIGIT / "-" / ".")
bool is_idstring(std::string_view token) noexcept
{
    return !token.empty() &&
           std::ranges::all_of(token, [](char c) { return is_alnum(c) || c == '-' || c == '.'; });
}

// license-ref = ["DocumentRef-" idstring ":"] "LicenseRef-" idstring
//
// Well-formed by construction: a LicenseRef names terms with no SPDX identifier
// and is never looked up on the list.
bool is_license_ref(std::string_view token) noexcept
{
    if (token.starts_with(document_ref_prefix))
    {
        auto const colon = token.find(':');
        if (colon == std::string_view::npos)
            return false;

        auto const document =
            token.substr(document_ref_prefix.size(), colon - document_ref_prefix.size());
        if (!is_idstring(document))
            return false;

        token = token.substr(colon + 1);
    }

    if (!token.starts_with(license_ref_prefix))
        return false;

    return is_idstring(token.substr(license_ref_prefix.size()));
}

std::vector<std::string_view> tokenize_spdx(std::string_view text)
{
    std::vector<std::string_view> tokens;
    std::size_t pos = 0;

    auto const is_space = [](char c) { return static_cast<unsigned char>(c) <= ' '; };
    auto const is_break = [is_space](char c) { return c == '(' || c == ')' || is_space(c); };

    while (pos < text.size())
    {
        if (is_space(text[pos]))
        {
            ++pos;
            continue;
        }

        std::size_t const first = pos;
        if (text[pos] == '(' || text[pos] == ')')
            ++pos;
        else
            while (pos < text.size() && !is_break(text[pos])) ++pos;

        tokens.push_back(text.substr(first, pos - first));
    }

    return tokens;
}

// Recursive descent over the SPDX license expression grammar, SPDX 2.3 Annex D.
//
// Precedence, loosest first: OR, AND, WITH. The "+" of an "or later" identifier
// binds tightest of all and is part of the token.
class spdx_parser
{
  public:
    explicit spdx_parser(std::string_view text) : tokens_(tokenize_spdx(text)) {}

    [[nodiscard]] spdx_expression_check run()
    {
        if (tokens_.empty() || !parse_or() || at_ != tokens_.size())
            return {.well_formed = false, .unknown_identifier = {}};

        return {.well_formed = true, .unknown_identifier = unknown_};
    }

  private:
    [[nodiscard]] std::string_view peek() const noexcept
    {
        return at_ < tokens_.size() ? tokens_[at_] : std::string_view{};
    }

    void note_unknown(std::string_view id) noexcept
    {
        if (unknown_.empty())
            unknown_ = id;
    }

    bool parse_or()
    {
        if (!parse_and())
            return false;

        while (peek() == "OR")
        {
            ++at_;
            if (!parse_and())
                return false;
        }

        return true;
    }

    bool parse_and()
    {
        if (!parse_with())
            return false;

        while (peek() == "AND")
        {
            ++at_;
            if (!parse_with())
                return false;
        }

        return true;
    }

    bool parse_with()
    {
        if (!parse_primary())
            return false;

        if (peek() != "WITH")
            return true;

        ++at_;
        auto const exception = peek();
        if (!is_idstring(exception) || is_spdx_operator(exception))
            return false;

        ++at_;
        if (!is_spdx_exception_id(exception))
            note_unknown(exception);

        return true;
    }

    bool parse_primary()
    {
        if (peek() != "(")
            return parse_simple();

        ++at_;
        if (!parse_or() || peek() != ")")
            return false;

        ++at_;
        return true;
    }

    bool parse_simple()
    {
        auto const token = peek();
        if (token.empty() || token == ")" || is_spdx_operator(token))
            return false;

        ++at_;

        if (token.starts_with(license_ref_prefix) || token.starts_with(document_ref_prefix))
            return is_license_ref(token);

        // license-id "+" is the "or later" form; the plus is not an operator.
        auto id = token;
        if (id.ends_with('+'))
            id.remove_suffix(1);

        if (!is_idstring(id))
            return false;

        if (!is_spdx_license_id(id))
            note_unknown(id);

        return true;
    }

    std::vector<std::string_view> tokens_;
    std::size_t at_ = 0;
    std::string_view unknown_;
};

}  // namespace

spdx_expression_check check_spdx_expression(std::string_view s)
{
    return spdx_parser{s}.run();
}

}  // namespace arcana::data
