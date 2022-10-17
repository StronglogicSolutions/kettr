#pragma once

#include <cpr/cpr.h>
#include <kutils.hpp>
#include <nlohmann/json.hpp>
#include <string_view>
#include <string>

using response_t = cpr::Response;
using header_t   = cpr::Header;
using body_t     = cpr::Body;
using file_t     = cpr::File;
using url_t      = cpr::Url;
using json_t     = nlohmann::json;
using media_t    = std::vector<std::string>;

struct tokens
{
  std::string bearer;
  std::string refresh;
  bool is_null() const
  {
    return bearer.empty();
  }
};

struct post
{

};

using posts_t = std::vector<post>;

class kettr
{
public:
  kettr(std::string_view email, std::string_view pass);
  bool       login();
  bool       refresh();
  bool       post(const std::string& text, const media_t& media = {})                             const;
  bool       upload()                                                                             const;
  posts_t    fetch(std::string_view user)                                                         const;
  json_t     get_auth()                                                                           const;
  header_t   get_header(size_t body_size, bool use_default = true)                                const;
  response_t do_post (std::string_view url, const json_t& body = {}, const header_t& header = {}) const;
  response_t f__post (const std::string& body, const header_t& header = {})                       const;
  response_t do_patch(std::string_view url, const file_t& file,      const header_t& header)      const;

private:
  std::string_view m_email;
  std::string_view m_pass;
  std::string      m_name;
  tokens           m_tokens;
};
