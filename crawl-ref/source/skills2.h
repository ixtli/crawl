/*
 *  File:       skills2.h
 *  Summary:    More skill related functions.
 *  Written by: Linley Henzell
 */


#ifndef SKILLS2_H
#define SKILLS2_H

const int MAX_SKILL_ORDER = 100;

#include "enum.h"

const char *skill_name(int which_skill);
int str_to_skill(const std::string &skill);

std::string skill_title(
    unsigned char best_skill, unsigned char skill_lev,
    // these used for ghosts and hiscores:
    int species = -1, int str = -1, int dex = -1, int god = -1 );

std::string player_title();

skill_type best_skill(int min_skill, int max_skill, int excl_skill = -1);
void init_skill_order();

void calc_mp();
void calc_hp();

int species_skills(int skill, species_type species);
unsigned int skill_exp_needed(int lev);
void show_skills();
void wield_warning(bool newWeapon = true);
bool is_invalid_skill(int skill);
void dump_skills(std::string &text);

#endif
