#include "kettr.hpp"

int main(int argc, char** argv)
{
  kettr kettr{argv[1], argv[2]};
  kutils::log(kettr.fetch(argv[3]).to_json());

  return 0;
}
