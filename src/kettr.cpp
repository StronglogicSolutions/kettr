#include "kettr.hpp"
#include <nlohmann/json.hpp>
#include <cpr/cpr.h>
#include <iostream>

template <typename T>
constexpr bool is_string_type = std::is_convertible_v<T, std::string_view>;
template <typename T>
void log(T s)
{
  if constexpr (is_string_type<T>)
    std::cout << s << std::endl;
}

template void log(std::string      s);
template void log(std::string_view s);

using response_t = cpr::Response;
using header_t   = cpr::Header;
using body_t     = cpr::Body;
using url_t      = cpr::Url;
using json_t     = nlohmann::json;

namespace urls
{
  static const char* login = "https://api.gettr.com/u/user/v2/login";
} // ns urls

response_t post(std::string_view url, const json_t& body = {})
{
  const auto body_s = body.dump();
  return cpr::Post(
    url_t   {url},
    body_t  {body_s},
    header_t{{"x-app-auth",      json_t{{"user", "null"}, {"token", "null"}}.dump()},
             {"accept",          "application/json, text/plain, */*"},
             {"accept-encoding", "gzip, deflate, br"},
             {"accept-language", "en-US,en;q=0.9"},
             {"Content-type",    "application/json"},
             {"content-length",  std::to_string(body_s.size())},
             {"origin",          "https://gettr.com"},
             {"referer",         "https://gettr.com"},
             {"sec-fetch-dest",  "empty"},
             {"sec-fetch-mode",  "cors"},
             {"sec-fetch-site",  "same-site"},
             {"sec-gpc",         "1"},
             {"ver",             "2.7.0"},
             {"user-agent:",     "Mozilla/5.0 (X11; Linux x86_64) "
                                 "AppleWebKit/537.36 (KHTML, like Gecko) "
                                 "Chrome/106.0.0.0 Safari/537.36"}});
}

kettr::kettr(std::string_view email, std::string_view pass)
: m_email(email),
  m_pass(pass)
{}

bool
kettr::login()
{
  auto response = post(urls::login, json_t{{"content", {{"email", m_email}, {"pwd", m_pass}, {"sms", ""}}}});
  log(response.text);
  return response.status_code > 400;
}
