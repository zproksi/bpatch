#include "stdafx.h"
#include "consoleparametersreader.h"
#include "bpatchfolders.h"

namespace
{
    constexpr const char* const manualText =
R"(bpatch -s SOURCE -a ACTIONS [-d/-w DEST] [-fa AFN] [-fb BFFN]
  -s SOURCE       SOURCE file data will be changed (as binary data)
  -a ACTIONS      according rules picked from ACTIONS file
  -d DEST         result data will be saved into DEST file if this
                  parameter is in command line. SOURCE file will be
                  changed if no -d/-w parameter provided
  -w DEST         use -w to force override result file
     all parameters are case insencitive (-w is the same as -W)
     no wild characters allowed
  -h, ?,
  --help, /h      for this help

  The ACTIONS file will be searched for in the current folder first,
  then in the subfolder 'bpatch_a' located near the 'bpatch'
  executable. Binary files from the ACTIONS files will first be
  searched for in the current folder, then in the 'bpatch_b'
  subfolder located near the 'bpatch' executable. It is possible
  to change these folders. Use:
  -fa             for Actions Folder Name: AFN
  -fb             for Binary Files Folder Name: BFFN

  ACTIONS file sample:
          proven for ASCII symbols, json format
          note: control characters in text cannot be unicode!
                there is no \uXXXX support
-------------------------------------------
{
  "dictionary": {
    "decimal": {
      "pattern name": [13, 10],
      "another binary": [13]
    },
    "hexadecimal": {
      "hex pattern name": ["0A", "09", "03"],
      "hex02": ["00", "Fe", "3A"]
    },
    "text": {
      "text name": "textual value",
      "Hi": "Hello World!"
    },
    "file": {
      "fileX": "filename.bin"
    },
    "composite": [{
        "composite 001": ["pattern name", "text name",
                          "hex pattern name"]
      },
      {
        "complexOne": ["hex02", "fileX", "Hi",
                       "composite 001"]
      }
    ]
  },
  "todo": [
    {
      "replace": {
        "pattern name": "hex pattern name",
        "hex02": "complexOne"
      }
    },
    {
      "replace": {
        "text name": "fileX"
      }
    }
  ]
}
-------------------------------------------
)";

    constexpr const char* GetManual()
    {
        return manualText;
    };


    constexpr std::string_view help_literals[] = 
    {
        "?"
        , "/h"
        , "--help"
        , "-h"
        , "--man"
    };


    /// <summary>
    ///   compare characters case insensitive
    /// </summary>
    /// <param name="a">first character</param>
    /// <param name="b">second character</param>
    /// <returns>true if equal, false otherwise</returns>
    bool ichar_equals(const char a, const char b)
    {
        return std::tolower(static_cast<int>(a)) ==
            std::tolower(static_cast<int>(b));
    }

};

namespace bpatch
{

ConsoleParametersReader::ConsoleParametersReader()
    : manualText(GetManual())
{
}

bool ConsoleParametersReader::ReadConsoleParameters(int argc, char* argv[])
{
    // create parameters vector
    std::vector<std::string_view> params(static_cast<size_t>(argc));
    std::transform(argv, argv + argc, params.begin(),
        [](const char* src) noexcept -> auto
        {
            return std::move(std::string_view(src, strlen(src)));
        }
    );

    // check for help request
    if (std::ranges::any_of(params, [](const std::string_view& sv) noexcept -> bool
        { // if any of parameters is string literal from help request patterns
            return std::ranges::any_of(help_literals, [&sv](const std::string_view& sv2) noexcept -> bool
            {
                return std::ranges::equal(sv, sv2, ichar_equals);
            });
        })
       )
    {
        return false; // request for help found
    }

    // read parameters
    auto readParameter = [&params](std::string_view paramAbbr, std::string_view& data) noexcept -> bool
    {
        auto inputMark = [&paramAbbr](const std::string_view& mark)
        {
            return std::ranges::equal(mark, paramAbbr, ichar_equals);
        };
        auto it = std::ranges::find_if(params, inputMark);
        if (it != params.end() && ++it != params.end())
        {
            data = *it;
            return true;
        }
        return false;
    };

    std::string_view value;
    if (readParameter("-fa", value)) // set the folder of the action files if provided
    {
        FolderActions() = value;
    }

    if (readParameter("-fb", value)) // set the folder of the binary data files if provided
    {
        FolderBinaryPatterns() = value;
    }

    readParameter("-s", sData.source);
    if (sData.forceOverwrite = readParameter("-w", sData.target); !sData.forceOverwrite)
    {
        if (!readParameter("-d", sData.target))
        {
            sData.target = sData.source; // inplace processing chosen
        }
    }


    // return true only if we have valid source and actions files
    return sData.source.size() > 0 && readParameter("-a", sData.actions);
}

}; // namespace bpatch
