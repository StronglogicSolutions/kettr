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
//---------------------------------------------------------------------------/
size_t
get_size(std::string_view path) { return std::filesystem::file_size(path); };
//---------------------------------------------------------------------------/
auto has_error = [](const auto r) { return r.error.code != cpr::ErrorCode::OK || r.status_code >= 400; };
//---------------------------------------------------------------------------/
file_info::file_info(const std::string& path)
{
  auto filename_idx = path.find_last_of('/');
  filename_idx == std::string::npos ? 0 : filename_idx + 1;
  size         = get_size(path);
  name         = path.substr(filename_idx + 1);
  auto mime    = kutils::GetMimeType(name);
  meta         = "filename " + to_base64(name) + ",filetype " + to_base64(mime.name);
}
//---------------------------------------------------------------------------/
bool
valid_json_object(const json_t& data)
{
  return (!data.is_null() && data.is_object());
}
//---------------------------------------------------------------------------/
auto handle_error = [](const auto response)
{
  kutils::log(response.error.message);
  kutils::log(response.text);
  kutils::log(std::to_string(static_cast<int>(response.error.code)));
  return false;
};
//---------------------------------------------------------------------------/
namespace urls
{

  static const char*       login    = "https://api.gettr.com/u/user/v2/login";
  static const char*       token    = "https://api.gettr.com/v1/chat/token";
  static const char*       activity = "https://analytics-api.gettr.com/api/events/activity";
  static const char*       post     = "https://api.gettr.com/u/post";
  static const char*       media    = "https://upload.gettr.com/media/big/upload";
  static const char*       fileurl  = "https://upload.gettr.com";
  static const std::string mediaurl = "https://media.gettr.com/";
  std::string              get_user_url(const std::string& user) {
                               return "https://api.gettr.com/u/user/" + user + "/posts"; }
} // ns urls
//---------------------------------------------------------------------------/
post_t::post_t() {}
//
post_t::post_t(const json_t& data, const json_t& post)
{
  auto id   = post["activity"]["tgt_id"].get<std::string>();
  auto info = data["result"]["aux"]["post"][id];
  name      = info["uid"].get<std::string>();
  text      = info["txt"].get<std::string>();
  date      = info["udate"].get<long>();
  for (const auto& media_data : info["imgs"])
    media.push_back(urls::mediaurl + media_data.get<std::string>());
}
//---------------------------------------------------------------------------/
post_t post_t::make(const json_t& data)
{
  post_t post;
  if (valid_json_object(data))
  {
    post.name = data["uid"]  .get<std::string>();
    post.text = data["txt"]  .get<std::string>();
    post.date = data["udate"].get<long>       ();
  }
  return post;
}
//---------------------------------------------------------------------------/
bool post_t::is_valid() const
{
  return (!name.empty() && !text.empty());
}
//---------------------------------------------------------------------------/
void
post_t::print() const
{
  kutils::log("Posted: ", std::to_string(date).c_str(),
              "\nText: ", text.c_str(),
              "\nBy: "  , name.c_str());
}
//---------------------------------------------------------------------------/
json_t
post_t::to_json() const
{
  json_t json{{"user", name}, {"date", date}, {"text", text}};
  for (const auto& url : media)
    json["media"].push_back(url);
  return json;
}
//---------------------------------------------------------------------------/
posts_t::posts_t(std::string_view text)
{
  if (const auto data = json_t::parse(text); valid_json_object(data))
  for (const auto post : data["result"]["data"]["list"])
    posts.emplace_back(post_t{data, post});
}
//---------------------------------------------------------------------------/
std::string
posts_t::to_json() const
{
  json_t json = json_t::array();
  for (const post_t& post : posts)
    json.emplace_back(post.to_json());
  return json.dump();
}
//---------------------------------------------------------------------------/
response_t
kettr::do_post(std::string_view url, const json_t& body, const header_t& header) const
{
  const auto body_s = body.dump();
  header_t request_header = (header.empty()) ? get_header(body_s.size()) : header;
  return cpr::Post(
    url_t   {url},
    body_t  {body_s},
    request_header);
}
//---------------------------------------------------------------------------/
response_t
kettr::do_patch(std::string_view url, const file_t& file, const header_t& header) const
{
  return cpr::Patch(url_t{url}, cpr::Body(file), header);
}
//---------------------------------------------------------------------------/
kettr::kettr(const std::string& email, const std::string& pass)
: m_email(email),
  m_pass(pass)
{}
//---------------------------------------------------------------------------/
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
//---------------------------------------------------------------------------/
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
//---------------------------------------------------------------------------/
json_t
kettr::get_auth() const
{
  return m_tokens.is_null() ? json_t{{"user", "null"}, {"token", "null"}} :
                              json_t{{"user", m_name}, {"token", m_tokens.bearer}};
}
//---------------------------------------------------------------------------/
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
//---------------------------------------------------------------------------/
post_t
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
  header_t   header;
  if (media.size())
  {
    body["content"]["data"]["imgs"]    = json_t(media);
    body["content"]["data"]["vid_wid"] = 260;
    body["content"]["data"]["vid_hgt"] = 260;
    body["content"]["data"]["meta"]    = json_t::array();
    // body_s = body.dump();
    //---------------- Alt method below --------------------
    // auto boundary = "--WebKitFormBoundary" + to_base64(kutils::generate_random_chars());
    //      body_s   = "----" + boundary + '\n' +
    //                 "Content-Disposition: form-data; name=\"content\"" + "\n\n" +
    //                 body["content"].dump() + '\n' +
    //                 + "----" + boundary + "--";
    // header   = get_header(body_s.size());
    // header["content-type"] = "multipart/form-data; boundary=--" + boundary;
    // header["enctype"] = "multipart/form-data";
    // response = cpr::Post(url_t{urls::post}, body_t{body_s}, header);
  }

  header   = get_header(body.dump().size());
  response = do_post(urls::post, body, header);
  kutils::log<std::string>(response.text);

  if (has_error(response))
    kutils::log<std::string>(response.error.message);

  return post_t::make(json_t::parse(response.text));
}
//---------------------------------------------------------------------------/
bool
kettr::upload() const
{
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

  kutils::log("Upload patch successful");
  auto response_body = json_t::parse(response.text);
  auto file_url      = response_body["ori"].get<std::string>(); if (file_url.back() == '\n') file_url.pop_back();

  if (response_body["status"].get<int>() == 0)             // Create post
    return post("This is the best thing I've ever seen", { file_url });

  return false;
}
//---------------------------------------------------------------------------/
posts_t
kettr::fetch(std::string_view user) const
{
  auto response = cpr::Get(url_t{urls::get_user_url(kutils::ToLower(user.data()))},
                           params_t{{"max", "20"}, {"incl", "posts|userinfo|liked"}});
  if (has_error(response))
    handle_error(response);
  return posts_t{response.text};
}
