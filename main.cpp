volatile int hihi = 0;
struct TestInitArray
{
    TestInitArray()
    {
        hihi = 0xc0ffee;
    }
};

TestInitArray test;

int main()
{
    return 0;
}