# bpatch Application Guide

**PURPOSE:** This is a manual for usage and parametrization of the console application `bpatch`

**Table Of Contents:**
* link to [`README.md`][readme_md] for `bpatch`
* [`bpatch Application Guide`](#bpatch-application-guide)
  * [Application Console Parameters](#application-console-parameters)
  * [Folders for Actions and Binary Data. AFN and BFFN](#folders-for-actions-and-binary-data-afn-and-bffn)
  * [Actions File Sample](#actions-file-sample)
  * [JSON Composition Rules](#json-composition-rules)
    * [dictionary](#dictionary)
    * [todo](#todo)
  * [Reference](#reference)

## Application Console Parameters
Command format of `bpatch` is defined as follows:

`bpatch -s SOURCE -a ACTIONS [-d/-w DEST] [-fa AFN] [-fb BFFN]`

| Parameter | Description |
| --- | --- |
| `-s SOURCE` | Data from the SOURCE file (considered as binary data) will be altered |
| `-a ACTIONS` | Rules fetched from the ACTIONS file will be applied to the SOURCE file |
| `-d DEST` |  If used, results will be saved into DEST file. If `-d` or `-w` is not used, SOURCE file will be modified directly in place. In case if `-d` was used and the DEST file exists `bpatch` interrupts processing |
| `-w DEST` | Use this flag to force override the DEST file |
| `-fa AFN` | To specify the Actions Folder Name: AFN, where ACTIONS file will be searched |
| `-fb BFFN` | To specify the Binary Files Folder Name: BFFN, where binary files potentially mentioned in ACTIONS file will be searched |
| `-h, ?, --help, /h` | Display help message |

**NOTE:** All parameters are case insensitive (e.g. `-w` is the same as `-W`). Wild characters are not allowed (but planned to simplify processing files by mask).

## Folders for Actions and Binary Data. AFN and BFFN

The ACTIONS file will be initially searched for in the current directory and then in the 'bpatch_a' subfolder located near the 'bpatch' executable. Binary files referenced in the ACTIONS file will be searched first in the current directory, then in the 'bpatch_b' subfolder located near the 'bpatch' executable. So the structure must be next:
  * Installation folder
    * `bpatch`
    * bpatch_a\\ACTIONS files
    * bpatch_b\\binary data files from ACTIONS file (if files are needed)

**NOTE:** It's possible to modify these folders using `-fa` for actions folder (AFN) and `-fb` for binary files folder (BFFN)

## Actions File Sample
An Actions file sample written in proven ASCII symbols and [`JSON`][JSON] format can be as follows:

<details><summary><B>Sample with ACTIONS file (press triangle to expand)</B></summary>

```json
{
    "dictionary":
    {
        "decimal":
        {
            "pattern name": [ 13, 10 ],
            "another binary": [ 13 ]
        },
        "hexadecimal":
        {
            "hex pattern name": [ "0A", "09", "03" ],
            "hex02": [ "00", "Fe", "3A" ]
        },
        "text":
        {
            "text name": "textual value",
            "Hi": "Hello World!"
        },
        "file":
        {
            "fileX": "filename.bin"
        },
        "composite":
        [
            {
                "composite 001":
                [
                    "pattern name",
                    "text name",
                    "hex pattern name"
                ]
            },
            {
                "complexOne":
                [
                    "hex02",
                    "fileX",
                    "Hi",
                    "composite 001"
                ]
            }
        ]
    },
    "todo":
    [
        {
            "replace":
            {
                "pattern name": "hex pattern name",
                "hex02": "complexOne"
            }
        },
        {
            "replace":
            {
                "text name": "fileX"
            }
        }
    ]
}
```
</details>

**Note:** `bpatch` has own [`JSON`][JSON] parser and therefore
  * Control characters in the text **cannot** be unicode. /uXXXX sequences will remain the same (ignored for transformation)
  * Control [`JSON`][JSON] character sequences in the [`JSON`][JSON] text literals like \\b \\f \\n \\r \\t \\\\ \\/ \\" will be transformed and processed. For example text [`JSON`][JSON] object { "text\\"" : "meaning" } will have a name `text"` and value `meaning`
  * [`JSON`][JSON] rules not allow line wrapping however for `bpatch` parser it is not a problem. Nonetheless, the recommendation is to use \\n or \\r instead of this trick


## JSON Composition Rules
1. ACTIONS file must contain root [`JSON`][JSON] object: { }
1. root [`JSON`][JSON] object must contain `dictionary` and `todo` objects inside

### dictionary
 '`dictionary`' is defining set of lexemes to search in the SOURCE file and to write into DEST file

The dictionary object follows these rules:

| Property | Description |
| --- | --- |
| `decimal` | The decimal object is required to have a nonempty name and should contain arrays of decimal values within the range of 0 to 255. Sample 1: `"decimal123" : [1, 2, 3]` to describe 3 sequential bytes 1,2 and 3 respectivelly; Sample 2: `"empty" : [ ]` to describe empty sequence which can be used to remove something from the data |
| `hexadecimal` | The hexadecimal object is required to have a nonempty name and should contain arrays of hexadecimal string values within the range of "00" to "FF". Sample 3: `"hexa1310" : ["0D", "0a"]` to describe 2 sequential bytes 13, 10 respectivelly |
| `text` | The text object is required to have a nonempty name and should contain text value. Values are strings which represent byte sequences. **Note:** Control characters in text cannot be unicode. |
| `file` | The file object is required to have a nonempty name and should contain value with file name - the whole file data will be treated as a pattern. The JSON Sample 4 `"file": { "oldImage": "x.png", "newImage": "flower.jpg"}` means that you can address binary sequence from file x.png as "oldImage" and binary sequence from file flower.jpg as "newImage" |
| `composite` | The composite object is required to have a nonempty name. It must be an array of objects that will be merged together. Values are arrays of previously defined objects from dictionary. For example the result byte sequence for the "composite 001" object from the [Actions File Sample](#actions-file-sample) will be merged from decimal:[13, 10] text:"textual value" and hexadecimal: "0A", "09", "03" |

### todo
 '`todo`' is an array of objects that defines transformations. At the moment only 'replace' objects are being supported
 * "replace" object describes the replacement(s) which should be applied to the SOURCE file data
 * Parralel replacement: for Sample 5 `"replace": { "decimal123": "hexa1310", "hexa1310": "decimal123" }` it means that in the DEST file tokens with the corresponding names will be replaced with other tokens in parallel (concurrently). In the place where the token data with the name "decimal123" used to be, there will be token data with the name "hexa1310" and vice versa, where there used to be data from "hexa1310" there will be data from "decimal123"
 * Sequential replacement: for Sample 6 `{"replace": { "decimal123": "hexa1310"}}, {"replace": {"hexa1310": "decimal123" }}` sequential replacement will be applied. Data from the token "decimal123" will be replaced with data from token "hexa1310". Subsequently, during next replacement all data which matches to "hexa1310" data tocken will be replaced with data from "decimal123" token. And DEST file will not contain any "hexa1310" tokens data

Big sample of ACTIONS file: [Actions File Sample](#actions-file-sample)

---
## Reference

[`JSON`][JSON]

[`README.md`][readme_md]

[JSON]:https://www.json.org/json-en.html

[readme_md]:./README.md
---
[`to the Table Of Contents`](#bpatch-application-guide)
