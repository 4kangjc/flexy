#include <flexy/schedule/async_io.h>

using flexy::async_cin;
using flexy::async_cout;

void test() {
    std::string s1, s2;
    async_cin >> s1;
    async_cout << s1 << std::endl;

    async_cin.Scan(s1, s2);
    async_cout.Print(s1, '\n', s2, '\n');
}

void sleep_print() {
    static int i = 10;
    while (i--) {
        sleep(1);
        printf("\nhello async io\n");
    }
}

int main() {
    flexy::IOManager iom;
    go test;
    go sleep_print;
}