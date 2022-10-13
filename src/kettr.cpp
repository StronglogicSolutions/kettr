#include "kettr.hpp"
#include <cpr/cpr.h>
#include <iostream>

using response_t = cpr::Response;
using header_t   = cpr::Header;
using body_t     = cpr::Body;
using url_t      = cpr::Url;

static json_t null_auth = json_t{{"user", "null"}, {"token", "null"}};

static bool valid_json_object(const json_t& data)
{
  return (!data.is_null() && data.is_object());
}

namespace urls
{
  static const char* login    = "https://api.gettr.com/u/user/v2/login";
  static const char* token    = "https://api.gettr.com/v1/chat/token";
  static const char* activity = "https://analytics-api.gettr.com/api/events/activity";
  static const char* post     = "https://api.gettr.com/u/post";

} // ns urls

response_t do_post(std::string_view url, const json_t& body = {}, const json_t& auth = null_auth)
{
  const auto body_s = body.dump();
  return cpr::Post(
    url_t   {url},
    body_t  {body_s},
    header_t{{"x-app-auth",      auth.dump()},
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
  auto response = do_post(urls::login, json_t{{"content", {{"email", m_email}, {"pwd", m_pass}, {"sms", ""}}}});
  kutils::log<std::string>(response.text);

  if (const auto data = json_t::parse(response.text); valid_json_object(data))
  {
    m_tokens.bearer  = data["result"]["token"];
    m_tokens.refresh = data["result"]["rtoken"];
    m_name           = data["result"]["user"]["username"];
  }

  return response.status_code < 400;
}

bool
kettr::refresh()
{
 auto response = do_post(urls::token, {}, get_auth());
 kutils::log<std::string>(response.text);
 if (const auto data = json_t::parse(response.text); valid_json_object(data))
   m_tokens.refresh  = data["result"]["token"];
  else
    return false;
  return true;
}

json_t
kettr::get_auth() const
{
  return json_t{{"user", m_name}, {"token", m_tokens.bearer}};
}

auto has_error = [](const auto r) { return r.error.code != cpr::ErrorCode::OK; };

bool
kettr::post(const std::string& text) const
{

  auto date = kutils::get_unixtime();
  auto response = do_post(urls::post,
    json_t{{"content", {{"data", {{"acl", {{"_t", "acl"}}},
                                 {"_t", "post"},
                                 {"txt", text},
                                 {"udate", date},
                                 {"cdate", date},
                                 {"uid", m_name}}},
                        {"aux", "null"},
                        {"serial", "post"}}}}, get_auth());
  kutils::log<std::string>(response.text);

  if (!has_error(response))
    kutils::log<std::string>(response.error.message);
  if (const auto data = json_t::parse(response.text); valid_json_object(data))
    return (data["rc"] == "OK");
  return false;
}
