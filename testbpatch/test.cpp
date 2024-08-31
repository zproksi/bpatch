#include "pch.h"
#include <gtest/gtest.h>
#include <fstream>


#if defined(__linux__) || ((defined(__APPLE__) && defined(__MACH__)))
#include <unistd.h>
#else
#include <tchar.h>
#include <windows.h>
#endif


namespace
{
    const std::string_view everythingSV("*");
    using bpatch::emptySV;
};

/// <summary>
///  same mask & deifferent folders return true
/// </summary>
TEST(WildCharacters, ProcessingValidTrue)
{
    using namespace wildcharacters;
    using namespace std;
    struct
    {
        string_view src;
        string_view dst;
    }
    okMask[] =
    {
#if defined(__linux__) || ((defined(__APPLE__) && defined(__MACH__)))
        {R"(/Users/Bobby/kon?e?.txt)", R"(./kon?e?.txt)"}
        , {R"(../Bobby/kon*e*.*)", R"(../kon*e*.*)"}
        , {R"(kon*e*.*)", R"(./kon*e*.*)"}
        , {R"(?e_.*)", R"(brouny/?e_.*)"}
        , {R"(*.*)", R"(anything/*.*)"}
        , {R"(*.???)", R"(../triple/*.???)"}
        , {R"(.*)", R"(specific/.*)"}
        , {R"(/*.*)", R"(uplevel/*.*)"}
        , {R"(./../*.???)", R"(./deeper/*.???)"}
        , {R"(*.)", R"(noext/*.)"}
        , {R"(*)", R"(everything/*)"}
#else
        {R"(c:\Users\Bobby\kon?e?.txt)", R"(here\kon?e?.txt)"}
        , {R"(..\Bobby\kon*e*.*)", R"(..\kon*e*.*)"}
        , {R"(kon*e*.*)", R"(.\kon*e*.*)"}
        , {R"(?e_.*)", R"(brouny\?e_.*)"}
        , {R"(*.*)", R"(anything\*.*)"}
        , {R"(*.???)", R"(..\triple\*.???)"}
        , {R"(*.)", R"(specific\*.)"}
        , {R"(\*.*)", R"(uplevel\*.*)"}
        , {R"(.\..\*.???)", R"(.\deeper\*.???)"}
        , {R"(c:*.)", R"(wellthisalso\*.)"}
        , {R"(*)", R"(everything\*)"}
#endif
    };

    for (auto&& item: okMask)
    {
        LookUp lup;
        EXPECT_TRUE(lup.RegisterSourceAndDestination(item.src, item.dst));
    }
}

/// <summary>
///  same folder and mask - return false
/// </summary>
TEST(WildCharacters, ProcessingValidFalse)
{
    using namespace wildcharacters;
    using namespace std;
    struct
    {
        string_view src;
        string_view dst;
    }
    okMask[] =
    {
#if defined(__linux__) || ((defined(__APPLE__) && defined(__MACH__)))
        {R"(/Users/Bobby/kon?e?.txt)", R"(/Users/Bobby/kon?e?.txt)"}
        , {R"(./../*.???)", R"(./../*.???)"}
        , {R"(*.*)", R"(*.*)"}
#else
        {R"(c:\Users\Bobby\kon?e?.txt)", R"(c:\Users\Bobby\kon?e?.txt)"}
        , {R"(.\..\*.???)", R"(.\..\*.???)"}
        , {R"(*.*)", R"(*.*)"}
#endif
    };

    for (auto&& item : okMask)
    {
        LookUp lup;
        EXPECT_FALSE(lup.RegisterSourceAndDestination(item.src, item.dst));
    }
}


