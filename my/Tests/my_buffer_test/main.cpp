#include <iostream>
#include <string>
#include "../../include/my_debug.h"
#include "../../include/my_membuf.h"

using namespace std;

void test_buf() {
    my::mem_buf<unsigned char> buf;
    ASSERT(buf.capacity() == 8192);
    ASSERT(buf.size() == 0);
    buf.resize(buf.capacity());
    ASSERT(buf.size() == buf.capacity() && buf.capacity() == 8192);
	typedef my::mem_view<my::mem_buf<unsigned char> > view_t;
    view_t v = view_t(buf, 0, 10);
}
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

    test_buf();

    return 0;
}
