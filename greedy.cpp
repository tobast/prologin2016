#include "greedy.h"

using namespace std;

struct Pulsar {
	position pos;
	bool linked;
};

vector<Pulsar> pulsars;



vector<position> possiblePlugs() {
	vector<position> pipes = liste_tuyaux();
	vector<position> out = liste_base_baies();

	for(auto it=pipes.begin(); it != pipes.end(); ++it) {
		if(pipe_ownership(*it) > PERCENT_PIPE_FOR_OWNERSHIP) {
			out.push_back(*it);
		}
	}

	return out;
}

int vraiesPulsRestantes(const pulsar_info& p) {
	int toursRest = NB_TOURS/2 - tour_actuel();

	return min(toursRest / p.periode,
		p.pulsations_restantes);
}

double scorePulsar(const Pulsar& puls) {
	if(puls.linked)
		return 0;
	position pos = puls.pos;
	if(!est_pulsar(pos))
		return -1;
	pulsar_info infos = info_pulsar(pos);
	if(infos.pulsations_restantes == 0)
		return -1;
	double emitScore = sq(infos.puissance /
		((double)infos.periode)) * 
		(NB_TOURS/2 + 1 - tour_actuel() -
			vraiesPulsRestantes(infos) * infos.periode);
	
	vector<position> plugs = possiblePlugs();
	pair<Path,int> pth = pathToPulsar(pos,
		setOfVect(plugs));

	return emitScore / ((double)pth.second);
}

bool orderPulsar(const Pulsar& p1, const Pulsar& p2) {
	return scorePulsar(p1) < scorePulsar(p2);
}


Pulsar targetPuls;
Path worklist, shadowWorkList;
Path reinforcements;
queue<position> aspiUp;
vector<position> usedBases;

bool allLinked=false;
int doubledBackbones = 0;


Path doubleBayPlugs(const Path& path, bool reinf=false);
Path doubleBayPlugs(const Path& path, bool reinf) {
	Path out;
	auto endIt = path.end();
	for(auto it=path.begin(); it != endIt; ++it) {
		if(baseOfBay(*it).x >= 0) { // est une base
			if(reinf)
				reinforcements.push_back(*it);
			aspiUp.push(baseOfBay(*it));
			usedBases.push_back(baseOfBay(*it));
			for(int adj=0; adj < NB_ADJACENCY; adj++) {
				position nPos = *it+ADJACENCY[adj];
				if(baseOfBay(nPos).x >= 0) {
					out.push_back(nPos);
					usedBases.push_back(baseOfBay(nPos));
					if(reinf)
						reinforcements.push_back(nPos);
					for(int i=0; i < NB_ADJACENCY; i++) {
						if(type_case(nPos+ADJACENCY[i]) == BASE) {
							out.push_back(nPos+
								ADJACENCY[(i+2)%4]);
							if(reinf)
								reinforcements.push_back(nPos+
									ADJACENCY[(i+2)%4]);
							break;
						}
					}
					break;
				}
			}
		}
	}

	for(auto pos : path)
		out.push_back(pos);

	return out;
}

void doubleLinearSection(position pos, Path& out, int dir=-1,
	int side=-1);
void doubleLinearSection(position pos, Path& out,  int dir,
		int side) {
	if(dir < 0) {
		Path connected = connectedPipes(pos);
		if(connected.empty() || connected.size() > 2)
			return;
		for(int adj=0; adj < NB_ADJACENCY; adj++) {
			if(pos+ADJACENCY[adj] == connected.front()) {
				dir=adj;
				break;
			}
		}

		out.push_back(pos+ADJACENCY[(dir+1)%4]);
		doubleLinearSection(pos + ADJACENCY[dir],
			out, dir, (dir+1)%4); 
		doubleLinearSection(pos + ADJACENCY[(dir+2)%4],
			out, (dir+2)%4, (dir+1)%4);
	}
	else {
		out.push_back(pos+ADJACENCY[side]);
		if(est_tuyau(pos+ADJACENCY[dir]) &&
				connectedPipes(pos).size() == 2)
			doubleLinearSection(pos+ADJACENCY[dir],
				out, dir, side);
	}
}