/// <summary>
///  must throw if wild characters in path
///  must throw if mask mismatch
/// </summary>
TEST(WildCharacters, ProcessingInvalid)
{
    using namespace wildcharacters;
    using namespace std;
    struct
    {
        string_view src;
        string_view dst;
    }
    badMask[] =
    {
#if defined(__linux__) || ((defined(__APPLE__) && defined(__MACH__)))
        {R"(/Users/Bo?by/kon?e?.txt)", emptySV}
        , {R"(../Bo*by/kon*e*.*)", emptySV}
        , {R"(*/kon*e*.*)", emptySV}
        , {R"(?/e_.*)", emptySV}
        , {R"(./*/*.???)", emptySV}
        , {R"(./?/*.)", emptySV}
        , {everythingSV, R"(/Users/Bo?by/kon?e?.txt)"}
        , {everythingSV, R"(../Bo*by/kon*e*.*)"}
        , {everythingSV, R"(*/kon*e*.*)"}
        , {everythingSV, R"(?/e_.*)"}
        , {everythingSV, R"(./*/*.???)"}
        , {everythingSV, R"(./?/*.)"}
        , {R"(./?/.*)", R"(./?/*.)"}
        , {R"(./a/???)", R"(./b/*)"}
#else
        {R"(c:\Users\Bo?by\kon?e?.txt)", emptySV}
        , {R"(..\Bo*by\kon*e*.*)", emptySV}
        , {R"(*\kon*e*.*)", emptySV}
        , {R"(?\e_.*)", emptySV}
        , {R"(.\*\*.???)", emptySV}
        , {R"(.\?\*.)", emptySV}
        , {everythingSV, R"(c:\Users\Bo?by\kon?e?.txt)"}
        , {everythingSV, R"(..\Bo*by\kon*e*.*)"}
        , {everythingSV, R"(*\kon*e*.*)"}
        , {everythingSV, R"(?\e_.*)"}
        , {everythingSV, R"(.\*\*.???)"}
        , {everythingSV, R"(.\?\*.)"}
        , {R"(.\?\.*)", R"(.\?\*.)"}
        , {R"(.\a\???)", R"(.\b\*)"}
#endif
    };

    for (auto&& item : badMask)
    {
        LookUp lup;
        EXPECT_THROW(lup.RegisterSourceAndDestination(item.src, item.dst), std::logic_error);
    }
}

/// <summary>
///  Files_For_Test class creates and then removes set of files and folders
///  for performing search by mask during unit tests execution
/// </summary>
class Files_For_Test final
{
    std::filesystem::path base_folder;
    std::filesystem::path subfolder_k;
    std::filesystem::path subfolder_keb;
    const std::string filenames[8] = {"a", "ae", "aeb", "aebc", "a.txt", "ae.txt", "aeb.txt", "aebc.txt"};

    Files_For_Test(const Files_For_Test&) = delete;
    Files_For_Test& operator =(const Files_For_Test&) = delete;
    Files_For_Test(Files_For_Test&&) noexcept = delete;
    Files_For_Test& operator =(Files_For_Test&&) noexcept = delete;
public:
    Files_For_Test(const std::filesystem::path& test_folder)
        : base_folder(test_folder / "fortesting")
        , subfolder_k(test_folder / "fortesting" / "k")
        , subfolder_keb(test_folder / "fortesting" / "keb")
    {
        // Create the folder "fortesting"
        std::filesystem::create_directory(base_folder);
        // Create subfolders in "fortesting"
        std::filesystem::create_directory(subfolder_k);
        std::filesystem::create_directory(subfolder_keb);

        // Create files in "fortesting"
        for (const auto& filename : filenames)
        {
            std::filesystem::path filepath = base_folder / filename;
            std::ofstream outfile(filepath);
            outfile << 'A';
            outfile.close();
        }
    }

    std::filesystem::path FolderWithFiles() const noexcept { return base_folder; }

    ~Files_For_Test()
    {
        // Remove all files
        for (const auto& filename : filenames) {
            std::filesystem::path filepath = base_folder / filename;
            std::filesystem::remove(filepath);
        }

        // Remove subdirectories
        std::filesystem::remove(subfolder_k);
        std::filesystem::remove(subfolder_keb);
        // Finally, remove the "fortesting" folder
        std::filesystem::remove(base_folder);
    }
}; // Files_For_Test

/// <summary>
///  must throw if wild characters in path
///  must throw if mask mismatch
/// </summary>
TEST(WildCharacters, SearchingTests)
{
    // get full path for executable
#if defined(__linux__) || ((defined(__APPLE__) && defined(__MACH__)))
    char pathBuffer[PATH_MAX] = {0};
    [[maybe_unused]] auto result = readlink(R"(/proc/self/exe)", pathBuffer, PATH_MAX);
#else
    TCHAR pathBuffer[MAX_PATH] = {0};
    GetModuleFileName(nullptr, pathBuffer, MAX_PATH);
#endif

    using namespace wildcharacters;
    using namespace std;

    filesystem::path pathTestFolder(pathBuffer);
    pathTestFolder = pathTestFolder.parent_path();

    // setup creation and deletion of files
    Files_For_Test testFilesStructure(pathTestFolder);
    pathTestFolder = testFilesStructure.FolderWithFiles();

    struct
    {
        string_view src;
        size_t nMatches;
    }
    arrToCheck[] =
    {
        {"*.*", 4}
        , {"*", 8}
        , {"*.", 0}
        , {".*", 0}
        , {"?", 1}
        , {"?.", 0}
        , {"?.*", 1}
        , {"a?", 1}
        , {"a?*.*", 3}
        , {"a?b", 1}
        , {"a?b*", 4}
        , {"?eb*", 4}
        , {"?eb*.*", 2}
        , {"*e*c*", 2}
        , {"*e*c*.*", 1}
        , {"ae.txt", 1}
    };

    /// test every mask for sourcce+target combinations
    for (auto&& item : arrToCheck)
    {
        for (size_t k = 0; k < 4; ++k)
        {
            filesystem::path sourceMask = pathTestFolder / item.src;
            filesystem::path destinationMask;
            switch (k % 3)
            {
            case 0: destinationMask = pathTestFolder / "k";
                break;
            case 1: destinationMask = pathTestFolder / "k" / item.src;
                break;
            case 2: destinationMask = pathTestFolder / item.src;
                break;
            };

            LookUp lup;

            lup.RegisterSourceAndDestination(sourceMask.string(), destinationMask.string());

            size_t count = 0;
            string srcFilename;
            string dstFilename;
            while (lup.NextFilenamesPair(srcFilename, dstFilename))
            {
                ++count;
            }
            EXPECT_EQ(count, item.nMatches);
        }
    }

};

