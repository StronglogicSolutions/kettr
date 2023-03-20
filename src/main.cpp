#include "kettr.hpp"
#include <INIReader.h>

static auto config = INIReader{kutils::GetCWD(12) + "config/config.ini"};
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
    kettr.post("Whatsup - it's been a while");
  // {
  //   kettr.upload();
  // }
//  kutils::log(kettr.fetch(argv[3]).to_json());

  return 0;
}
