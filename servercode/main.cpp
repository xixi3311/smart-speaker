#include <iostream>
#include "server.h"

int main()
{
    Server s;           // ��������������
    s.listen(IP, PORT); // �����ͻ��˵�����

    return 0;
}