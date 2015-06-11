#pragma once

#include <exception>
#include <iostream>
#include <iterator>
#include <string>
#include <type_traits>
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

    // default API types
#define URI_API_TYPES(PREFIX) \
      typedef UriUri##PREFIX UriObjType; \
      typedef UriParserState##PREFIX UriStateType; \
      typedef UriPathSegment##PREFIX UriPathSegmentType; \
      typedef UriQueryListStruct##PREFIX UriQueryListType; \
      \
      std::function<int(UriStateType*, UrlTextType)> parseUri;  \
      std::function<void(UriObjType*)> freeUriMembers;  \
      std::function<int(UriObjType*)> uriNormalizeSyntax; \
      typedef decltype(UriQueryListType::key) QueryListCharType;  \
      std::function<int(UriQueryListType**, int*, QueryListCharType, QueryListCharType)> uriDissectQueryMalloc; \
      std::function<void(UriQueryListType*)> uriFreeQueryList;  \
      /* add_const to support UrlTextType == tchar* & const tchar* ( api output is exactly const tchar* )*/ \
      std::function<typename base_const_ptr<UrlTextType>::type  \
        (typename base_ptr<UrlTextType>::type, UriBool, UriBreakConversion)> uriUnescapeInPlaceEx;  \
        \
      UriTypes() :  \
        parseUri(&uriParseUri##PREFIX),  \
        freeUriMembers(&uriFreeUriMembers##PREFIX),  \
        uriNormalizeSyntax(&uriNormalizeSyntax##PREFIX), \
        uriUnescapeInPlaceEx(&uriUnescapeInPlaceEx##PREFIX), \
        uriDissectQueryMalloc(&uriDissectQueryMalloc##PREFIX), \
        uriFreeQueryList(&uriFreeQueryList##PREFIX)  \
     {}


    // by default lets use ANSI api functions
    template <class UrlTextType, class Empty = void>
    struct UriTypes
    {
      typedef std::string UrlReturnType;
      URI_API_TYPES(A)
    };

    template <class UrlTextType>
    struct UriTypes<UrlTextType, typename std::enable_if<std::is_convertible<UrlTextType, const wchar_t*>::value >::type>
    {
      typedef std::wstring UrlReturnType;
      URI_API_TYPES(W)
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

  template <class UrlReturnType>
  struct UriQueryEntry
  {
    UrlReturnType key_;
    UrlReturnType value_;
  };

  template <class UriQueryListType, class UrlReturnType>
  class UriQueryListIterator :
    public std::iterator<std::forward_iterator_tag, UriQueryEntry<UrlReturnType>>
  {
    typedef UriQueryListIterator<UriQueryListType, UrlReturnType> IteratorType;
  public:
    UriQueryListIterator(UriQueryListType* list) :
      currentQueryList_(list){}

    IteratorType& operator++()
    {
      if (currentQueryList_->next != nullptr)
      {
        currentQueryList_ = currentQueryList_->next;
      }
      else
      {
        *this = IteratorType(nullptr);
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
      // supports end() == end()
      if (currentQueryList_ == right.currentQueryList_)
      {
        return true;
      }

      // comparing Iterator with end()
      if (right.currentQueryList_ == nullptr)
      {
        return false;
      }

      // comparing two "normal" iterators
      return ( currentQueryList_->key == right.currentQueryList_->key &&
        (currentQueryList_->value == right.currentQueryList_->value) &&
        (currentQueryList_->next == right.currentQueryList_->next));
    }

    bool operator!=(const IteratorType& right)
    {
      return !operator==(right);
    }

    UriQueryEntry<UrlReturnType> operator*() const
    {
      return UriQueryEntry<UrlReturnType>{
        currentQueryList_->key == nullptr ? UrlReturnType() : currentQueryList_->key,
        currentQueryList_->value == nullptr ? UrlReturnType() : currentQueryList_->value};
    }

    UriQueryEntry<UrlReturnType>* operator->()
    {
      returnObjStorage_ = std::move(operator*());
      return &returnObjStorage_;
    }

  private:
    UriQueryListType* currentQueryList_;
    UriQueryEntry<UrlReturnType> returnObjStorage_;
  };

  template <class UrlTextType>
  class UriQueryList
  {
    typedef internal::UriTypes<UrlTextType> UriApiTypes;
    typedef typename UriApiTypes::UriObjType UriObjType;
    typedef typename UriApiTypes::UriQueryListType UriQueryListType;
    typedef typename UriApiTypes::UrlReturnType UrlReturnType;
    typedef UriQueryListIterator<UriQueryListType, UrlReturnType> IteratorType;
  public:
    UriQueryList(UriQueryList&& right) :
      itemCount_(std::move(right.itemCount_)),
      uriTypes_(std::move(right.uriTypes_)),
      queryList_(std::move(right.queryList_))
    {
      // we have to cleaup right ptr, not to free it in right's destructor
      right.queryList_ = nullptr;
    }

    bool empty() const { return itemCount_ == 0; }
    int size() const { return itemCount_; }

    UriQueryList& operator=(UriQueryList&& right)
    {
      std::swap(this->itemCount_, right.itemCount_);
      std::swap(this->uriTypes_, right.uriTypes_);
      this->queryList_ = right.queryList_;
      right.queryList_ = nullptr;
      return *this;
    }

    IteratorType begin() const { return IteratorType(queryList_); };
    const IteratorType cbegin() const { return begin(); };
    IteratorType end() const { return GetEndIterator(); };
    const IteratorType cend() const { return end(); };

    // If failed returns end()
    // If succ - iterator to element found
    IteratorType findKey(UrlTextType keyStr)
    {
      for (auto item = std::begin(*this); item != std::end(*this); ++item)
      {
        if (item->key_.compare(keyStr) == 0)
        {
          return item;
        }
      }
      return GetEndIterator();
    }

    IteratorType findValue(UrlTextType valueStr)
    {
      for (auto item = std::begin(*this); item != std::end(*this); ++item)
      {
        if (item->value_.compare(valueStr) == 0)
        {
          return item;
        }
      }
      return GetEndIterator();
    }

    UriQueryList(const UriObjType& uri) :
      queryList_{},
      itemCount_{}
    {
      uriTypes_.uriDissectQueryMalloc(&queryList_, &itemCount_, uri.query.first, uri.query.afterLast);
    }

    virtual ~UriQueryList()
    {
      if (queryList_ != nullptr)
      {
        uriTypes_.uriFreeQueryList(queryList_);
      }
    }

  private:
    IteratorType GetEndIterator() const { return IteratorType(nullptr); }

    int itemCount_;
    UriApiTypes uriTypes_;
    UriQueryListType* queryList_;
  };

  // free helper functions
  template <class UrlTextType, class UrlReturnType>
  bool UnescapeString(
    UrlTextType srcStrBegin,
    UrlReturnType& retVal,
    bool plusToSpace = true,
    UriBreakConversion breakConversion = URI_BR_DONT_TOUCH)
  {
    typedef typename internal::base_const_ptr<UrlTextType>::type(*uriUnescapeInPlaceEx)
      (typename internal::base_ptr<UrlTextType>::type, UriBool, UriBreakConversion);

    UrlReturnType reslt(srcStrBegin);

    if (reslt.empty())
    {
      return false;
    }

    uriUnescapeInPlaceEx uriUnescapeInPlaceExProc =
      std::is_convertible<UrlTextType, const char*>::value ?
      (uriUnescapeInPlaceEx)&uriUnescapeInPlaceExA : (uriUnescapeInPlaceEx)&uriUnescapeInPlaceExW;

    uriUnescapeInPlaceExProc(
      &reslt.front(),
      plusToSpace ? URI_TRUE : URI_FALSE,
      breakConversion);

    retVal = std::move(reslt);
    return true;
  }

} // namespace uri_parser