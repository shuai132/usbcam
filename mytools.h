#ifndef MYTOOLS_H
#define MYTOOLS_H

#include <iostream>
#include <vector>
using namespace std;

int myexec(const char *cmd, vector<string> &resvec, bool dispResult = false);
bool myexec_find(const char *cmd, const char *str, bool dispResult = false);

bool isBigEndian();

size_t jpgClean(const unsigned char *data, size_t len);

#endif // MYTOOLS_H
