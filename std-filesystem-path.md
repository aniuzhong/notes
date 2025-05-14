# `std::filesystem::path` - A Cross-platform String Designed for Filesystem

## Contents
- [`std::filesystem::path` - A Cross-platform String Designed for Filesystem](#stdfilesystempath---a-cross-platform-string-designed-for-filesystem)
  - [Contents](#contents)
  - [Introduction](#introduction)
  - [How to initialize a `std::filesystem::path` object](#how-to-initialize-a-stdfilesystempath-object)
  - [Only Syntactic Aspects of Paths Are Handled](#only-syntactic-aspects-of-paths-are-handled)
  - [Concatenation and Decomposition](#concatenation-and-decomposition)
  - [Relative Path](#relative-path)
  - [std::filesystem::path::string() are NOT recommended on Windows](#stdfilesystempathstring-are-not-recommended-on-windows)
  - [An Alternative for std::codecvt](#an-alternative-for-stdcodecvt)

## Introduction

According to the [CppReference](https://en.cppreference.com/w/cpp/filesystem/path), std::filesystem::path

> Objects of type path represent paths on a filesystem. Only syntactic aspects of paths are handled: the pathname may represent a non-existing path or even one that is not allowed to exist on the current file system or OS.

This illustrates that std::filesystem::path offers functionalities for manipulating paths without requiring interaction with the underlying filesystem. Thus, in essence, std::filesystem::path serves as a cross-platform abstraction with string-like behaviors, specifically designed for handling filesystem paths.

## How to initialize a `std::filesystem::path` object

``` C++
#include <filesystem>

int main()
{
#ifdef _WIN32
    static_assert(std::is_same_v<std::filesystem::path::value_type, wchar_t>);
    static_assert(std::is_same_v<std::filesystem::path::string_type, std::wstring>);
    static_assert(std::filesystem::path::preferred_separator == L'\\');
#else
    static_assert(std::is_same_v<std::filesystem::path::value_type, char>);
    static_assert(std::is_same_v<std::filesystem::path::string_type, std::string>);
    static_assert(std::filesystem::path::preferred_separator == '/');
#endif

    // Platform-Specific Native Methods
#ifdef _WIN32
    // Windows native UTF-16 (wstring)
    std::filesystem::path windows_path(L"C:\\path\\to\\file");
#else
    // POSIX systems (Linux/macOS) native UTF-8 (string)
    std::filesystem::path posix_path("/path/to/file");
#endif

    // UTF-8 string literals (`u8`)
#if __cplusplus >= 202002L
    auto u8path = std::filesystem::path(u8"C:\\📁_1\\📁_2\\📁_3\\📄.txt");
#else
    auto u8path = std::filesystem::u8path(u8"C:\\📁_1\\📁_2\\📁_3\\📄.txt");
#endif

    // UTF-16 string literals (`u`)
    auto u16path = std::filesystem::path(u"/📁_1/📁_2/📁_3/📄.txt");

    // UTF-32 string literals (`u`)
    auto u32path = std::filesystem::path(U"/📁_1/📁_2/📁_3/📄.txt");
}
```

Note that, in MSVC's implementation of C++17's std::filesystem::path, the encoding behavior depends on the character type used during construction:

- When constructed with char or u8 string literals, the path retains the original byte sequence without conversion.
- When constructed with wchar_t, char16_t, or char32_t, the path is converted into Windows' native UTF-16 encoding format.

To ensure proper encoding, a std::filesystem::path object should be explicitly constructed using std::filesystem::u8path(). However, since C++20, std::filesystem::u8path() has been deprecated, and the constructor can now be used directly.

## Only Syntactic Aspects of Paths Are Handled

std::filesystem::path correctly interprets the path structure according to platform-specific rules. It allows the creation of path objects without immediately validating whether the path is legal. It does not ensure

- Whether the path points to an existing file or directory.
- Whether the path contains illegal characters (though decomposition might still work).
- Whether the path respects platform-specific limitations (e.g., length constraints or reserved names like CON on Windows).

``` C++
#ifdef _WIN32
#include <minwindef.h>
#endif

#include <filesystem>

int main()
{
    // Illegal characters: \ / : * ? " < > |
    std::filesystem::path illegal(u"\\/:*?\"<>\|");
    assert(illegal.u16string() == std::u16string(u"\\/:*?\"<>\|"));
    
    // Overlong on Windows
#ifdef _WIN32
    std::wstring s(MAX_PATH + 1, L'*');
    std::filesystem::path overlong(s);
    assert(overlong.u16string() == std::u16string(MAX_PATH + 1, L'*'));
#endif
}
```

## Concatenation and Decomposition

``` C++
int main()
{
    auto drive_c = std::filesystem::path(u"C:\\"); // The internal string on Windows is L"C:\" while on Posix it is "C:\".
    assert(drive_c.u16string() == std::u16string(u"C:\\"));
    assert(drive_c.generic_u16string() == std::u16string(u"C:/"));
    assert(drive_c.root_name().u16string() == std::u16string(u"C:"));
    assert(drive_c.root_path().u16string() == std::u16string(u"C:\\"));
    assert(drive_c.root_path().generic_u16string() == std::u16string(u"C:/"));
    assert(drive_c.root_directory().u16string() == std::u16string(u"\\"));
    assert(drive_c.root_directory().generic_u16string() == std::u16string(u"/"));

    std::filesystem::path folder_1(u"📁_1");
    std::filesystem::path folder_2(u"📁_2");
    std::filesystem::path folder_3(u"📁_3");

    std::filesystem::path filename(u"📄.🆎");
    assert(filename.stem().u16string() == std::u16string(u"📄"));
    assert(filename.extension().u16string() == std::u16string(u".🆎"));
    assert(filename.filename().u16string() == std::u16string(u"📄.🆎"));

    auto path = drive_c / folder_1 / folder_2 / folder_3 / filename;
    assert(path.generic_u16string() == std::u16string(u"C:/📁_1/📁_2/📁_3/📄.🆎"));
}
```

``` C++
int main()
{
#if __cplusplus >= 202002L
    auto p = std::filesystem::path(u8"C:\\📁_1\\📁_2\\📁_3\\📄.txt");
#else
    auto p = std::filesystem::u8path(u8"C:\\📁_1\\📁_2\\📁_3\\📄.txt");
#endif
    //
    // [ L"C:", L"\\", L"📁_1", L"📁_2", L"📁_3", L"📄.txt" ]
    //
    bool is_absolute                     = p.is_absolute(); // true
    bool has_root_name                   = p.has_root_name(); // true
    bool has_root_path                   = p.has_root_path(); // true
    bool has_root_directory              = p.has_root_directory(); // true
    std::filesystem::path root_name      = p.root_name(); // L"C:"
    std::filesystem::path root_path      = p.root_path(); // L"C:\\"
    std::filesystem::path root_directory = p.root_directory(); // L"\\"
    std::filesystem::path stem           = p.stem(); // L"📄"
    std::filesystem::path filename       = p.filename(); // L"📄.txt"
    std::filesystem::path extension      = p.extension(); // L".txt"
}
```

## Relative Path

std::filesystem::path recognizes a relative path when

1. No Root Component: A relative path does not start with a root directory (/ on Linux/macOS or C:\ on Windows).
2. No Drive Letter (Windows): If a path lacks a drive letter (C:), it is considered relative.

Relative paths are interpreted based on the current working directory (std::filesystem::current_path)

## std::filesystem::path::string() are NOT recommended on Windows

On the Windows platform, using std::filesystem::path::string can require some caution. The behavior of std::filesystem::path::string depends on the current system's default code page, which can lead to unexpected results when dealing with non-ASCII characters. Instead, it is recommended to use std::filesystem::path::wstring to obtain a UTF-16 encoded std::wstring, which is more consistent and reliable for handling Unicode characters on Windows.

``` C++
#include <filesystem>
#include <iostream>

int main()
{
    std::filesystem::path path(L"你好，世界，👋，🌏");
    
    auto u8str  = path.u8string();
    auto u16str = path.u16string();
    auto u32str = path.u32string();

    try
    {
        auto str = path.string();
    }
    catch (std::exception& e)
    {
        // On Windows may get
        // No mapping for the Unicode character exists in the target multi-byte code page
        std::cerr << e.what() << '\n';
    }
}
```

In MSVC, the std::filesystem::path::string function ultimately calls __std_fs_convert_wide_to_narrow. From the following source code, it can be seen that when the current code page is not CP_UTF8, the Win32 API WideCharToMultiByte is called. Additionally, it checks whether _Used_default_char is TRUE to determine if the local character set includes a mapping for the input Unicode characters. If no such mapping exists, an exception is thrown ("No mapping for the Unicode character exists in the target multi-byte code page"). Here is the [source code](https://github.com/microsoft/STL/blob/main/stl/src/filesystem.cpp)

``` C++
[[nodiscard]] __std_fs_convert_result __stdcall __std_fs_convert_wide_to_narrow(_In_ const __std_code_page _Code_page,
    _In_reads_(_Input_len) const wchar_t* const _Input_str, _In_ const int _Input_len,
    _Out_writes_opt_(_Output_len) char* const _Output_str, _In_ const int _Output_len) noexcept {
    __std_fs_convert_result _Result;

    if (_Code_page == __std_code_page{CP_UTF8} || _Code_page == __std_code_page{54936}) {
        // For UTF-8 or GB18030, attempt to use WC_ERR_INVALID_CHARS. (These codepages can't use WC_NO_BEST_FIT_CHARS
        // below, and other codepages can't use WC_ERR_INVALID_CHARS.)
        _Result._Len = WideCharToMultiByte(static_cast<unsigned int>(_Code_page), WC_ERR_INVALID_CHARS, _Input_str,
            _Input_len, _Output_str, _Output_len, nullptr, nullptr);
    } else { // For other codepages, attempt to use WC_NO_BEST_FIT_CHARS.
             // Codepages that don't support this will activate the ERROR_INVALID_FLAGS fallback below.
        BOOL _Used_default_char = FALSE;

        _Result._Len = WideCharToMultiByte(static_cast<unsigned int>(_Code_page), WC_NO_BEST_FIT_CHARS, _Input_str,
            _Input_len, _Output_str, _Output_len, nullptr, &_Used_default_char);

        if (_Used_default_char) { // Report round-tripping failure with ERROR_NO_UNICODE_TRANSLATION,
                                  // "No mapping for the Unicode character exists in the target multi-byte code page."
            return {0, __std_win_error{ERROR_NO_UNICODE_TRANSLATION}};
        }
    }

    _Result._Err = _Result._Len == 0 ? __std_win_error{GetLastError()} : __std_win_error::_Success;

    if (_Result._Err == __std_win_error{ERROR_INVALID_FLAGS}) { // Fall back to a non-strict conversion.
        _Result._Len = WideCharToMultiByte(static_cast<unsigned int>(_Code_page), 0, _Input_str, _Input_len,
            _Output_str, _Output_len, nullptr, nullptr);

        _Result._Err = _Result._Len == 0 ? __std_win_error{GetLastError()} : __std_win_error::_Success;
    }

    return _Result;
}
```

## An Alternative for std::codecvt

In C++17, std::codecvt was deprecated. Following approach leverages std::filesystem::path to convert between different string encodings. While this does work, it is not a reliable or standard way to perform encoding conversion. A more robust solution would be using a dedicated library like [ICU](https://icu.unicode.org).

``` C++
#include <string>
#include <string_view>
#include <filesystem>

//
// Helpers for safe conversion between char_t and char8_t since C++20
//
#if __cplusplus >= 202002L
inline char*           toC(char8_t* p)                            { return reinterpret_cast<char*>(p); }
inline char const*     toC(char8_t const* p)                      { return reinterpret_cast<char const*>(p); }
inline char8_t*        toU(char* p)                               { return reinterpret_cast<char8_t*>(p); }
inline char8_t const*  toU(char const* p)                         { return reinterpret_cast<char8_t const*>(p); }
inline std::string     toC(const std::u8string& s)                { return { toC(s.data()), s.size() }; }
inline std::u8string   toU(const std::string& s)                  { return { toU(s.data()), s.size() }; }
#endif

//
// Helpers for safe conversion between wchar_t and char16_t in MSVC
// https://github.com/LibreOffice/core/blob/master/include/o3tl/char16_t2wchar_t.hxx
//
#ifdef _MSC_VER
inline wchar_t*             toW(char16_t* p)                           { return reinterpret_cast<wchar_t*>(p); }
inline wchar_t const*       toW(char16_t const* p)                     { return reinterpret_cast<wchar_t const*>(p); }
inline char16_t*            toU(wchar_t* p)                            { return reinterpret_cast<char16_t*>(p); }
inline char16_t const*      toU(wchar_t const* p)                      { return reinterpret_cast<char16_t const*>(p); }
inline std::u16string_view  toU(std::wstring_view v)                   { return { toU(v.data()), v.size() }; }
inline std::wstring_view    toW(std::u16string_view v)                 { return { toW(v.data()), v.size() }; }
inline std::wstring         toW(std::u16string s)                      { return { toW(s.data()), s.size() }; }
inline std::u16string       toU(std::wstring s)                        { return { toU(s.data()), s.size() }; }

#if __cplusplus >= 202002L
inline std::u8string   to_u8string(const std::u16string& u16str)  { return std::filesystem::path(u16str).u8string(); }
inline std::u16string  to_u16string(const std::u8string& u8str)   { return std::filesystem::path(u8str).u16string(); }
#else
inline std::string     to_u8string(const std::u16string& u16str)  { return std::filesystem::path(u16str).u8string(); }
inline std::u16string  to_u16string(const std::string& u8str)     { return std::filesystem::u8path(u8str).u16string(); }
#endif

#if __cplusplus >= 202002L
inline std::string     to_string(const std::wstring& wstr)        { return toC(to_u8string(toU(wstr))); }
inline std::wstring    to_wstring(const std::string& str)         { return toW(to_u16string(toU(str))); }
#else
inline std::string     to_string(const std::wstring& wstr)        { return to_u8string(toU(wstr)); }
inline std::wstring    to_wstring(const std::string& str)         { return toW(to_u16string(str)); }
#endif
#endif // _MSC_VER
```