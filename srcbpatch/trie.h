#pragma once

#include <string_view>
#include <deque>

/// @brief A node of prefix Trie, each node contains current target (if exist) and child nodes in children.
/// And doesn't contain a char of current node
class TrieNode {
    public:
        /// a list of child nodes of current node.
        std::unordered_map<char, std::reference_wrapper<TrieNode>> children;
        /// target of current node (if there are node target at this node in our lexemePairs --> target is std::nullopt)
        std::optional<std::string_view> target;
};

/// @brief Prefix tree class to speed up UniformLexemeReplacer::DoReplace
class Trie final{
public:
    /// <summary>
    ///  Adds a new key-value pair in prefix tree
    /// </summary>
    /// <param name="key"> source lexeme </param>
    /// <param name="value"> target lexeme </param>
    void insert(std::string_view key, std::string_view value);

    /// <summary>
    ///  Looking for a full match in prefix tree
    /// </summary>
    /// <param name="cachedData"> key to find </param>
    /// <returns>string_view: target, bool: FullMatch</returns>
    [[nodiscard]] std::pair<std::string_view, bool> searchFullMatch(std::span<const char> cachedData) const;

private:
    /// a root node, doesn't contains target value
    TrieNode root;
    /// holds all the nodes of Trie. While default Trie uses pointers, we are going to use reference_wrapper to this deque elements
    std::deque<TrieNode> nodes;
};