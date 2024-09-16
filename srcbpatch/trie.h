#pragma once

#include <string_view>

#ifdef _WIN32
#pragma warning(disable : 4127)
#pragma warning(disable : 4324)
#endif
#include <tsl/robin_map.h>
#ifdef _WIN32
#pragma warning(default : 4127)
#pragma warning(disable : 4324)
#endif

class TrieNode {
    public:
        tsl::robin_map<char, std::unique_ptr<TrieNode>> children;
        std::optional<std::string_view> target;
    };

/// @brief Frefix tree class to speed up UniformLexemeReplacer::DoReplace
class Trie {
public:
    /// <summary>
    ///  Adds a new key-value pair in prefix tree
    /// </summary>
    /// <param name="key"> source lexemme </param>
    /// <param name="value"> target lexemme </param>
    void insert(std::string_view key, std::string_view value);

    /// <summary>
    ///  Looking for a full match in prefix tree
    /// </summary>
    /// <param name="cachedData"> key to find </param>
    /// <returns>string_view: target, bool: FullMatch</returns>
    [[nodiscard]] std::pair<std::string_view, bool> searchFullMatch(const std::string_view& cachedData);

private:
    TrieNode root;
};