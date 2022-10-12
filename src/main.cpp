#include "kettr.hpp"

int main(int argc, char** argv)
{

  kettr{argv[1], argv[2]}.login();
  return 0;
}
