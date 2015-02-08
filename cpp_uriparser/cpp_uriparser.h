#pragma once

#include <boost/optional.hpp>
#include <boost/noncopyable.hpp>
#include <stdexcept>
#include <iterator>
#include <memory>
#include "cpp_uriparser_query.h"
#include "uriparser/Uri.h"

namespace uri_parser
{
  namespace internal
  {
    template <typename T>
    struct base_type{
      typedef typename std::remove_const<typename std::remove_pointer<T>::type>::type type;
    };

    template <typename T>
    struct base_const_ptr{
      typedef typename std::add_pointer<typename std::add_const<typename base_type<T>::type>::type>::type type;
    };

    template <typename T>
    struct base_ptr{
      typedef typename std::add_pointer<typename base_type<T>::type>::type type;
    };

    // by default lets use ANSI api functions
    template <class UrlTextType, class Empty = void>
    struct UriTypes: private boost::noncopyable
    {
      typedef UriUriA UriObjType;
      typedef UriParserStateA UriStateType;
      typedef UriPathSegmentA UriPathSegmentType;
      typedef std::string UrlReturnType;

      std::function<int(UriStateType*, UrlTextType)> parseUri;
      std::function<void(UriObjType*)> freeUriMembers;
      std::function<int(UriObjType*)> uriNormalizeSyntax;
      // add_const to support UrlTextType == tchar* & const tchar* ( api output is exactly const tchar* )
      std::function<typename base_const_ptr<UrlTextType>::type(typename base_ptr<UrlTextType>::type, UriBool, UriBreakConversion)> uriUnescapeInPlaceEx;

      UriTypes() :
        parseUri(&uriParseUriA),
        freeUriMembers(&uriFreeUriMembersA),
        uriNormalizeSyntax(&uriNormalizeSyntaxA),
        uriUnescapeInPlaceEx(&uriUnescapeInPlaceExA)
      {}

      // todo: remove copypasted move method!
      UriTypes(UriTypes&& right) :
        parseUri(std::move(right.parseUri)),
        freeUriMembers(std::move(right.freeUriMembers)),
        uriNormalizeSyntax(std::move(right.uriNormalizeSyntax)),
        uriUnescapeInPlaceEx(std::move(right.uriUnescapeInPlaceEx))
      {}
    };

    template <class UrlTextType>
    struct UriTypes<UrlTextType, typename std::enable_if<std::is_convertible<UrlTextType, const wchar_t*>::value >::type>:
      private boost::noncopyable
    {
      typedef UriUriW UriObjType;
      typedef UriParserStateW UriStateType;
      typedef UriPathSegmentW UriPathSegmentType;
      // hardcoded output type for wchar_t* to wstring
      typedef std::wstring UrlReturnType;

      std::function<int(UriStateType*, UrlTextType)> parseUri;
      std::function<void(UriObjType*)> freeUriMembers;
      std::function<int(UriObjType*)> uriNormalizeSyntax;
      std::function<typename base_const_ptr<UrlTextType>::type(typename base_ptr<UrlTextType>::type, UriBool, UriBreakConversion)> uriUnescapeInPlaceEx;

      UriTypes() :
        parseUri(&uriParseUriW),
        freeUriMembers(&uriFreeUriMembersW),
        uriNormalizeSyntax(&uriNormalizeSyntaxW),
        uriUnescapeInPlaceEx(&uriUnescapeInPlaceExW)
      {}

      UriTypes(UriTypes&& right) :
        parseUri(std::move(right.parseUri)),
        freeUriMembers(std::move(right.freeUriMembers)),
        uriNormalizeSyntax(std::move(right.uriNormalizeSyntax)),
        uriUnescapeInPlaceEx(std::move(right.uriUnescapeInPlaceEx))
      {}
    };

    template <class UriTextRangeType, class UrlReturnType>
    UrlReturnType GetStringFromUrlPartInternal(UriTextRangeType& range)
    {
      if (range.first == nullptr || (range.afterLast == nullptr))
      {
        return UrlReturnType();
      }

      return UrlReturnType(range.first, range.afterLast);
    }
  } // namespace internal

  template <class UriPathSegmentType, class UrlReturnType>
  class UrlPathIterator: public std::iterator <std::forward_iterator_tag, UriPathSegmentType>
  {
    typedef UrlPathIterator<UriPathSegmentType, UrlReturnType> IteratorType;
  public:
    explicit UrlPathIterator(UriPathSegmentType pathSegment):
      pathSegment_(pathSegment){}
    UrlPathIterator(const UrlPathIterator& mit):
      pathSegment_(mit.pathSegment_),
      returnObjStorage_(mit.returnObjStorage_){}

    IteratorType& operator++()
    {
      if (pathSegment_.next != nullptr)
      {
        pathSegment_ = *pathSegment_.next;
      }
      else
      {
        *this = CreateEndIt<const IteratorType>();
      }
        
      return *this;
    }

    IteratorType operator++(int) const
    {
      IteratorType tmp(*this);
      return ++tmp;
    }

    bool operator==(const IteratorType& right)
    {
      return ((pathSegment_.text.afterLast == right.pathSegment_.text.afterLast) &&
        (pathSegment_.next == right.pathSegment_.next));
    }

