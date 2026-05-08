#ifndef OPTIMIZE_H
#define OPTIMIZE_H

#include <string>
#include <vector>
#include "common.h"

void optimizeAll(std::vector<IRInstr>& tac);

void printTAC(const std::vector<IRInstr>& tac, const std::string& title);

#endif