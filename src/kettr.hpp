#pragma once

#include <string_view>

class kettr
{
public:
  kettr(std::string_view email, std::string_view pass);
  bool login();

private:
  std::string_view m_email;
  std::string_view m_pass;
};
