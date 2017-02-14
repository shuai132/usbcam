#include "mytools.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

/**
 * @brief  执行一个shell命令 输出结果逐行存储在resvec中 并返回行数
 * @date   2017-01-04
 * @author LiuShuai
 */
int myexec(const char *cmd, vector<string> &resvec, bool dispResult)
{
    resvec.clear();
    FILE *pp = popen(cmd, "r");         //建立管道
    if(!pp) {
        return -1;
    }
    char tmp[1024];                     //设置一个合适的长度，以存储每一行输出
    while(fgets(tmp, sizeof(tmp), pp) != NULL) {
        if(tmp[strlen(tmp) - 1] == '\n') {
            tmp[strlen(tmp) - 1] = '\0';//去除换行符
        }
        if(dispResult) {
            cout<<tmp<<endl;
        }
        resvec.push_back(tmp);
    }
    pclose(pp); //关闭管道
    return resvec.size();
}

/**
 * @brief  执行一个shell命令 并检查输出结果是否含有指定的字符串
 * @date   2017-01-04
 * @author LiuShuai
 */
bool myexec_find(const char *cmd, const char *str, bool dispResult)
{
    vector<string> return_cmd;
    int num = myexec(cmd, return_cmd, dispResult);

    for(int i=0; i<num; i++)
    {
        string _str = return_cmd.at(i);
        auto a = _str.find(str);
        if(a != std::string::npos) {
            return true;
        }
    }
    return false;
}

/**
 * @brief  大端模式返回true
 * @date   2017-01-22
 * @author LiuShuai
 */
bool isBigEndian()
{
    uint16_t a = 0x1234;
    uint8_t  b = *(uint8_t *)&a;
    if(b == 0x12) {
        return true;
    }else {
        return false;
    }
}

/**
 * @brief  返回jpg中去除多余信息后的大小
 * @date   2017-01-22
 * @author LiuShuai
 */
size_t jpgClean(const unsigned char *data, size_t len)
{
    uint8_t EOI[2] = {0xff, 0xd9};

    for(size_t i=0; i<len; i++)
    {
        if(*data++ == EOI[0]) {
            if(*data == EOI[1]) {
                if(i+1 <= len) {
                    return (i+1);
                }
            }
        }
    }
    return len;
}
