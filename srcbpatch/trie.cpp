#include "trie.h"

void Trie::insert(std::string_view key, std::string_view value)
{
    TrieNode *node = &root;
    for (char character : key)
    {
        auto [it, inserted] = node->children.try_emplace(character, std::make_unique<TrieNode>());
        node = it->second.get();
    }
    node->target = value;
}

[[nodiscard]] std::pair<std::string_view, bool> Trie::searchFullMatch(const std::string_view& cachedData)
{
    TrieNode *node = &root;
    for (char c: cachedData)
    {
        auto res = node->children.find(c);
        if (res == node->children.end())
        {
            return std::make_pair(std::string_view(), false);
        }
        node = res->second.get();
    }

    // full match
    if (node->target)
    {
        return std::make_pair(node->target.value(), true);
    }

    return std::make_pair(std::string_view(), false);
}