/*
Path bestBackboneReinforcement() {
	vector<position> pipes = liste_tuyaux();
	SPos bays = setOfVect(liste_base_baies());
	double bestScore=0;
	Path bestPath;
	bool once=true;

	vector<position> chosen;
	for(int i=0; i < sqrt(pipes.size()); i++) {
		chosen.push_back(pipes[rand()%pipes.size()]);
	}

	for(auto it=chosen.begin(); it != chosen.end(); ++it) {
		int enemyDist = distToBase(*it, true);
		int selfDist = distToBase(*it);
		pair<Path,int> nPath = shortpath(*it, bays, true);
		int apUsed = countActionPoints(nPath.first);
		double score = ((double)(
			(selfDist - enemyDist) *
			(enemyDist - nPath.second))) /
			((double)apUsed);

		if(once || score > bestScore) {
			bestScore = score;
			bestPath = nPath.first;
			once=false;
		}
	}
	bestPath = doubleBayPlugs(bestPath, true);
	return bestPath;
}
*/

bool orderByManhattan(const position& p1, const position& p2) {
	return manhattanToBase(p1) < manhattanToBase(p2);
}
double costyReinf(const position& p) {
	position targetBase = manhattanNearestBase(p);
	int enemyDist = manhattanToBase(p, true);
	int selfDist = manhattan(p,targetBase);
	int distToOwnedBase = INFTY;
	for(position bPos : usedBases)
		distToOwnedBase = min(distToOwnedBase,
			manhattan(bPos, targetBase));
	double ownership = pipe_ownership(p);

	return ((double)distToOwnedBase) * (1. - ownership) *
		log(((double)enemyDist) / ((double)selfDist) + 1.01);
}

Path bestBackboneReinforcement() {
	vector<position> pipes = liste_tuyaux();
	sort(pipes.begin(), pipes.end(), orderByManhattan);

	int optId=-1;
	double optScore = -1;
	for(int pos=0; pos < min(100,(int)pipes.size()); pos++) {
		double score = costyReinf(pipes[pos]);
		if(score > optScore) {
			optScore = score;
			optId = pos;
		}
	}

	SPos bays = setOfVect(liste_base_baies());
	Path bestPath = shortpath(pipes[optId], bays, true).first;
	bestPath = doubleBayPlugs(bestPath, true);
	return bestPath;
}

double rankFiringPos(const position& pos, int aspi,
		const position& prev)
{
	double score = ((double)sq(aspi+1)) * avgPlasma(pos) *
		(sqrt(charges_presentes(prev)) + 1.);
	if(est_super_tuyau(pos))
		score /= 100.;
	return score;
}

pair<position,double> followSingleBackbone(int aspi,
		bool seen[][TAILLE_TERRAIN],
		const position& cPos)
{
	vector<position> adjPipes;
	for(int adj=0; adj < NB_ADJACENCY; adj++) {
		position nPos = cPos + ADJACENCY[adj];
		if(inGrid(nPos) && est_tuyau(nPos) &&
				!seen[nPos.y][nPos.x])
			adjPipes.push_back(nPos);
	}

	seen[cPos.y][cPos.x] = true;

	if(adjPipes.size() != 1)
		return make_pair(mkPos(-1,-1), -INFTY);
	
	pair<position,double> nextRes =
		followSingleBackbone(aspi, seen, adjPipes.front());

	double selfRank = rankFiringPos(cPos, aspi,
		adjPipes.front());

	if(selfRank > nextRes.second)
		return make_pair(cPos,selfRank);
	return nextRes;
}

position findFiringTarget() {
	vector<position> baseCells = liste_base(true);

	bool seen[TAILLE_TERRAIN][TAILLE_TERRAIN];
	for(int row=0;row < TAILLE_TERRAIN; row++)
		for(int col=0;col < TAILLE_TERRAIN; col++)
			seen[row][col]=false;

	position optPos = mkPos(-1,-1);
	double bestScore = -2*INFTY;
	for(position cell : baseCells) {
		int aspi = puissance_aspiration(cell);
		pair<position, double> backboneOpti =
			followSingleBackbone(aspi, seen, bayOfBase(cell));
		if(backboneOpti.second > bestScore) {
			optPos = backboneOpti.first;
			bestScore = backboneOpti.second;
		}
	}
	return optPos;
}

position findAspiPoint() {
	for(position pos: ma_base()) {
		if(find(usedBases.begin(), usedBases.end(), pos)
				!= usedBases.end())
			continue;
		if(puissance_aspiration(pos) == 0)
			continue;
		return pos;
	}
	return mkPos(-1,-1);
}

