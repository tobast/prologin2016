#include "greedy.h"

using namespace std;

struct Pulsar {
	position pos;
	bool linked;
};

vector<Pulsar> pulsars;

/** Liste des tuyaux auxquels il est intéressant de se brancher */
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

/** Tours d'activité restants pour un pulsar */
int activityTurns(const pulsar_info& infos) {
	return (infos.pulsations_restantes - 1) * infos.periode;
}

int cellDistToPipe[TAILLE_TERRAIN][TAILLE_TERRAIN];
/** Donne un score à un pulsar pour le greedy */
double scorePulsar(const Pulsar& puls) {
	if(puls.linked)
		return -1;
	position pos = puls.pos;
	if(!est_pulsar(pos))
		return -1;
	pulsar_info infos = info_pulsar(pos);
	if(infos.pulsations_restantes == 0)
		return -1;
	double emitScore = sq(infos.puissance /
		((double)infos.periode)) * infos.pulsations_restantes;
	//	(NB_TOURS + 1 - tour_actuel() -
	//		activityTurns(infos));
	
	/*
	vector<position> plugs = possiblePlugs();
	pair<Path,int> pth = pathToPulsar(pos,
		setOfVect(plugs));
	double distScore = pth.second;
	*/
	double distScore = cellDistToPipe[pos.y][pos.x];
	if(manhattanToBase(pos) > manhattanToBase(pos,true))
		distScore *= sqrt(manhattanToBase(pos)-
			manhattanToBase(pos,true) + 1) / 4.;
		//manhattanToBase(pos);

	return emitScore / distScore;
}

bool orderPulsar(const Pulsar& p1, const Pulsar& p2) {
	return scorePulsar(p1) < scorePulsar(p2);
}


Pulsar targetPuls;
Path worklist, shadowWorkList;
Path reinforcements;
queue<position> aspiUp;
vector<position> usedBases;
SPos selectedBackbone;
Path allBackbones;

bool allLinked=false;
int doubledBackbones = 0;


/** Ajoute un second plug à une backbone, le but étant de
	rendre inefficaces les IA tirant à la base de la backbone
	(contre la base) */