/// <summary>
///   put data from binary lexeme to vector
/// </summary>
/// <param name="lexeme">lexeme to read</param>
/// <returns>vector with duplicate of the initial data</returns>
std::vector<char> getDataInsideLexeme(const std::unique_ptr<bpatch::AbstractBinaryLexeme>& lexeme)
{
    auto& acc = lexeme->access();

    std::vector<char> ret(acc.data(), acc.data() + acc.size());
    return ret;
}


TEST(FlexibleCache, LinkedList)
{
    using namespace bpatch;
    using namespace std;
    {
        FlexibleCache cache;
        // empty
        EXPECT_FALSE(cache.RootChunkFull());

        std::vector<char> xdata(SZBUFF_FC - 1);

        cache.Accumulate({xdata.data(), xdata.size()});
        // not enough 1 byte
        EXPECT_FALSE(cache.RootChunkFull());

        xdata.resize(SZBUFF_FC * 3);
        // add twice size
        cache.Accumulate({xdata.data(), xdata.size()});
        EXPECT_TRUE(cache.RootChunkFull());

        // expect 3 full chain pieses and +1 with last byte empty
        cache.Accumulate('k');

        std::unique_ptr<FlexibleCache::Chunk> achunk;
        int n = 4;
        while (cache.RootChunkFull())
        {
            cache.Next(achunk);
            --n;
        }
        EXPECT_EQ(0, n);
        cache.Next(achunk);
        EXPECT_EQ(0, achunk->accumulated);
        // achunk valid all the time
        cache.Next(achunk);
        EXPECT_EQ(0, achunk->accumulated);

        cache.Accumulate('O');
        cache.Accumulate('k');
        cache.Next(achunk);
        EXPECT_EQ('O', achunk->data[0]);
        EXPECT_EQ('k', achunk->data[1]);
        EXPECT_EQ(2, achunk->accumulated);
    }
}

/// <summary>
///   replace logic for the lexemes
///     life time of the data for lexemes should be the same or longer
/// </summary>
TEST(BinaryLexeme, Replaces)
{
    using namespace bpatch;
    auto svLexeme001 = AbstractBinaryLexeme::LexemeFromStringView("Hello World");
    auto svLexemeEmpty = AbstractBinaryLexeme::LexemeFromStringView("");

    /// replace middle
    auto svLexeme002 = AbstractBinaryLexeme::LexemeFromVector(std::vector<char>{'_', '_', 'C'});
    svLexeme001->Replace(4, 3, svLexeme002);
    svLexeme001->Replace(0,0, svLexemeEmpty); // empty lexeme is also ok
    EXPECT_TRUE(std::ranges::equal(getDataInsideLexeme(svLexeme001), std::string_view("Hell__Corld")));

    /// insert at the beginning
    auto svLexeme003 = AbstractBinaryLexeme::LexemeFromStringView("Be+");
    svLexeme001->Replace(0, 0, svLexeme003);
    svLexeme001->Replace(14, 0, svLexemeEmpty); // empty lexeme is also ok
    EXPECT_TRUE(std::ranges::equal(getDataInsideLexeme(svLexeme001), std::string_view("Be+Hell__Corld")));

    /// insert at the end
    auto svLexeme004 = AbstractBinaryLexeme::LexemeFromStringView("enD");
    svLexeme001->Replace(14, 0, svLexeme004);
    svLexeme001->Replace(14, 0, svLexemeEmpty); // empty lexeme is also ok
    EXPECT_TRUE(std::ranges::equal(getDataInsideLexeme(svLexeme001), std::string_view("Be+Hell__CorldenD")));

    /// insert in the middle
    auto svLexeme005 = AbstractBinaryLexeme::LexemeFromStringView("|V");
    svLexeme001->Replace(8, 1, svLexeme005);
    EXPECT_TRUE(std::ranges::equal(getDataInsideLexeme(svLexeme001), std::string_view("Be+Hell_|VCorldenD")));

    /// insert & overlap a lot
    auto svLexeme006 = AbstractBinaryLexeme::LexemeFromStringView("_QWERTY_");
    svLexeme001->Replace(1, 16, svLexeme006);
    EXPECT_TRUE(std::ranges::equal(getDataInsideLexeme(svLexeme001), std::string_view("B_QWERTY_D")));

    /// replace at the beginning of the lexeme
    auto svLexeme007 = AbstractBinaryLexeme::LexemeFromStringView("an ");
    svLexeme001->Replace(1, 4, svLexeme007);
    EXPECT_TRUE(std::ranges::equal(getDataInsideLexeme(svLexeme001), std::string_view("Ban RTY_D")));

    /// replace at the end of the lexeme
    auto svLexeme008 = AbstractBinaryLexeme::LexemeFromStringView("ka ");
    svLexeme001->Replace(6, 2, svLexeme008);
    EXPECT_TRUE(std::ranges::equal(getDataInsideLexeme(svLexeme001), std::string_view("Ban RTka D")));
}


