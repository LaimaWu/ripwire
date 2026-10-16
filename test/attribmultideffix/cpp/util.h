#pragma once
#include <string>
void CanonicalizePath(std::string* path, unsigned long* bits);
void CanonicalizePath(char* path, unsigned long* len, unsigned long* bits);
void Normalize(std::string* path);
void Normalize(char* path);
