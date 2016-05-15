#pragma once

#include "prologin.hh"
#include <vector>
#include <set>
#include <queue>
#include <utility>
#include <iostream>
#include <functional>
#include <ctime>

typedef std::vector<position> Path;
typedef std::set<position> SPos;

const int INFTY = 1<<16;

inline double sq(double x) {
	return x*x;
}

inline position operator+(const position& a, const position& b) {
	position out;
	out.x = a.x+b.x;
	out.y = a.y+b.y;
	return out;
}
inline position operator-(const position& a, const position& b) {
	position out;
	out.x = a.x-b.x;
	out.y = a.y-b.y;
	return out;
}

inline position mkPos(int x, int y) {
	position out;
	out.x=x;
	out.y=y;
	return out;
}

const position ADJACENCY[4] = { mkPos(0,1),
	mkPos(1,0),
	mkPos(0,-1),
	mkPos(-1,0) };
const int NB_ADJACENCY = 4;

double pipe_ownership(const position& pos);

std::pair<Path,int> shortpath(
	const position& target, const SPos& from,
	bool newBackbone = false);
std::pair<Path,int> pathToPulsar(
	const position& puls, const SPos& from);

bool inGrid(const position& pos);

std::set<position> setOfVect(
	const std::vector<position>& v);

int countActionPoints(Path pth);

std::vector<position> liste_base(bool adv=false);
position bayOfBase(position pos);
position baseOfBay(position pos);
std::vector<position> liste_base_baies(bool adv=false);

int distToBase(position pos, bool adv=false);

bool inGrid(const position& pos);
bool buildable(const position& pos);
bool needsBuild(const position& pos);

Path connectedPipes(const position& pos);

int manhattan(const position& p1, const position& p2);
int manhattanToBase(const position& pos, bool adv=false);
position manhattanNearestBase(const position& pos,
	bool adv=false);

double ellapsed(clock_t from);