/// <summary>
///   valid json data should not be the problem untill we have outher array
/// </summary>
TEST(JSON, JsonParserValid)
{
    // case with/without white spaces around json
    std::string_view jsonValid[] = {
        R"(
           {"dictionary":{"decimal":{"pattern name":[13,10],"another binary":[13]},"hexadecimal":{"hex pattern name":["0A","09","03"],"hex02":["00","Fe","3A"]},"text":{"text name":"textual value","Hi":"Hello World!"},"file":{"fileX":"filename.bin"},"composite":[{"composite 001":["pattern name","text name","hex pattern name"]},{"complexOne":["hex02","fileX","Hi","composite 001"]}]},"todo":[{"replace":{"pattern name":"hex pattern name"}},{"replace":{"text name":"fileX"}}]}
        )"
        , R"({"dictionary":{"decimal":{"pattern name":[13,10],"another binary":[13]},"hexadecimal":{"hex pattern name":["0A","09","03"],"hex02":["00","Fe","3A"]},"text":{"text name":"textual value","Hi":"Hello World!"},"file":{"fileX":"filename.bin"},"composite":[{"composite 001":["pattern name","text name","hex pattern name"]},{"complexOne":["hex02","fileX","Hi","composite 001"]}]},"todo":[{"replace":{"pattern name":"hex pattern name"}},{"replace":{"text name":"fileX"}}]})"
        , R"({})"
        , R"({"a":true, "b":null, "c":false, "arr":[[],["kk",null],[{},{}],[{}]]})"
        , R"({"a":{}, "b":[], "c":"\""})"
    };
    for (auto& sv : jsonValid)
    {
        std::vector<char> vec(std::begin(sv), std::end(sv));
        std::string_view dta(vec.data(), vec.size());

        using namespace bpatch;
        EXPECT_NO_THROW(TJSONObject::CreateJSONObject(dta, nullptr));
    }

}


/// <summary>
///   invalid json data should throws only std::logic_error exceptions
/// </summary>
TEST(JSON, JsonParserInvalid)
{
    std::string_view jsonInvalid[] = {
        R"({)"
        , R"([])" // it is case when we expecting only object as outer data, but the json here is valid
        , R"({}[])"
        , R"({"a": "b", "c})"
        , R"({"a": "b", "c"})"
        , R"({"a": "b", "c",})"
        , R"({"a": "b", "c":})"
        , R"({"a": "b", "c":[[]})"
        , R"({"a": "b", "c":"})"
        , R"({"a": "b", "c":"[})"
        , R"({"a": {})"
        , R"({"a": tRUE})"
        , R"({"a": nuLl})"
        , R"({"a": fAlse})"
        , R"({"a": "b", "c":[{]})"
        , R"({"a": "b", "c":{]})"
        , R"({[]})"
        , R"({"e": "a", "z":"q", ["__"] })"
        , R"({"arr":[[],["kk":{}]})"
    };
    for (auto& sv : jsonInvalid)
    {
        std::vector<char> vec(std::begin(sv), std::end(sv));
        std::string_view dta(vec.data(), vec.size());

        using namespace bpatch;
        EXPECT_THROW(TJSONObject::CreateJSONObject(dta, nullptr), std::logic_error);
    }
}


