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

double scorePulsar(const Pulsar& puls) {
	if(puls.linked)
		return 0;
	position pos = puls.pos;
	if(!est_pulsar(pos))
		return -1;
	pulsar_info infos = info_pulsar(pos);
	double emitScore = infos.puissance /
		((double)infos.periode) * infos.pulsations_restantes;

	vector<position> plugs = possiblePlugs();
	pair<Path,int> pth = shortpath(pos+ADJACENCY[0],
		setOfVect(plugs));

	return pth.second * emitScore;
}

bool orderPulsar(const Pulsar& p1, const Pulsar& p2) {
	return scorePulsar(p1) < scorePulsar(p2);
}

Pulsar targetPuls;
Path worklist;

bool haveANap=false;

void acquireTarget() {
	sort(pulsars.begin(), pulsars.end(), orderPulsar);
	targetPuls = pulsars.back();
	if(pulsars.back().linked)
		haveANap = true;

	pulsars.back().linked = true;

	pair<Path,int> pth = pathToPulsar(
		targetPuls.pos, setOfVect(possiblePlugs()));

	worklist = pth.first;
}

void tour() {
	vector<position> stacked;
	while(points_action() > 0 && !haveANap) {
		if(worklist.empty() && stacked.empty())
			acquireTarget();

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
	tour();
}
}
