#pragma once
#include <limits>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace bpatch
{
/// <summary>
///  class provides representation of a binary lexeme
/// </summary>
/// 
class AbstractBinaryLexeme final
{
    AbstractBinaryLexeme(const AbstractBinaryLexeme&) = delete;
    AbstractBinaryLexeme(AbstractBinaryLexeme&&) = delete;
    AbstractBinaryLexeme& operator = (const AbstractBinaryLexeme&) = delete;
    AbstractBinaryLexeme& operator = (AbstractBinaryLexeme&&) = delete;

public:
    ~AbstractBinaryLexeme() = default;

    /// <summary>
    ///   replace inner part of data with new AbstractBinaryLexeme
    /// </summary>
    /// <param name="at">position of replacement</param>
    /// <param name="alength">size of data to replace</param>
    /// <param name="awith">data to set instead of replaced </param>
    /// <returns>span for access Lexeme</returns>
    const std::span<const char>& Replace(size_t at, size_t alength, std::unique_ptr<AbstractBinaryLexeme>& awith);

    /// <summary>
    ///   provides the way of access the lexeme data for reading
    /// </summary>
    /// <returns>span for access Lexeme</returns>
    const std::span<const char>& access() const {return access_;};

protected:
    ///@brief our data is here
    std::vector<char> dataSequence_;

    /// here we hold access to provide
    std::span<const char> access_;


public:
// Way of construction:
    /// @brief moves vector to BinaryLexeme and holds data inside
    static std::unique_ptr<AbstractBinaryLexeme> LexemeFromVector(std::vector<char>&& asrcVec);

    /// @brief creates a copy of data from a string view
    static std::unique_ptr<AbstractBinaryLexeme> LexemeFromStringView(const std::string_view asrc);
    /// @brief creates a copy of data from a span
    static std::unique_ptr<AbstractBinaryLexeme> LexemeFromSpan(const std::span<const char> asrc);

    /// @brief holds copy of source binary lexeme inside
    /// up to the end or length provided as second parameter
    static std::unique_ptr<AbstractBinaryLexeme> LexemeFromLexeme(
        std::unique_ptr<AbstractBinaryLexeme>& source,
        const size_t length = std::numeric_limits<size_t>::max());

protected:
    ///@brief An AbstractBinaryLexeme can only be created from an existing vector,
    ///  which will subsequently be held internally.
    AbstractBinaryLexeme(std::vector<char>&& asrcVec);

}; // class AbstractBinaryLexeme


};// namespace bpatch
