#include "overlay.h"
using namespace std;

struct BfsElem {
	BfsElem() {}
	BfsElem(position pos, int depth) : pos(pos),depth(depth) {}
	position pos;
	int depth;
	bool operator<(const BfsElem& e) const {
		return depth < e.depth;
	}
};

double pipe_ownership(const position& pos) {
	if(type_case(pos) == BASE) {
		if(proprietaire_base(pos) == moi())
			return 1;
		return 0;
	}

	vector<position> nPos = directions_plasma(pos);
	double out=0;
	for(auto it=nPos.begin(); it != nPos.end(); ++it) {
		out += pipe_ownership(*it) / ((double)nPos.size());
	}
	return out;
}
	

int posCost(const position& p) {
	switch(type_case(p)) {
		case VIDE: return 1;
		case TUYAU: return 0;
		case SUPER_TUYAU: return 0;
		case DEBRIS: return 2;
		case PULSAR: 
		case BASE:
		case INTERDIT: return INFTY;
	}
	return INFTY;
}

position addAdj(const position& p, int i) {
	return p+ADJACENCY[i];
}

std::pair<Path,int> shortpath(
	const position& target, const SPos& from)
{
	int seen[TAILLE_TERRAIN][TAILLE_TERRAIN];
	for(int row=0; row < TAILLE_TERRAIN; row++) {
		for(int col=0; col < TAILLE_TERRAIN; col++) {
			seen[row][col] = -1;
		}
	}
	
	priority_queue<BfsElem> process;
	process.push(BfsElem(target, 0));

	position fromPos = {-1,-1};

	while(!process.empty()) {
		BfsElem cElt = process.top();
		process.pop();

		if(!inGrid(cElt.pos) ||
				seen[cElt.pos.y][cElt.pos.x] >= 0)
			continue;
		seen[cElt.pos.y][cElt.pos.x] = - cElt.depth;

		if(from.find(cElt.pos) != from.end()) {
			fromPos = cElt.pos;
			break;
		}


		for(int i=0; i < NB_ADJACENCY; i++) {
			position nPos = cElt.pos + ADJACENCY[i];
			if(posCost(nPos) == INFTY)
				continue;
			int nCost = -cElt.depth + 1 + posCost(nPos);
			process.push(BfsElem(nPos, -nCost));
		}	
	}
	
	if(fromPos.x < 0)
		return make_pair(Path(),INFTY); // no path found

	Path out;
	out.push_back(fromPos);
	while(!(out.back() == target)) {
		position bPos=out.back();
		for(int i=0; i < NB_ADJACENCY; i++) {
			position cPos = addAdj(out.back(),i);
			if(!buildable(cPos))
				continue;
			if(seen[cPos.y][cPos.x] < seen[bPos.y][bPos.x] &&
					seen[cPos.y][cPos.x] >= 0) {
				bPos = cPos;
			}
		}
		out.push_back(bPos);
	}
	
	return make_pair(out, seen[fromPos.y][fromPos.x]);
}

std::pair<Path,int> pathToPulsar(
		const position& puls, const SPos& from) {
	int optScore = INFTY+2, optAdj=-1;
	Path optPath;
	for(int adj=0; adj < NB_ADJACENCY; adj++) {
		pair<Path,int> pth = shortpath(puls+ADJACENCY[adj],from);
		if(pth.second <  optScore) {
			optScore = pth.second;
			optAdj = adj;
			optPath = pth.first;
		}
	}

	SPos nFrom = from;
	for(auto it=optPath.begin(); it != optPath.end(); ++it)
		nFrom.insert(*it);
	
	Path circPath;
	for(int adj=0; adj < NB_ADJACENCY; adj++) {
		if(adj == optAdj)
			continue;
		pair<Path,int> addPth=shortpath(puls+ADJACENCY[adj],
			nFrom);
		Path& pth = addPth.first;
		for(auto it=pth.begin(); it != pth.end(); ++it) {
			circPath.push_back(*it);
			nFrom.insert(*it);
		}	
	}

	copy(optPath.begin(), optPath.end(), back_inserter(circPath));

	return make_pair(circPath,optScore);
}

bool inGrid(const position& pos) {
	return pos.x > 0 && pos.x < TAILLE_TERRAIN-1 &&
		pos.y > 0 && pos.y < TAILLE_TERRAIN-1;
}
bool buildable(const position& pos) {
	if(!inGrid(pos))
		return false;
	case_type ctype = type_case(pos);
	return ctype != BASE && ctype != INTERDIT;
}
bool needsBuild(const position& pos) {
	case_type ctype = type_case(pos);
	return (ctype == VIDE || ctype == DEBRIS);
}

set<position> setOfVect(const vector<position>& v) {
	set<position> out;
	for(auto it=v.begin(); it != v.end(); ++it)
		out.insert(*it);
	return out;
}

std::vector<position> liste_base_baies() {
	std::vector<position> baies, base = ma_base();
	for(auto it=base.begin(); it != base.end(); ++it) {
		for(int adj=0; adj < NB_ADJACENCY; adj++) {
			position nPos = *it + ADJACENCY[adj];
			if(buildable(nPos))
				baies.push_back(nPos);
		}
	}
	return baies;
}

