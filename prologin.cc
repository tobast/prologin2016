/// This file has been generated, if you wish to
/// modify it in a permanent way, please refer
/// to the script file : gen/generator_cxx.rb

#include <cstdlib>
#include <ctime>

#include "prologin.hh"

#include "naive.h"
#include "greedy.h"
using namespace greedy; // greedy strategy

#include <iostream>

/// Fonction appelée au début de la partie.
void partie_init()
{
	srand(time(NULL)+42);
	do_partie_init();
  // fonction a completer
}

/// Fonction appelée à chaque tour.
void jouer_tour()
{
	do_jouer_tour();
}

/// Fonction appelée à la fin de la partie.
void partie_fin()
{
  // fonction a completer
}