/// <summary>
///   actions processing throws std::runtime_error only
/// </summary>
TEST(ACollection, ActionsProcessingInvalid)
{
    std::string_view jsonActionsInvalid[] = {
        // 256 is not valid for us
        R"(
            {"dictionary":{"decimal":{"pattern name":[256]}},
                           "todo":[{"replace":{"pattern name":"pattern name"}}]}
        )"
        // -1 is not valid for us
        , R"(
            {"dictionary":{"decimal":{"pattern name":[-1]}},
             "todo":[{"replace":{"pattern name":"pattern name"}}]}
        )"
        // more than 255 is not valid for us
        , R"(
            {"dictionary":{"hexadecimal":{"pattern name":["100"]}},
             "todo":[{"replace":{"pattern name":"pattern name"}}]}
        )"
        // nonexistent pattern name is not valid 1
        , R"(
            {"dictionary":{"hexadecimal":{"pattern name":["1a"]}},
             "todo":[{"replace":{"strange name":"pattern name"}}]}
        )"
        // nonexistent pattern name is not valid 2
        , R"(
            {"dictionary":{"hexadecimal":{"pattern name":["1a"]}},
             "todo":[{"replace":{"pattern name":"strange name"}}]}
        )"
        // nonexistent pattern name is not valid 3
        , R"(
             {"dictionary":{"text":{"text1":"Textual value","text2":"another text"},
                            "composite" : [{"composite 001": ["text1","complexOne"] },{"complexOne": ["text2","text1"] }] },
              "todo" : [{"replace":{"composite 001":"complexOne"}}] }
        )"
        // nonexistent pattern name is not valid 4
        , R"(
             {"dictionary":{"text":{"text1":"Textual value","text2":"another text"},
                            "composite" : [{"composite 001": ["text1","text2"] },{"complexOne": ["text3","text1"] }] },
              "todo" : [{"replace":{"composite 001":"complexOne"}}] }
        )"
        // duplicate pattern name
        , R"(
             {"dictionary":{"text":{"text1":"Textual value"},"hexadecimal":{"text1":["1a"]}},
              "todo" : [{"replace":{"text1":"text1"}}] }
        )"
        // duplicate pattern name
        , R"(
             {"dictionary":{"text":{"text1":"Textual value"},"hexadecimal":{"text2":["1a"]},
                            "composite" : [{"composite 001": ["text1","text2"] },{"text2": ["text1","composite 001"] }]
                           },
              "todo" : [{"replace":{"text1":"text1"}}] }
        )"
        // no data at all
        , R"({})"
        // no dictionary
        , R"({"todo" : [{"replace":{"text1":"text1"}}] })"
        // no todo
        , R"(
             {"dictionary":{"text":{"text1":"Textual value"},"hexadecimal":{"text2":["1a"]},
                            "composite" : [{"composite 001": ["text1","text2"] },{"text2": ["text1","composite 001"] }]
                           }, "todo":[]}
        )"
    };
    for (auto& sv : jsonActionsInvalid)
    {
        std::vector<char> vec(std::begin(sv), std::end(sv));

        using namespace bpatch;
        EXPECT_THROW(ActionsCollection(move(vec)), std::runtime_error);
    }

    {
        // nothing in replace source
        std::string_view sv = R"({"dictionary":{"text":{"text1":""}},""todo" : [{"replace":{"text1":"text1"}}] })";
        std::vector<char> vec(std::begin(sv), std::end(sv));

        using namespace bpatch;
        EXPECT_THROW(ActionsCollection(move(vec)), std::logic_error);
    }
}


namespace // for emulate writer
{
    using namespace bpatch;
    using namespace std;
    class TestWriter : public Writer
    {
    public:
        vector<char> data_accumulator;


        virtual size_t WriteCharacter(const char toProcess, const bool aEod) override
        {
            if (!aEod)
            {
                data_accumulator.emplace_back(toProcess);
            }
            return aEod ? 0 : 1;
        };


        virtual size_t Written() const noexcept override
        {
            return data_accumulator.size();
        }
    };

    struct TestData
    {
        std::string_view jsonData;
        std::string_view testData;
        std::string_view resultData;
    };

}; // namespace


