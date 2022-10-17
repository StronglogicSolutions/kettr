#include "kettr.hpp"
#include <iostream>
#include <sys/stat.h>
#include <cstdlib>
#include <filesystem>

std::string to_base64(std::string data)
{
  std::stringstream ss;
  std::system(std::string{"echo " + data + "| base64 > base64encoded"}.c_str());
  ss << std::ifstream("base64encoded").rdbuf();
  auto s = ss.str();
  if (!s.empty() && s.back() == '\n')
    s.pop_back();
  return s;
}

size_t get_size(std::string_view path) { return std::filesystem::file_size(path); };
struct file_info
{
  std::string name;
  size_t      size;
  std::string meta;

  file_info(const std::string& path)
  {
    auto filename_idx = path.find_last_of('/');
    filename_idx == std::string::npos ? 0 : filename_idx + 1;
    size         = get_size(path);
    name         = path.substr(filename_idx + 1);
    auto mime    = kutils::GetMimeType(name);
    meta         = "filename " + to_base64(name) + ",filetype " + to_base64(mime.name);
  }
};

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
  static const char* media    = "https://upload.gettr.com/media/big/upload";
  static const char* fileurl  = "https://upload.gettr.com";

} // ns urls

response_t kettr::do_post(std::string_view url, const json_t& body, const header_t& header) const
{
  const auto body_s = body.dump();
  header_t request_header = (header.empty()) ? get_header(body_s.size()) : header;
  return cpr::Post(
    url_t   {url},
    body_t  {body_s},
    request_header);
}

response_t kettr::f__post(const std::string& body, const header_t& header) const
{
  return cpr::Post(url_t {urls::post}, body_t{body}, header);
}

response_t kettr::do_patch(std::string_view url, const file_t& file, const header_t& header) const
{
  return cpr::Patch(url_t{url}, cpr::Body(file), header);
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
  return m_tokens.is_null() ? json_t{{"user", "null"}, {"token", "null"}} :
                              json_t{{"user", m_name}, {"token", m_tokens.bearer}};
}

header_t
kettr::get_header(size_t body_size, bool use_default) const
{
  return (use_default) ?
    header_t{{"x-app-auth",      get_auth().dump()},
             {"accept",          "application/json, text/plain, */*"},
             {"accept-encoding", "gzip, deflate, br"},
             {"accept-language", "en-US,en;q=0.9"},
             {"content-type",    "application/json"},
             {"content-length",  std::to_string(body_size)},
             {"origin",          "https://gettr.com"},
             {"referer",         "https://gettr.com"},
             {"sec-fetch-dest",  "empty"},
             {"sec-fetch-mode",  "cors"},
             {"sec-fetch-site",  "same-site"},
             {"sec-gpc",         "1"},
             {"ver",             "2.7.0"},
             {"user-agent",      "Mozilla/5.0 (X11; Linux x86_64) "
                                 "AppleWebKit/537.36 (KHTML, like Gecko) "
                                 "Chrome/106.0.0.0 Safari/537.36"}} :
    header_t{{"Authorization",   m_tokens.bearer},
             {"Accept",          "*/*"},
             {"Accept-Encoding", "gzip, deflate, br"},
             {"Accept-Language", "en-US,en;q=0.9"},
             {"Connection",      "keep-alive"},
             {"Content-Length",  std::to_string(body_size)},
             {"env",             "prod"},
             {"lv",              "0"},
             {"Origin",          "https://gettr.com"},
             {"Referer",         "https://gettr.com"},
             {"Sec-Fetch-Dest",  "empty"},
             {"Sec-Fetch-Mode",  "cors"},
             {"Sec-Fetch-Site",  "same-site"},
             {"Sec-GPC",         "1"},
             {"User-Agent",      "Mozilla/5.0 (X11; Linux x86_64) "
                                 "AppleWebKit/537.36 (KHTML, like Gecko) "
                                 "Chrome/106.0.0.0 Safari/537.36"},
             {"userid",          m_name}};
}

auto has_error = [](const auto r) { return r.error.code != cpr::ErrorCode::OK || r.status_code >= 400; };

bool
kettr::post(const std::string& text, const media_t& media) const
{
  auto date = kutils::get_unixtime();
  auto body = json_t{{"content", {{"data", {{"acl", {{"_t", "acl"}}},
                                           {"_t",   "post"},
                                           {"txt",   text},
                                           {"udate", date},
                                           {"cdate", date},
                                           {"uid",   m_name}}},
                                 {"aux",    "null"},
                                 {"serial", "post"}}}};
  response_t response;
  if (media.size())
  {
    body["content"]["data"]["imgs"]    = json_t(media);
    body["content"]["data"]["vid_wid"] = 260;
    body["content"]["data"]["vid_hgt"] = 260;
    body["content"]["data"]["meta"]    = json_t::array();

    //---------------- Alt method below --------------------
    // auto boundary = "--WebKitFormBoundary" + to_base64(kutils::generate_random_chars());
    // auto body_s   = "----" + boundary + '\n' +
    //                 "Content-Disposition: form-data; name=\"content\"" + "\n\n" +
    //                 body["content"].dump() + '\n' +
    //                 + "----" + boundary + "--";
    // auto header   = get_header(body_s.size());
    // header["content-type"] = "multipart/form-data; boundary=" + boundary;
    // response = f__post(body_s, header);

  }
  // else
    response = do_post(urls::post, body, get_header(body.dump().size())); // Ugly
  kutils::log<std::string>(response.text);

  if (!has_error(response))
    kutils::log<std::string>(response.error.message);
  if (const auto data = json_t::parse(response.text); valid_json_object(data))
    return (data["rc"] == "OK");
  return false;
}

bool
kettr::upload() const
{
  auto handle_error = [](const auto response)
  {
    kutils::log(response.error.message);
    kutils::log(response.text);
    kutils::log(std::to_string(static_cast<int>(response.error.code)));
    return false;
  };

  std::string upload_destination;
  std::string path  = "/home/logicp/Pictures/suffer.png";
  auto filedata     = file_info(path);
  auto file         = cpr::File(std::move(path));
  auto hdrs         = get_header(0, false);

  hdrs["filename"]        = filedata.name;
  hdrs["Upload-Length"]   = std::to_string(filedata.size);
  hdrs["Upload-Metadata"] = filedata.meta;
  hdrs["Tus-Resumable"]   = "1.0.0";

  auto response = do_post(urls::media, {}, hdrs);          // Request new resource
  if (has_error(response))
    return handle_error(response);

  if (const auto it = response.header.find("location"); it != response.header.end())
    upload_destination = urls::fileurl + it->second;

  if (upload_destination.empty())
  {
    kutils::log("Upload request accepted but returned no destination url");
    return false;
  }

  hdrs = get_header(filedata.size, false);
  hdrs["Content-Type"]  = "application/offset+octet-stream";
  hdrs["filename"]      = filedata.name;
  hdrs["Upload-Offset"] = "0";
  hdrs["Tus-Resumable"] = "1.0.0";
  response = do_patch(upload_destination, file, hdrs);     // Patch resource
  if (has_error(response))
    return handle_error(response);

  auto response_body = json_t::parse(response.text);
  auto file_url      = response_body["ori"].get<std::string>(); if (file_url.back() == '\n') file_url.pop_back();

  if (response_body["status"].get<int>() == 0)             // Create post
    return post("This is the best thing I've ever seen", { file_url });

  return false;
}