Path doubleBayPlugs(const Path& path, bool reinf=false);
Path doubleBayPlugs(const Path& path, bool reinf) {
	Path out;
	auto endIt = path.end();
	for(auto it=path.begin(); it != endIt; ++it) {
		if(baseOfBay(*it).x >= 0) { // est une base
			aspiUp.push(baseOfBay(*it));
			usedBases.push_back(baseOfBay(*it));
			for(int adj=0; adj < NB_ADJACENCY; adj++) {
				position nPos = *it+ADJACENCY[adj];
				if(baseOfBay(nPos).x >= 0) {
					out.push_back(nPos);
					usedBases.push_back(baseOfBay(nPos));
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

/** Doubler une section droite de tuyau - utilisé sur les
backbones */
void doubleLinearSection(position pos, Path& out, int dir=-1,
	int side=-1);
void doubleLinearSection(position pos, Path& out,  int dir,
		int side) {
	if(dir < 0) {
		Path connected = connectedPipes(pos);
		if(connected.empty() || connected.size() > 2) {
			if(manhattanToBase(pos) <= 2) {
				for(int cDir=0; cDir < 4; cDir++) {
					if(manhattanToBase(pos+ADJACENCY[cDir]) <
							manhattanToBase(pos)) {
						dir=cDir;
						break;
					}
				}
			}
			else			
				return;
		}
		else {
			for(int adj=0; adj < NB_ADJACENCY; adj++) {
				if(pos+ADJACENCY[adj] == connected.front()) {
					dir=adj;
					break;
				}
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

/** Prend trop longtemps. */
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

/* Ordre de distance de manhattan à la base */
bool orderByManhattan(const position& p1, const position& p2) {
	if(selectedBackbone.find(p1) != selectedBackbone.end())
		return false;
	return manhattanToBase(p1) < manhattanToBase(p2);
}

/** distance aux backbones déjà posées */
double distToPrevBackbones(const position& p) {
	double dst=0;
	for(position prevBack : selectedBackbone) {
		dst += manhattan(prevBack, p);
	}
	return dst / ((double)selectedBackbone.size());
}

/** ordre (cf ^) */
double cheapBackboneWeight(const position& p) {

	return distToPrevBackbones(p) / sq(avgPlasma(p)+0.01);
}

bool orderByCheapBackbone(const position& p1,
	const position& p2)
{
	return cheapBackboneWeight(p1) < cheapBackboneWeight(p2);
}
/** Fonction d'évaluation de l'utilité d'un renforcement
coûteuse */
double costyReinf(const position& p) {
	position targetBase = manhattanNearestBase(p);
	int enemyDist = manhattanToBase(p, true);
	int selfDist = manhattan(p,targetBase);
	int distToOwnedBase = INFTY;
	for(position bPos : usedBases)
		distToOwnedBase = min(distToOwnedBase,
			manhattan(bPos, targetBase));
	int minDistToBackbone = INFTY;
	for(position pos : allBackbones)
		minDistToBackbone = min(minDistToBackbone,
			manhattan(pos, p));
	double ownership = pipe_ownership(p);

	if(minDistToBackbone < 5)
		return sq((double)INFTY);

	return ((double)distToOwnedBase) * (1.0001 - ownership)
		/ pow(distToPrevBackbones(p),2)
		* log(((double)enemyDist) / ((double)selfDist) + 1.01)
		/ (avgPlasma(p)+0.01);
}

/** Trouve une backbone à poser */
Path bestBackboneReinforcement() {
	vector<position> pipes = liste_tuyaux();
	sort(pipes.begin(), pipes.end(), orderByCheapBackbone);

	int optId=0;
	double optScore = costyReinf(pipes[0]);
	for(int pos=1; pos < min(100,(int)pipes.size()); pos++) {
		double score = costyReinf(pipes[pos]);

		if(score < optScore) {
			optScore = score;
			optId = pos;
		}
	}

	selectedBackbone.insert(pipes[optId]);

	SPos bays = setOfVect(liste_base_baies());
	Path bestPath = shortpath(pipes[optId], bays, true).first;
	bestPath = doubleBayPlugs(bestPath, true);

	copy(bestPath.begin(), bestPath.end(),
		std::back_inserter(allBackbones));

	return bestPath;
}

struct BfsElem {
	BfsElem() {}
	BfsElem(position pos, int dist) : pos(pos),dist(dist) {}
	position pos;
	int dist;
};

/** Remplit une grille de distance de chaque case aux pipes plus
proches (manhattan) de moi que de l'adversaire */
void fillDistsFromMyPipes(
		int dists[TAILLE_TERRAIN][TAILLE_TERRAIN])
{
	queue<BfsElem> process;
	vector<position> pipes = liste_tuyaux();
	for(position pipe : pipes) {
		if(manhattanToBase(pipe,false) <
				manhattanToBase(pipe,true))
			process.push(BfsElem(pipe, 0));
	}
	for(position p : liste_base_baies())
		process.push(BfsElem(p,0));

	for(int row=0; row < TAILLE_TERRAIN; row++)
		for(int col=0; col < TAILLE_TERRAIN; col++)
			dists[row][col] = -1;
	
	while(!process.empty()) {
		BfsElem cElt = process.front();
		process.pop();
		position& cPos = cElt.pos;
		if(!(inGrid(cPos)) || dists[cPos.y][cPos.x] >= 0)
			continue;
		dists[cPos.y][cPos.x] = cElt.dist;

		for(int adj=0; adj < NB_ADJACENCY; adj++) {
			position nPos = cPos + ADJACENCY[adj];
			process.push(BfsElem(nPos, cElt.dist+1));
		}
	}
}

/** Évaluation d'une position sur laquelle tirer */
double rankFiringPos(const position& pos, int aspi,
		const position& prev)
{
	double score = ((double)sq(aspi+1)) * avgPlasma(pos, 2) *
		(sqrt(charges_presentes(prev)) + 1.);
	if(est_super_tuyau(pos))
		score /= 100.;
	return score;
}

/** Dfs sur une backbone pour choisir la cible d'un tir */
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

/** Trouve une case sur laquelle tirer */
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

/** Trouve un point d'aspiration non utilisé qu'on peut
bouger */
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

/** Réalloue un opint d'aspiration */
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

/** Trouve le pulsar suivant à viser */
void acquireTarget() {
	fillDistsFromMyPipes(cellDistToPipe);
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

/** Score pour placer un superpipe selon la position */
double distanceScoreSPipes(const position& p1) {
	if(manhattanToBase(p1) < manhattanToBase(p1,true))
		return manhattanToBase(p1);
	return 3*manhattanToBase(p1);
}

bool supPipesSorter(const position& p1, const position& p2) {
	if(est_super_tuyau(p1))
		return false;
	return avgPlasma(p1) * distanceScoreSPipes(p1) <
		avgPlasma(p2) * distanceScoreSPipes(p2);
}

/** Utilisé quand il reste des points qu'on ne sait pas dépenser */
void makeSuperPipe(int num) {
	Path pipes = liste_tuyaux();
	sort(pipes.begin(), pipes.end(), supPipesSorter);

	if(num > (int)pipes.size()) {
		cerr << "Not enough pipes to keep me busy..." << endl;
		return;
	}

	for(int pos=0; pos < num; pos++)
		ameliorer(pipes[pos]);
}


int nbPulsarsInRow = 0;
/** Joue un tour de jeu */
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

	int didNothing = -1; // On ne "fait rien" au premier tour
	int lastAP = points_action();
	bool tryingBackbone = false;

	while(points_action() > 0) {

		// On n'a rien fait au tour de boucle précédent --
		// infinite loop?!
		if(lastAP == points_action())
			didNothing++;
		lastAP = points_action();

		if(didNothing > 12 && tryingBackbone) {
			// We took no action 8 times.
			cerr << "Fallback super pipe" << endl;
			makeSuperPipe(lastAP);
			break;
		}

		if(shadowWorkList.empty() && info_pulsar(
				targetPuls.pos).pulsations_restantes == 0) {
			worklist.clear();
		}
		if(worklist.empty() && reinforcements.empty() &&
				stacked.empty()) {
			if(!shadowWorkList.empty()) {
				worklist = shadowWorkList;
				shadowWorkList.clear();
			}
			else if(allLinked ||
					nbPulsarsInRow >= MAX_PULSARS_IN_ROW) {
#ifdef PROFILING
				stClock = clock();
#endif
				worklist = bestBackboneReinforcement();
				tryingBackbone = true;
				nbPulsarsInRow = 0;
#ifdef PROFILING
			cerr << "bBkbReinf " << ellapsed(stClock) << endl;
#endif
			}
			else {
#ifdef PROFILING
				stClock = clock();
#endif
				acquireTarget();
				tryingBackbone = false;
				didNothing = 0;
				nbPulsarsInRow++;
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

/** Répare les dégâts des tirs de l'ennemi */
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

/** Tire sur une cible */
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
	// On met à jour les stats
	gatherStats();
#ifdef PROFILING
	cerr << "gatherStats " << ellapsed(cl) << endl;
	cl = clock();
#endif
	// Répare les dégâts
	repareDamage();
#ifdef PROFILING
	cerr << "repareDmg " << ellapsed(cl) << endl;
	cl = clock();
#endif
	// Tire sur l'ennemi (possiblement) (random)
	killItWithFire();
#ifdef PROFILING
	cerr << "killItWithFire " << ellapsed(cl) << endl;
	cl = clock();
#endif
	// Joue vraiment.
	tour();
#ifdef PROFILING
	cerr << "tour " << ellapsed(cl) << endl;
#endif
}
}