/// <summary>
///   actions processing - check input and output
/// </summary>
///
TEST(ACollection, ActionsProcessing)
{
    TestData arrTests[] = {
        { // test with sequential replacement 112 -> 22 -> 3
        R"(
            {"dictionary":{"text":{"v1":"11", "v2":"2", "v3":"22", "v4":"3"}},
                           "todo":
            [
                {
                    "replace": {
                        "v1": "v2"
                    }
                },
                {
                    "replace": {
                        "v3": "v4"
                    }
                }
            ]}
        )",
        R"(112)",
        R"(3)"
        }

        , { // test with replacement within two blocks of data and the block of data which can be replaced only at the end of the full data stream
        R"(
            {"dictionary":{"text":{"v7":"1111111", "v4":"1111", "v5":"3", "v2":"2"}},
                           "todo":
            [
                {
                    "replace": {
                        "v7": "v5"
                    }
                },
                {
                    "replace": {
                        "v4": "v2"
                    }
                }
            ]}
        )",
        R"(111111111111)", // 12 of 1
        R"(321)"
        }

        , {
        R"(
            {"dictionary":{"text":{"empty":""},
                           "decimal":{"whitespace":[32]},
                           "hexadecimal":{"tab":["09"]}
                          },
             "todo":
            [
                {
                    "replace": {"whitespace": "empty"}
                },
                {
                    "replace": {"tab": "whitespace"}
                }
            ]}
        )",
        R"(  	        	      )", // 2 tabs here
        R"(  )" // 2 white spaces in result
        }

        , {
        R"(
            {"dictionary":{"text":{"empty":""},
                           "decimal":{"whitespace":[32]},
                           "hexadecimal":{"tab":["09"]}
                          },
             "todo":
            [
                {
                    "replace": {"tab": "whitespace"}
                },
                {
                    "replace": {"whitespace": "empty"}
                }
            ]}
        )",
        R"(     	      	     )", // 2 tabs here
        R"()" // empty result because first we replace tabs to whitespaces
        }

        , { // test with parralel replacement
        R"(
            {"dictionary":{"text":{"33":"33", "22":"22", "23":"23", "-":"--"
                    , "2222--3333":"2222--3333", "ok":"ok"}
                          },
             "todo":
            [
                {
                    "replace": {"33": "22", "22":"33","23":"-"}
                }
                , {
                    "replace": {"2222--3333": "ok"}
                }
            ]}
        )",
        R"(3333232222)", //
        R"(ok)" // result passed through caching at least twice
        }

        , { // test with parralel replacement
        R"(
        {"dictionary":{"text":{"333":"333", "22":"22", "23":"23", "-":"-"
                , "2222-333333":"2222-333333", "ok":"ok"}
                        },
            "todo":
        [
            {
                "replace": {"333": "22", "22":"333","23":"-"}
            }
            , {
                "replace": {"2222-333333": "ok"}
            }
        ]}
        )",
        R"(333333232222)", //
        R"(ok)" // result passed through caching at least twice
        }

        , { // test with parralel replacement
        R"(
        {"dictionary":{"text":{"a":"a", "b":"b", "c":"c", "-":"-", "k":"k", "e":"e", "+":"+"
                , "kkbb+-+bbbkk":"kkbb+-+bbbkk", "ok":"ok"}
                        },
            "todo":
        [
            {
                "replace": {"a": "b", "c":"k","-":"+","e":"-"}
            }
            , {
                "replace": {"k": "b", "b":"k","-":"+","+":"-"}
            }
            , {
                "replace": {"kkbb+-+bbbkk": "ok"}
            }
        ]}
        )",
        R"(abcke-ekccba)", // bbkk-+-kkkbb
        R"(ok)" // result passed through caching at least twice
        }
    };
    for (auto& tst : arrTests)
    {
        std::vector<char> vec(std::begin(tst.jsonData), std::end(tst.jsonData));

        using namespace bpatch;
        ActionsCollection ac(move(vec)); // processor
        TestWriter tw; // here we accumulating data
        ac.SetLastReplacer(StreamReplacer::ReplacerLastInChain(&tw)); // set write point


        std::ranges::for_each(tst.testData, [&ac](const char c) {ac.DoReplacements(c, false); });
        ac.DoReplacements('a', true);

        EXPECT_TRUE(std::ranges::equal(tw.data_accumulator, tst.resultData));

        tw.data_accumulator.clear(); // clear & crecheck
        std::ranges::for_each(tst.testData, [&ac](const char c) {ac.DoReplacements(c, false); });
        ac.DoReplacements('b', true);

        EXPECT_TRUE(std::ranges::equal(tw.data_accumulator, tst.resultData));
    }
}


