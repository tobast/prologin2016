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
Path worklist;

bool allLinked=false;

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

void acquireTarget() {
	sort(pulsars.begin(), pulsars.end(), orderPulsar);
	targetPuls = pulsars.back();
	if(pulsars.back().linked || scorePulsar(pulsars.back()) < 0)
		allLinked = true;

	pulsars.back().linked = true;

	pair<Path,int> pth = pathToPulsar(
		targetPuls.pos, setOfVect(possiblePlugs()));

	worklist = pth.first;
}

void tour() {
	vector<position> stacked;
	while(points_action() > 0) {
		if(worklist.empty() && stacked.empty()) {
			if(allLinked)
				worklist = bestBackboneReinforcement();
			else
				acquireTarget();
		}

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
	for(auto it=stacked.begin(); it != stacked.end(); ++it)
		worklist.push_back(*it);
}

void repareDamage() {
	vector<position> ldestr = hist_tuyaux_detruits();
	if(!ldestr.empty()) {
		position destr = ldestr.front();
		deblayer(destr);
		construire(destr);
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
	
	/*
	vector<position> bays = liste_base_baies(true);
	vector<position> targets;
	for(position bay : bays) {
		if(est_simple_tuyau(bay))
			targets.push_back(bay);
	}

	position target = targets[rand() % targets.size()]; */
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
	gatherStats();
	repareDamage();
	killItWithFire();
	tour();
}
}
