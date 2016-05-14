#include "naive.h"
#include <set>

using namespace std;

position target = {-1,-1};
Path toBuild = Path();

void findTarget() {
	set<position> baiesBase = setOfVect(
		liste_base_baies());
	vector<position> pulsars = liste_pulsars();
	Path bestPath = Path();
	int bestScore = INFTY;
	for(auto it=pulsars.begin(); it != pulsars.end(); ++it) {
		pair<Path,int> pth = shortpath(*it + ADJACENCY[0],
			baiesBase);
		if(bestScore > pth.second) {
			bestScore = pth.second;
			bestPath = pth.first;
		}
	}
	toBuild = bestPath;
}

void buildStuff() {
	while(points_action() > 0) {
		cerr << moi() << " loopin' to build stuff." << endl;
		cerr << points_action() << endl;
		if(toBuild.empty()) { // No target
			findTarget();
		}
		if(toBuild.empty()) {
//			fprintf(stderr, "ERR: Too lazy to build anything.\n");
			cerr << "ERR: Too lazy to build anything." << endl;
			return;
		}

		position cCase = toBuild.back();
		if(type_case(cCase) == DEBRIS &&
				points_action() > COUT_DEBLAYAGE) {
			deblayer(cCase);
		}
		else if(type_case(cCase) == VIDE) {
			cerr << "Build." << endl;
			construire(cCase);
			toBuild.pop_back();
		}
		else {
//			fprintf(stderr, "ERR: Dafuq is here?! (%d,%d)\n",
//				cCase.x, cCase.y);
			cerr << "ERR: Dafuq is here?!" << endl;
			return;
		}
	}
}


namespace naive {
void jouer_tour() {
	buildStuff();
}
}
