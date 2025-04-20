#pragma once

#include <set>
#include <sstream>
#include <string>
#include <algorithm>


namespace ed
{
    template <typename CharT_,
              typename Traits_ = std::char_traits<CharT_>,
              typename Alloc_ = std::allocator<CharT_>>
    std::set<std::basic_string<CharT_, Traits_, Alloc_>>
    Split(const std::basic_string<CharT_, Traits_, Alloc_>& s, CharT_ delimiter)
    {
        std::set<std::basic_string<CharT_, Traits_, Alloc_>> result;
        std::basic_stringstream<CharT_, Traits_, Alloc_> ss(s);
        std::basic_string<CharT_, Traits_, Alloc_> item;

        while (std::getline(ss, item, delimiter))
        {
            result.insert(item);
        }
        return result;
    }

    template <typename CharT_,
              typename Traits_ = std::char_traits<CharT_>,
              typename Alloc_ = std::allocator<CharT_>>
    std::basic_string<CharT_, Traits_, Alloc_>
    Merge(const std::set<std::basic_string<CharT_, Traits_, Alloc_>>& st, CharT_ delimiter)
    {
        std::basic_string<CharT_, Traits_, Alloc_> result;
        auto it = st.begin();
        while (it != st.end())
        {
            result += *it;
            if (++it != st.end())
            {
                result.push_back(delimiter);
            }
        }
        return result;
    }

    inline std::string WString2StringTruncate(const std::wstring& str)
    {
        std::string result;
        result.reserve(str.size());

        std::ranges::for_each(str, [&result](wchar_t wc) {
            result += static_cast<char>(wc);
            });

        return result;
    }


}
