#include <iostream>
#include <string>
#include "../../include/my_debug.h"
//#include "../../include/my_buffer.h"

using namespace std;

void ffs() {}
int main() {
  constexpr size_t sz = sizeof(size_t);
  SHOW("SHOWing size of size_t is:", sz, "bytes");
  cout << "done1" << endl;

  TRACE("Sizeof  size of size_t is: %d bytes\n", sz);
  cout << "done2" << endl;
  ASSERT(sizeof(size_t) == 32 / 8);
  SHOW("The answer to my sum is:", 32 / 8);

  cout << "done 3" << endl;
  PRINT("The answer is: ", sz * 8, "bit compiler");
  cout << "done 4" << endl;
  return 0;
}
