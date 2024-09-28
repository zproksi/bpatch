#include "trie.h"

void Trie::insert(std::string_view key, std::string_view value)
{
    std::reference_wrapper<TrieNode> node = root;
    for (char character : key)
    {
        auto [it, inserted] = node.get().children.emplace(character, nodes.emplace_back());
        node = it->second.get();
    }
    node.get().target = value;
}

[[nodiscard]] std::pair<std::string_view, bool> Trie::searchFullMatch(std::span<const char> cachedData) const
{
    std::reference_wrapper<const TrieNode> node = root;
    for (char c : cachedData)
    {
        auto res = node.get().children.find(c);
        if (res == node.get().children.end())
        {
            return std::make_pair(std::string_view(), false);
        }
        node = res->second.get();
    }

    // full match
    if (node.get().target)
    {
        return std::make_pair(node.get().target.value(), true);
    }

    return std::make_pair(std::string_view(), false);
}
