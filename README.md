cpp_uriparser [![Build Status](https://travis-ci.org/azerg/cpp_uriparser.svg?branch=master)] ( https://travis-ci.org/azerg/cpp_uriparser )
=============
С++ header-only library to parse urls, traverse through url parts, queries.
cpp_uriparser is C++ wrapper for c [uriparser library].

# Features
C++ wrapper for uriparser have next features:
- Iterator for path
- Iterator for query
- One implementation supports both ANSI & Unicode

# Dependencies
* [uriparser library] - tested with **uriparser-0.8.1**
* [boost] - boost::optional
* [gtest] - is required for building sample test project

# Sample usage
1. Sample shows hot to iterate through paths:
```cpp
auto parsedUrl = uri_parser::UriParseUrl("https://github.com/azerg/cpp_uriparser/blob/master/README.md");
auto pathHead = parsedUrl.PathHead();
for ( auto pathIt : pathHead )
{
    std::cout << pathIt->c_str() << std::endl;
}
```

# License
MIT


[uriparser library]:http://uriparser.sourceforge.net/
[boost]: http://boost.org
[gtest]: https://code.google.com/p/googletest/
