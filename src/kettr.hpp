#pragma once

#include <string_view>
#include <string>

struct tokens
{
  std::string bearer;
  std::string refresh;
};

class kettr
{
public:
  kettr(std::string_view email, std::string_view pass);
  bool login();
  bool refresh();
  // bool post (std::string_view) const;
  bool post(const std::string& text) const;
  std::string get_auth() const;

private:
  std::string_view m_email;
  std::string_view m_pass;
  std::string      m_name;
  tokens           m_tokens;
};
