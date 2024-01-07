#pragma once
#include <string_view>

namespace bpatch
{
/// @brief Class for read console parameters of bpatch.
///  Provides methods to get data
class ConsoleParametersReader final
{
public:
    ConsoleParametersReader();

    /// <summary>
    ///  reads console parameters for bProcessor
    /// </summary>
    /// <param name="argc">count of parameters</param>
    /// <param name="argv">parameters provided to application</param>
    /// <returns>
    ///  true - if mandatary parameters have been provided;
    ///  false - if we get request for help at any parameter or
    ///    not all necessary parameters have been provided;
    /// </returns>
    bool ReadConsoleParameters(int argc, char* argv[]);

    /// <summary> returns manual texts for printing </summary>
    /// <returns> pointer to the manual text </returns>
    const char* Manual() const noexcept {return manualText;};

    /// <summary> returns Source file name </summary>
    /// <returns> pointer to the source file name</returns>
    std::string_view Source() const noexcept { return sData.source; };

    /// <summary> returns Actions file name </summary>
    /// <returns> pointer to the actions file name</returns>
    std::string_view Actions() const noexcept { return sData.actions; };

    /// <summary> returns Target file name </summary>
    /// <returns> pointer to the Target file name</returns>
    std::string_view Target() const noexcept { return sData.target; };

    /// <summary> returns true if we need to overwrite Target file </summary>
    /// <returns> returns true if we need to overwrite Target file </returns>
    bool Overwrite() const noexcept { return sData.forceOverwrite; };

// members
protected:
    const char * const manualText;
    struct ProcessorData
    {
        std::string_view source;
        std::string_view target;
        std::string_view actions;
        bool forceOverwrite = false;
    } sData;
};

}; // namespace bpatch