/// <summary>
///   Check processing for the full range of characters inside LexemeOf1 class.
/// </summary>
///
TEST(ACollection, LexemeOf1Test)
{

    TestData arrTests[] = {
        {
    R"(
            {"dictionary":{
                           "hexadecimal":{"someHex":["AB"], "minusOne":["FF"]}
                          },
             "todo":
            [
                {
                    "replace": {"minusOne": "someHex"}
                }
            ]}
        )",
            { "\xFF\xFF", 2 },
            { "\xAB\xAB", 2 }
        },
        {
    R"(
            {"dictionary":{
                           "hexadecimal":{"first":["AD"], "someHex":["AB"], "minusOne":["FF"]}
                          },
             "todo":
            [
                {
                    "replace": {"first": "minusOne", "minusOne": "someHex"}
                }
            ]}
        )",
    { "\xAD\xFF", 2 },
    { "\xFF\xAB", 2 }
        },
{
    R"(
            {"dictionary":{
                           "hexadecimal":{"zero":["00"], "one":["01"], "minusOne":["FF"], "minusTwo" : ["FE"], "two":["02"], "EE" : ["EE"]}
                          },
             "todo":
            [
                {
                    "replace": {"zero": "minusTwo", "one": "minusOne", "minusTwo": "zero", "minusOne": "one", "two": "EE", "EE": "two"}
                }
            ]}
        )",
    { "\x00\x01\x02\xFF\xFE\xEE", 6 },
    { "\xFE\xFF\xEE\x01\x00\x02", 6 }
        },
{
    R"(
            {"dictionary":{
                           "decimal":{"zero":["0"], "one":["1"], "onetwosix":["126"],
 "onetwoseven":["127"], "onetwoeight":["128"], "onetwonine":["129"], "twofivefour":["254"], "twofivefive":["255"] }
                          },
             "todo":
            [
                {
                    "replace": {"zero": "twofivefive", "one": "twofivefour", "onetwosix": "onetwonine", "onetwoseven": "onetwoeight",
"onetwoeight": "onetwoseven", "onetwonine": "onetwosix", "twofivefour": "one", "twofivefive": "zero"}
                }
            ]}
        )",
    { "\x00\x01\x7E\x7F\x80\x81\xFE\xFF", 8 },
    { "\xFF\xFE\x81\x80\x7F\x7E\x01\x00", 8 }
}
    };
    for (auto& tst : arrTests)
    {
        std::vector<char> vec(std::begin(tst.jsonData), std::end(tst.jsonData));

        using namespace bpatch;
        ActionsCollection ac(move(vec)); // processor
        TestWriter tw; // here we accumulating data
        ac.SetLastReplacer(StreamReplacer::ReplacerLastInChain(&tw)); // set write point


        std::ranges::for_each(tst.testData, [&ac](const char c) {ac.DoReplacements(c, false); });
        ac.DoReplacements('a', true);

        EXPECT_TRUE(std::ranges::equal(tw.data_accumulator, tst.resultData));

        tw.data_accumulator.clear(); // clear & crecheck
        std::ranges::for_each(tst.testData, [&ac](const char c) {ac.DoReplacements(c, false); });
        ac.DoReplacements('b', true);

        EXPECT_TRUE(std::ranges::equal(tw.data_accumulator, tst.resultData));
    }
}

