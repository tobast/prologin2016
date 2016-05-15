#include "stats.h"

CellStats cellStats[TAILLE_TERRAIN][TAILLE_TERRAIN];

void gatherCellStats(int row, int col) {
	position pos = mkPos(row,col);
	cellStats[row][col].plasmaTransited.push_back(
		charges_presentes(pos));
}

void gatherStats() {
	for(int row=0; row < TAILLE_TERRAIN; row++) {
		for(int col=0; col < TAILLE_TERRAIN; col++) {
			gatherCellStats(row,col);
		}
	}
}

int _max(int a, int b) {
	return (a<b)? b : a;
}
double avgPlasma(position pos, int turns) {
	vector<int>& transit = cellStats[pos.y][pos.x].
		plasmaTransited;

	if(transit.size() == 0)
		return 0;
	double sum = 0;
	int num = 0;

	for(int i=_max(0,transit.size()-turns);
			i < (int)transit.size(); i++)
	{
		sum += transit[i];
		num++;
	}

	return sum / ((double)num);
}

