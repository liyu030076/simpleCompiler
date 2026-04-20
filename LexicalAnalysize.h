#ifndef LEXICAL_ANALYSIZE_H
#define LEXICAL_ANALYSIZE_H

#include <string>
#include <utility>
#include <vector>

enum TokenCategory;

std::vector<std::pair<std::string, TokenCategory> > LexicalAnalysis(const std::string& input);

#endif 