/// <summary>
///    We need to prove that second usage of ActionsCollection class
///  will provide the same result
/// </summary>
///
TEST(ACollection, MultipleUsageOfProcessing)
{
    TestData arrTests[] = {
        {
        R"(
            {"dictionary":{"text":{"should":"should", "must":"must", "binary":"binary", "solid":"solid"}},
                           "todo":
            [
                {
                    "replace": {
                        "should": "must",
                        "binary": "solid"
                    }
                }
            ]}
        )",
        R"(This should be binary text data.)",
        R"(This must be solid text data.)"
        },

        {
        R"(
            {"dictionary":{"text":{"8of5":"55555555", "2of5":"55", "v4":"4"}},
                           "todo":
            [
                {
                    "replace": {
                        "8of5": "v4",
                        "2of5": "v4"
                    }
                }
            ]}
        )",
        R"(555555555551)",
        R"(4451)"
        },

        {
        R"(
            {"dictionary":{"text":{"8of5":"55555555", "3of5":"555", "v4":"4"}},
                           "todo":
            [
                {
                    "replace": { "8of5": "v4" }
                }
                , {
                    "replace": {"3of5": "v4"}
                }
            ]}
        )",
        R"(5555555555)",
        R"(455)"
        }

        , {
        R"(
            {"dictionary":{"text":{"8of5":"55555555", "3of5":"555", "v4":"4"}},
                           "todo":
            [
                {
                    "replace": {
                        "8of5": "v4",
                        "3of5": "v4"
                    }
                }
            ]}
        )",
        R"(5555555555)",
        R"(455)"
        }

        , {
        R"(
            {"dictionary":{"text":{"8of5":"55555555", "2of5":"55", "v4":"4"}},
                           "todo":
            [
                {
                    "replace": { "8of5": "v4" }
                }
                , {
                    "replace": {"2of5": "v4"}
                }
            ]}
        )",
        R"(5555555555)",
        R"(44)"
        }

        , {
        R"(
            {"dictionary":{"text":{"8of5":"55555555", "2of5":"55", "v4":"4"}},
                           "todo":
            [
                {
                    "replace": {
                        "8of5": "v4",
                        "2of5": "v4"
                    }
                }
            ]}
        )",
        R"(5555555555)",
        R"(44)"
        }

        , {
        R"(
            {"dictionary":{"text":{"8of5":"55555555", "2of5":"55", "v4":"4"}},
                           "todo":
            [
                {
                    "replace": { "8of5": "v4" }
                }
                , {
                    "replace": {"2of5": "v4"}
                }
            ]}
        )",
        R"(555555555551)",
        R"(4451)"
        }

        , {
        R"(
            {"dictionary":{"text":{"8of5":"55555555", "2of5":"55", "v4":"4"}},
                           "todo":
            [
                {
                    "replace": {
                        "8of5": "v4",
                        "2of5": "v4"
                    }
                }
            ]}
        )",
        R"(555555555551)",
        R"(4451)"
        }

        , {
        R"(
        {"dictionary":{"text":{"12=":"12=", "12-":"12-", "12+":"12+", "12_":"12_"}},
                        "todo":
        [
            {
                "replace": { "12=": "12-" }
            }
            , {
                "replace": { "12-": "12+" }
            }
            , {
                "replace": { "12+": "12_" }
            }
            , {
                "replace": { "12_": "12=" }
            }
        ]}
        )",
        R"(212_1212+1212-1212==)",
        R"(212=1212=1212=1212==)"
        }

        , {
        R"(
        {"dictionary":{"text":{"12=":"12=", "12-":"12-", "12+":"12+", "12_":"12_"}},
                        "todo":
        [
            {
                "replace": {
                  "12=": "12-"
                , "12-": "12+"
                , "12+": "12_"
                , "12_": "12="
                }
            }
        ]}
        )",
        R"(212_1212+1212-1212==)",
        R"(212=1212_1212+1212-=)"
        }

        , {
        R"(
        {"dictionary":{"text":{"BCDEFGH":"BCDEFGH", "1":"1", "DCE":"DCE", "2":"2"}},
                        "todo":
        [
            {
                "replace": {
                "BCDEFGH": "1"
                , "DCE": "2"
                }
            }
        ]}
        )",
        R"(ABCDBDCEBCDEFBCDEFGH )",
        R"(ABCDB2BCDEF1 )"
        }

        , {
        R"(
        {"dictionary":{"text":{"BCDEFGH":"BCDEFGH", "1":"1", "DCE":"DCE", "2":"2"}},
                        "todo":
        [
            {
                "replace": {
                "BCDEFGH": "1"
                , "DCE": "2"
                }
            }
        ]}
        )",
        R"(ABCDBCDEBCDEFBCDEFGH DCA)",
        R"(ABCDBCDEBCDEF1 DCA)"
        }

            , {
            R"(
        {"dictionary":{"text":{"eres":"eres", "1":"1", "inner":"inner", "ntk":"ntk", " O":" O", " V":" V"
                             , "nt1":"nt1", "erebu":"erebu"}},
                        "todo":
        [
            {
                "replace": {
                " V": " V"
                , "erebu": "1"
                , "ntk": "1"
                , "eres": "1"
                , "nt1": "1"
                , "inner": "1"
                , " O": "1"
                }
            }
        ]}
        )",
        R"(Here is some text with intersection of inner characters. and one replace array only.)",
        R"(Here is some text with intersection of 1 characters. and one replace array only.)"
            }
    }; // TestData;

    for (auto& tst : arrTests)
    {
        std::vector<char> vec(std::begin(tst.jsonData), std::end(tst.jsonData));

        using namespace bpatch;
        ActionsCollection ac(move(vec)); // processor
        TestWriter tw; // here we accumulating data
        ac.SetLastReplacer(StreamReplacer::ReplacerLastInChain(&tw)); // set write point


        std::ranges::for_each(tst.testData, [&ac](const char c) {ac.DoReplacements(c, false); });
        ac.DoReplacements('c', true);
        EXPECT_TRUE(std::ranges::equal(tw.data_accumulator, tst.resultData));

        tw.data_accumulator.clear(); // clear & crecheck
        std::ranges::for_each(tst.testData, [&ac](const char c) {ac.DoReplacements(c, false); });
        ac.DoReplacements('d', true);

        EXPECT_TRUE(std::ranges::equal(tw.data_accumulator, tst.resultData));
    }

}


int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
