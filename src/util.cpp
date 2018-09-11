#include "tntn/util.h"
#include <cstring>

namespace tntn {

static bool is_delimiter(const char c, const char* delimiters, const size_t num_delimiters)
{
    for(size_t i = 0; i < num_delimiters; i++)
    {
        if(c == delimiters[i])
        {
            return true;
        }
    }
    return false;
}

static void tokenize_impl(const char* s,
                          const size_t s_size,
                          std::vector<std::string>& out_tokens,
                          const char* delimiters)
{
    const char default_delimiters[] = " \n\r\t\0";
    size_t num_delimiters = 0;
    if(!delimiters)
    {
        delimiters = default_delimiters;
        num_delimiters = sizeof(default_delimiters) - 1;
    }
    else
    {
        num_delimiters = std::strlen(delimiters);
    }

    out_tokens.clear();

    std::string token;
    size_t pos = 0;

state_in_space_label:
    for(; pos < s_size; pos++)
    {
        const char c = s[pos];
        if(is_delimiter(c, delimiters, num_delimiters))
        {
            continue;
        }
        else
        {
            //switch to token state
            goto state_in_token_label;
        }
    }

state_in_token_label:
    for(; pos < s_size; pos++)
    {
        const char c = s[pos];
        if(is_delimiter(c, delimiters, num_delimiters))
        {
            //swith to delimiter state, push token to output container
            if(!token.empty())
            {
                out_tokens.push_back(std::move(token));
            }
            token.clear();
            goto state_in_space_label;
        }
        else
        {
            //store char in temporary token
            token.push_back(c);
        }
    }

    //also store last token
    if(!token.empty())
    {
        out_tokens.push_back(std::move(token));
    }
}

void tokenize(const std::string& s, std::vector<std::string>& out_tokens, const char* delimiters)
{
    tokenize_impl(s.c_str(), s.size(), out_tokens, delimiters);
}

void tokenize(const char* s,
              size_t s_size,
              std::vector<std::string>& out_tokens,
              const char* delimiters)
{
    tokenize_impl(s, s_size, out_tokens, delimiters);
}

void tokenize(const char* s, std::vector<std::string>& out_tokens, const char* delimiters)
{
    tokenize_impl(s, strlen(s), out_tokens, delimiters);
}

} //namespace tntn