void reallocAspiPoint() {
	position aspiTakeFrom = findAspiPoint();
	if(aspiTakeFrom.x < 0) // none available
		return;

	while(!aspiUp.empty()) {
		position pos = aspiUp.front();
		if(puissance_aspiration(pos) == 5) {
			aspiUp.pop();
			continue;
		}
		
		deplacer_aspiration(aspiTakeFrom, pos);
		return;
	}
}

void acquireTarget() {
	sort(pulsars.begin(), pulsars.end(), orderPulsar);
	targetPuls = pulsars.back();
	if(pulsars.back().linked || scorePulsar(pulsars.back()) < 0)
		allLinked = true;

	pulsars.back().linked = true;

	pair<Path,int> pth = pathToPulsar(
		targetPuls.pos, setOfVect(possiblePlugs()));

	Path out = doubleBayPlugs(pth.first, true);

	worklist = out;
}

void tour() {
#ifdef PROFILING
	clock_t stClock;
#endif
	vector<position> stacked;

#ifdef PROFILING
	stClock = clock();
#endif
	reallocAspiPoint();
#ifdef PROFILING
	cerr << "reallocAspi " << ellapsed(stClock) << endl;
#endif

	while(points_action() > 0) {
		if(worklist.empty() && reinforcements.empty() &&
				stacked.empty()) {
			if(!shadowWorkList.empty()) {
				worklist = shadowWorkList;
				shadowWorkList.clear();
			}
			else if(allLinked) {
#ifdef PROFILING
				stClock = clock();
#endif
				worklist = bestBackboneReinforcement();
#ifdef PROFILING
			cerr << "bBkbReinf " << ellapsed(stClock) << endl;
#endif
			}
			else {
#ifdef PROFILING
				stClock = clock();
#endif
				acquireTarget();
#ifdef PROFILING
			cerr << "acqTarget" << ellapsed(stClock) << endl;
#endif
			}
		}

		if(!worklist.empty()) {
			position pos = worklist.back();
			worklist.pop_back();

			if(needsBuild(pos)) {
				if(type_case(pos) == DEBRIS) {
					if(points_action() > COUT_DEBLAYAGE)
						deblayer(pos);
					else
						stacked.push_back(pos);
				}
				else {
					construire(pos);
				}
			}
		}
		else {
			position pos = reinforcements.back();
			reinforcements.pop_back();

			if(est_simple_tuyau(pos))
				ameliorer(pos);
		}
	}
	for(auto it=stacked.begin(); it != stacked.end(); ++it)
		worklist.push_back(*it);
}

void repareDamage() {
	vector<position> ldestr = hist_tuyaux_detruits();
	if(!ldestr.empty()) {
		position destr = ldestr.front();
		deblayer(destr);
		construire(destr);

		if(shadowWorkList.empty() &&
				doubledBackbones < MAX_DOUBLED_BACKBONES) {
			Path doub;
			doubleLinearSection(destr, doub);
			shadowWorkList = worklist;
			worklist = doub;
			doubledBackbones++;
		}
	}
}

void killItWithFire() {
	if(points_action() < 3 || score(moi()) < CHARGE_DESTRUCTION)
		return; // ABORT MISSION, REPEAT, ABORT MISSION

	/* Determine whether we're going to fire or not.
	This event has a different probability before and after
	having linked every pulsar. */
	int odds;
	if(allLinked)
		odds = PRE_LINK_DESTROY_PROB;
	else
		odds = POST_LINK_DESTROY_PROB;

	if(rand() % odds > 0)
		return;
	
	position target = findFiringTarget();
	detruire(target);
}

namespace greedy {
void do_partie_init() {
	vector<position> puls = liste_pulsars();
	for(auto it=puls.begin(); it != puls.end(); ++it) {
		Pulsar p;
		p.pos = *it;
		p.linked=false;
		pulsars.push_back(p);
	}
}

void do_jouer_tour() {
#ifdef PROFILING
	clock_t cl = clock();
#endif
	gatherStats();
#ifdef PROFILING
	cerr << "gatherStats " << ellapsed(cl) << endl;
	cl = clock();
#endif
	repareDamage();
#ifdef PROFILING
	cerr << "repareDmg " << ellapsed(cl) << endl;
	cl = clock();
#endif
	killItWithFire();
#ifdef PROFILING
	cerr << "killItWithFire " << ellapsed(cl) << endl;
	cl = clock();
#endif
	tour();
#ifdef PROFILING
	cerr << "tour " << ellapsed(cl) << endl;
#endif
}
}
