#pragma once
#include <kutils.hpp>
#include <nlohmann/json.hpp>
#include <string_view>
#include <string>

using json_t     = nlohmann::json;

struct tokens
{
  std::string bearer;
  std::string refresh;
};

class kettr
{
public:
  kettr(std::string_view email, std::string_view pass);
  bool   login();
  bool   refresh();
  bool   post(const std::string& text) const;
  json_t get_auth() const;

private:
  std::string_view m_email;
  std::string_view m_pass;
  std::string      m_name;
  tokens           m_tokens;
};
