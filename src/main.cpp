#include "kettr.hpp"

int main(int argc, char** argv)
{
  // if (kettr kettr{argv[1], argv[2]}; kettr.login())
  // {
  //   kutils::log<std::string>("Login successful");
  //   std::this_thread::sleep_for(std::chrono::seconds(1));
  //   // kettr.post("It's great to be back");
  //   kettr.upload();
  // }
  kettr kettr{argv[1], argv[2]};
  for (const auto& post : kettr.fetch(argv[3]))
  {
    kutils::log("Printed:\n");
    post.print();
    kutils::log("As JSON:\n");
    kutils::log(post.to_json());
  }

  return 0;
}
