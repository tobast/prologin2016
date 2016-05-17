#!/bin/bash

TMP_CFG="stechecCfg/tmp.sty"

base="rules: /usr/lib/libprologin2016.so\nverbose: 3\nclients:\n  - ./champion.so\n  - ./champion.so\names:\n  - Player 1\n  - Player 2"

echo -e "$base" > $TMP_CFG
echo "map: $1" >> $TMP_CFG
if [ "$2" = "gui" ] ; then
	echo -e "spectators:\n  - /usr/share/stechec2/prologin2016/gui/gui.so" >> $TMP_CFG
fi

stechec2-run "$TMP_CFG"

