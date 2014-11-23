cpp_uriparser
=============
cpp_uriparser is C++ wrapper for c [uriparser library].

# Features
C++ wrapper for uriparser have next features:
- Iterator for path
- One implementation supports both ANSI & Unicode

# Dependencies
* [uriparser library] - tested with **uriparser-0.8.1**
* [boost] - boost::optional

# Sample usage
1. Sample shows hot to iterate through paths:
```cpp
auto parsedUrl = core::common_parsers::UriParseUrl("https://github.com/azerg/cpp_uriparser/blob/master/README.md");
auto headIt = parsedUrl.PathHead();
for ( auto pathIt = headIt.cbegin(); pathIt != headIt.cend(); ++pathIt )
{
    std::cout << pathIt->c_str() << std::endl;
}
```

# License
MIT


[uriparser library]:http://uriparser.sourceforge.net/
[boost]: http://boost.org
