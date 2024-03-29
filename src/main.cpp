#include "kettr.hpp"
#include <INIReader.h>

static auto config = INIReader{kutils::GetCWD(12) + "../config/config.ini"};
bool config_valid = []
{
  if (config.ParseError() < 0)
    throw std::runtime_error{"Config path failed"};
  return true;
}();


std::string
get_config_value(const char* key)
{
  return config.GetString("kettr", key, "");
}

int main(int argc, char** argv)
{
  kettr kettr{get_config_value("name"), get_config_value("pass")};
  if (kettr.login())
  {
    const auto post = kettr.post("Whatsup - it's been a while");
    if (post.is_valid())
      kutils::log(post.to_json());
    else
      throw std::invalid_argument{"Post failed"};
  }
  throw std::invalid_argument{"Login failed"};

  return 0;
}
