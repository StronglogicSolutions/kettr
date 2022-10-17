#include "kettr.hpp"
#include <thread>

int main(int argc, char** argv)
{
  if (kettr kettr{argv[1], argv[2]}; kettr.login())
  {
    kutils::log<std::string>("Login successful");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    // kettr.post("It's great to be back");
    kettr.upload();
  }

  return 0;
}
