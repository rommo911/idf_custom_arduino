
#ifndef string_replace_h
#define string_replace_h
#pragma once

#include <string>
namespace std
{

    class str
    {
    public:
        static IRAM_ATTR const std::string to_lower_copy(const std::string &str)
        {
            std::string ret = str;
            for (auto &c : ret)
            {
                c = std::tolower(c);
            }
            return ret;
        }
        static IRAM_ATTR void to_lower(std::string &str)
        {
            for (auto &c : str)
            {
                c = std::tolower(c);
            }
        }
        static void to_upper(std::string &str)
        {
            for (auto &c : str)
            {
                c = std::toupper(c);
            }
        }
        static std::string to_upper_copy(const std::string &str)
        {
            std::string ret = str;
            for (auto &c : ret)
            {
                c = std::toupper(c);
            }
            return ret;
        }
        static void replace(std::string &target, char find, char replace)
        {
            std::replace(target.begin(), target.end(), find, replace);
        }
        static IRAM_ATTR bool replace(std::string &target, const std::string &from, const std::string &to)
        {
            size_t start_pos = target.find(from);
            if (start_pos == std::string::npos)
                return false;
            target.replace(start_pos, from.length(), to);
            return true;
        }
        static IRAM_ATTR std::string replace_copy(const std::string &target, const std::string &from, const std::string &to)
        {
            size_t start_pos = target.find(from);
            if (start_pos == std::string::npos)
                return std::string("");
            auto ret = target;
            ret.replace(start_pos, from.length(), to);
            return ret;
        }
        static bool replace_ignore_case(std::string &target, const std::string &from, const std::string &to)
        {
            std::string Newtarget = to_lower_copy(target);
            const auto newfrom = to_lower_copy(from);
            const auto newTo = to_lower_copy(to);
            if (replace(Newtarget, newfrom, newTo))
            {
                target = Newtarget;
                return true;
            }
            return false;
        }
        bool equals_ignore_case(const std::string &s, const std::string &d)
        {
            const auto l_s = to_lower_copy(s);
            const auto l_d = to_lower_copy(d);
            return l_s == l_d;
        }

        static bool starts_with(std::string_view str, std::string_view prefix)
        {
            return str.size() >= prefix.size() && 0 == str.compare(0, prefix.size(), prefix);
        }

        static bool starts_with(const std::string &str, const char *prefix, unsigned prefixLen)
        {
            return str.size() >= prefixLen && 0 == str.compare(0, prefixLen, prefix, prefixLen);
        }

        static bool starts_with(const std::string &str, const char *prefix)
        {
            return starts_with(str, prefix, std::string::traits_type::length(prefix));
        }

        static bool ends_with(std::string_view str, std::string_view suffix)
        {
            return str.size() >= suffix.size() && 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
        }
        static bool ends_with(const std::string &str, const char *suffix, unsigned suffixLen)
        {
            return str.size() >= suffixLen && 0 == str.compare(str.size() - suffixLen, suffixLen, suffix, suffixLen);
        }

        static bool ends_with(const std::string &str, const char *suffix)
        {
            return ends_with(str, suffix, std::string::traits_type::length(suffix));
        }

        // trim from end (in place)
        static inline void trim(std::string &s)
        {
            s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch)
                                            { return !std::isspace(ch); }));
        }

        // trim from start (copying)
        static inline std::string trim_copy(const std::string s)
        {
            auto ret = s;
            trim(ret);
            return ret;
        }
        static inline int compare_to(const std::string &s, const std::string &d)
        {
            return strcmp(s.c_str(), d.c_str());
        }
        static inline int index_of(const std::string &str, const char *c, std::size_t pos = 0)
        {
            return str.find(c, pos);
        }
        static inline int index_of(const std::string &str, const std::string &c, std::size_t pos = 0)
        {
            return str.find(c, pos);
        }
        static inline int last_ndex_of(const std::string &str, const char *c, const std::size_t pos = std::string::npos)
        {
            return str.find_last_of(c, pos);
        }
        static inline void substr(std::string &str, const std::size_t pos, const std::size_t len = -1)
        {
            str.substr(pos, (len > 0) ?len : str.length() );
        }
    };
} // namespace std

#endif