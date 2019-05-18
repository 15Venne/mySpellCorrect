#include"cppjieba.h"

using std::cout;
using std::endl;
using std::cin;
using std::string;

int test0()
{
    std::string word;
    while(cin >> word)
    {
        cout << cutCword(word) << endl;
        cout << endl;
    }
    return 0;
}


int main()
{
    string s1("中国人");
    cout << s1.size() << endl;
    printf("%c%c%c\n", s1[0], s1[1], s1[2]);
}
