#pragma once
#include <vector>
#include <algorithm>
#include "overlay.h"
#include "prologin.hh"

using namespace std;

struct CellStats {
	CellStats() : plasmaTransited() {}
	vector<int> plasmaTransited;
};

void gatherStats();

double avgPlasma(position pos, int turns=10);

