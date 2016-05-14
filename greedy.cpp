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
		cerr << "Begin " << it->y << ' ' << it->x << endl;
		int ennemyDist = distToBase(*it, true);
		int selfDist = distToBase(*it);
		pair<Path,int> nPath = shortpath(*it, bays, true);
		cerr << "Path aquired." << endl;
		int apUsed = countActionPoints(nPath.first);
		double score = ((double)(
			(selfDist - ennemyDist) *
			(ennemyDist - nPath.second))) /
			((double)apUsed);

		if(once || score > bestScore) {
			bestScore = score;
			bestPath = nPath.first;
			once=false;
		}
		cerr << "end" << endl;
	}
	return bestPath;
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
	tour();
}
}