    bool operator!=(const IteratorType& right)
    {
      return !operator==(right);
    }

    UrlReturnType operator*() const { return GetStringFromUrlPart(pathSegment_.text); }
    UrlReturnType* operator->()
    {
      returnObjStorage_ = std::move(operator*());
      return &returnObjStorage_;
    }

    IteratorType begin() const { return UrlPathIterator(pathSegment_); }
    const IteratorType cbegin() const { return UrlPathIterator(pathSegment_); }
    IteratorType end() const { return CreateEndIt<IteratorType>(); }
    const IteratorType cend() const { return CreateEndIt<const IteratorType>(); }
  private:
    template <typename UriTextRangeTypeT>
    UrlReturnType GetStringFromUrlPart(UriTextRangeTypeT val) const
    {
      return internal::GetStringFromUrlPartInternal<UriTextRangeTypeT, UrlReturnType>(val);
    }

    template <typename OutItType>
    OutItType CreateEndIt() const
    {
      decltype(pathSegment_.text) endTextRange = {nullptr, nullptr};
      UriPathSegmentType endPathSegment = {endTextRange, nullptr, nullptr};
      return OutItType(endPathSegment);
    }

    UriPathSegmentType pathSegment_;
    UrlReturnType returnObjStorage_;
  };

  template <class UrlTextType>
  class UriEntry: boost::noncopyable
  {
    typedef internal::UriTypes<UrlTextType> UriApiTypes;
    typedef typename UriApiTypes::UriObjType UriObjType;
    typedef typename UriApiTypes::UrlReturnType UrlReturnType;
    typedef typename UriApiTypes::UriStateType UriStateType;
    typedef typename UriApiTypes::UriPathSegmentType UriPathSegmentType;
  public:
    UriEntry(UrlTextType urlText) :
      freeMemoryOnClose_(true)
    {
      state_.uri = &uriObj_;
      if (uriTypes_.parseUri(&state_, urlText) != URI_SUCCESS)
      {
        throw std::runtime_error("uriparser: Uri entry creation failed");
      }
    }

    UriEntry(UriEntry&& right) :
      state_(std::move(right.state_)),
      uriObj_(std::move(right.uriObj_)),
      uriTypes_(std::move(right.uriTypes_)),
      freeMemoryOnClose_(true)
    {
      right.freeMemoryOnClose_ = false;
    }

    virtual ~UriEntry()
    {
      if (freeMemoryOnClose_)
      {
        uriTypes_.freeUriMembers(&uriObj_);
      }
    }

    boost::optional<UrlReturnType> Scheme() const
    {
      return GetStringFromUrlPart(uriObj_.scheme);
    }

    boost::optional<UrlReturnType> UserInfo() const
    {
      return GetStringFromUrlPart(uriObj_.userInfo);
    }

    boost::optional<UrlReturnType> HostText() const
    {
      return GetStringFromUrlPart(uriObj_.hostText);
    }

    UriQueryList<UrlTextType> Query() const
    {
      return UriQueryList<UrlTextType>(uriObj_);
    }

    boost::optional<UrlReturnType> Fragment() const
    {
      return GetStringFromUrlPart(uriObj_.fragment);
    }

    UrlPathIterator<UriPathSegmentType, UrlReturnType> PathHead() const
    {
      return UrlPathIterator<UriPathSegmentType, UrlReturnType>(*uriObj_.pathHead);
    }

    void Normalize()
    {
      uriTypes_.uriNormalizeSyntax(&uriObj_);
    }

    boost::optional<UrlReturnType> GetUnescapedUrlString(bool plusToSpace, UriBreakConversion breakConversion = URI_BR_DONT_TOUCH)
    {
      typedef std::remove_pointer<UrlTextType> UrlTextTypeNoPtr;
      typedef std::remove_const<UrlTextTypeNoPtr> UrlTextTypeBase;

      UrlReturnType reslt(uriObj_.scheme.first);

      UrlTextType endPtr = uriTypes_.uriUnescapeInPlaceEx(
        &reslt.front(),
        plusToSpace ? URI_TRUE : URI_FALSE,
        breakConversion);

      reslt.assign(reslt.c_str());

      return boost::optional<UrlReturnType>(reslt);
    }

  private:
    template <class UriTextRangeType>
    boost::optional<UrlReturnType> GetStringFromUrlPart(UriTextRangeType range) const
    {
      auto retVal = internal::GetStringFromUrlPartInternal<UriTextRangeType, UrlReturnType>(range);
      if (!retVal.empty())
      {
        return boost::optional<UrlReturnType>(retVal);
      }
      return boost::optional<UrlReturnType>();
    }

    UriApiTypes uriTypes_;

    UriStateType state_;
    UriObjType uriObj_;
    bool freeMemoryOnClose_;
  };

  // Use this helper proc to create UriEntry obj
  template <typename T>
  UriEntry<T> UriParseUrl(T url)
  {
    return UriEntry<T>(url);
  }
    
  template <typename T>
  UriEntry<T> ParseUrlWithHost(T url)
  {
    UriEntry<T> retVal(url);

    auto hostText = retVal.HostText();

    if (!hostText.is_initialized())
    {
      throw std::runtime_error("uriparser: Expected url with host, but no host found");
    }

    return retVal;
  }
} // namespace uri_parser