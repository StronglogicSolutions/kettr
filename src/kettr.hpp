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
using params_t   = cpr::Parameters;
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

struct post_t
{
  post_t(const json_t& data, const json_t& post);
  void        print()   const;
  json_t      to_json() const;

  std::string name;
  std::string text;
  long        date;
  media_t     media;
};

struct file_info
{
  file_info(const std::string& path);
  std::string name;
  size_t      size;

  std::string meta;
};
extern bool
valid_json_object(const json_t& data);
struct posts_t
{
  using posts_data_t = std::vector<post_t>;
  posts_t(std::string_view text);

std::string
to_json() const;

  posts_data_t posts;
};

class kettr
{
public:
  kettr(std::string_view email, std::string_view pass);

  bool       login();
  bool       refresh();
  bool       post(const std::string& text, const media_t& media = {})                             const;
  bool       upload()                                                                             const;
  posts_t    fetch(std::string_view user)                                                         const;

private:
  json_t     get_auth()                                                                           const;
  header_t   get_header(size_t body_size, bool use_default = true)                                const;
  response_t do_post (std::string_view url, const json_t& body = {}, const header_t& header = {}) const;
  response_t do_patch(std::string_view url, const file_t& file,      const header_t& header)      const;

  std::string_view m_email;
  std::string_view m_pass;
  std::string      m_name;
  tokens           m_tokens;
};
