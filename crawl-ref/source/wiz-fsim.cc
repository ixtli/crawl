/*
 *  File:       wiz-fsim.cc
 *  Summary:    Fight simualtion wizard functions.
 *  Written by: Linley Henzell and Jesse Jones
 */

#include "AppHdr.h"

#include "wiz-fsim.h"

#include <errno.h>

#include "beam.h"
#include "dbg-util.h"
#include "fight.h"
#include "items.h"
#include "item_use.h"
#include "it_use2.h"
#include "message.h"
#include "monplace.h"
#include "monster.h"
#include "monstuff.h"
#include "options.h"
#include "player.h"
#include "quiver.h"
#include "skills2.h"
#include "species.h"

#ifdef WIZARD
static int _create_fsim_monster(int mtype, int hp)
{
    const int mi =
        create_monster(
            mgen_data::hostile_at(
                static_cast<monster_type>(mtype), you.pos()));

    if (mi == -1)
        return (mi);

    monsters *mon = &menv[mi];
    mon->hit_points = mon->max_hit_points = hp;
    return (mi);
}

static skill_type _fsim_melee_skill(const item_def *item)
{
    skill_type sk = SK_UNARMED_COMBAT;
    if (item)
        sk = weapon_skill(*item);
    return (sk);
}

static void _fsim_set_melee_skill(int skill, const item_def *item)
{
    you.skills[_fsim_melee_skill(item)] = skill;
    you.skills[SK_FIGHTING]             = skill * 15 / 27;
    you.skills[SK_ARMOUR]               = skill * 15 / 27;
    you.skills[SK_SHIELDS]              = skill;
    for (int i = 0; i < 15; ++i)
        you.skills[SK_SPELLCASTING + i] = skill;
}

static void _fsim_set_ranged_skill(int skill, const item_def *item)
{
    you.skills[range_skill(*item)] = skill;
    you.skills[SK_THROWING]        = skill * 15 / 27;
}

static void _fsim_item(FILE *out,
                       bool melee,
                       const item_def *weap,
                       const char *wskill,
                       unsigned long damage,
                       long iterations, long hits,
                       int maxdam, unsigned long time)
{
    double hitdam = hits? double(damage) / hits : 0.0;
    double avspeed = ((double) time / (double)  iterations);
    fprintf(out,
            " %-5s|  %3ld%%    |  %5.2f |    %5.2f  |"
            "   %5.2f |   %3d   |   %5.2g\n",
            wskill,
            100 * hits / iterations,
            double(damage) / iterations,
            hitdam,
            double(damage) * player_speed() / avspeed / iterations,
            maxdam,
            avspeed);
}

static void _fsim_defence_item(FILE *out, long cum, int hits, int max,
                               int speed, long iters)
{
    // AC | EV | Arm | Dod | Acc | Av.Dam | Av.HitDam | Eff.Dam | Max.Dam | Av.Time
    fprintf(out, "%2d   %2d    %2d   %2d   %3ld%%   %5.2f      %5.2f      %5.2f      %3d"
            "       %2d\n",
            player_AC(),
            player_evasion(),
            you.skills[SK_DODGING],
            you.skills[SK_ARMOUR],
            100 * hits / iters,
            double(cum) / iters,
            hits? double(cum) / hits : 0.0,
            double(cum) / iters * speed / 10,
            max,
            100 / speed);
}


static bool _fsim_ranged_combat(FILE *out, int wskill, int mi,
                                const item_def *item, int missile_slot)
{
    monsters &mon = menv[mi];
    unsigned long cumulative_damage = 0L;
    unsigned long time_taken = 0L;
    long hits = 0L;
    int maxdam = 0;

    const int thrown = missile_slot == -1 ? you.m_quiver->get_fire_item() : missile_slot;
    if (thrown == ENDOFPACK || thrown == -1)
    {
        mprf("No suitable missiles for combat simulation.");
        return (false);
    }

    _fsim_set_ranged_skill(wskill, item);

    no_messages mx;
    const long iter_limit = Options.fsim_rounds;
    const int hunger = you.hunger;
    for (long i = 0; i < iter_limit; ++i)
    {
        mon.hit_points = mon.max_hit_points;
        bolt beam;
        you.time_taken = player_speed();

        // throw_it() will decrease quantity by 1
        inc_inv_item_quantity(thrown, 1);

        beam.target = mon.pos();
        if (throw_it(beam, thrown, true, DEBUG_COOKIE))
            hits++;

        you.hunger = hunger;
        time_taken += you.time_taken;

        int damage = (mon.max_hit_points - mon.hit_points);
        cumulative_damage += damage;
        if (damage > maxdam)
            maxdam = damage;
    }
    _fsim_item(out, false, item, make_stringf("%2d", wskill).c_str(),
               cumulative_damage, iter_limit, hits, maxdam, time_taken);

    return (true);
}

static bool _fsim_mon_melee(FILE *out, int dodge, int armour, int mi)
{
    you.skills[SK_DODGING] = dodge;
    you.skills[SK_ARMOUR]  = armour;

    const int yhp  = you.hp;
    const int ymhp = you.hp_max;
    unsigned long cumulative_damage = 0L;
    long hits = 0L;
    int maxdam = 0;
    no_messages mx;

    for (long i = 0; i < Options.fsim_rounds; ++i)
    {
        you.hp = you.hp_max = 5000;
        monster_attack(&menv[mi]);
        const int damage = you.hp_max - you.hp;
        if (damage)
            hits++;
        cumulative_damage += damage;
        if (damage > maxdam)
            maxdam = damage;
    }

    you.hp = yhp;
    you.hp_max = ymhp;

    _fsim_defence_item(out, cumulative_damage, hits, maxdam, menv[mi].speed,
                       Options.fsim_rounds);
    return (true);
}

static bool _fsim_melee_combat(FILE *out, int wskill, int mi,
                               const item_def *item)
{
    monsters &mon = menv[mi];
    unsigned long cumulative_damage = 0L;
    unsigned long time_taken = 0L;
    long hits = 0L;
    int maxdam = 0;

    _fsim_set_melee_skill(wskill, item);

    no_messages mx;
    const long iter_limit = Options.fsim_rounds;
    const int hunger = you.hunger;
    for (long i = 0; i < iter_limit; ++i)
    {
        mon.hit_points = mon.max_hit_points;
        you.time_taken = player_speed();
        if (you_attack(mi, true))
            hits++;

        you.hunger = hunger;
        time_taken += you.time_taken;

        int damage = (mon.max_hit_points - mon.hit_points);
        cumulative_damage += damage;
        if (damage > maxdam)
            maxdam = damage;
    }
    _fsim_item(out, true, item, make_stringf("%2d", wskill).c_str(),
               cumulative_damage, iter_limit, hits, maxdam, time_taken);

    return (true);
}

static bool debug_fight_simulate(FILE *out, int wskill, int mi, int miss_slot)
{
    int weapon = you.equip[EQ_WEAPON];
    const item_def *iweap = weapon != -1? &you.inv[weapon] : NULL;

    if (iweap && iweap->base_type == OBJ_WEAPONS && is_range_weapon(*iweap))
        return _fsim_ranged_combat(out, wskill, mi, iweap, miss_slot);
    else
        return _fsim_melee_combat(out, wskill, mi, iweap);
}

static const item_def *_fsim_weap_item()
{
    const int weap = you.equip[EQ_WEAPON];
    if (weap == -1)
        return NULL;

    return &you.inv[weap];
}

static std::string _fsim_wskill(int missile_slot)
{
    const item_def *iweap = _fsim_weap_item();
    if (!iweap && missile_slot != -1)
        return skill_name(range_skill(you.inv[missile_slot]));

    if (iweap && iweap->base_type == OBJ_WEAPONS)
    {
        if (is_range_weapon(*iweap))
            return skill_name(range_skill(*iweap));

        return skill_name(_fsim_melee_skill(iweap));
    }
    return skill_name(SK_UNARMED_COMBAT);
}

static std::string _fsim_weapon(int missile_slot)
{
    std::string item_buf;
    if (you.equip[EQ_WEAPON] != -1 || missile_slot != -1)
    {
        if (you.equip[EQ_WEAPON] != -1)
        {
            const item_def &weapon = you.inv[ you.equip[EQ_WEAPON] ];
            item_buf = weapon.name(DESC_PLAIN, true);
            if (is_range_weapon(weapon))
            {
                const int missile =
                    (missile_slot == -1 ? you.m_quiver->get_fire_item()
                                        : missile_slot);

                if (missile < ENDOFPACK && missile >= 0)
                {
                    return item_buf + " with "
                           + you.inv[missile].name(DESC_PLAIN);
                }
            }
        }
        else
            return you.inv[missile_slot].name(DESC_PLAIN);
    }
    else
        return "unarmed";

    return item_buf;
}

static std::string _fsim_time_string()
{
    time_t curr_time = time(NULL);
    struct tm *ltime = TIME_FN(&curr_time);
    if (ltime)
    {
        char buf[100];
        snprintf(buf, sizeof buf, "%4d%02d%02d/%2d:%02d:%02d",
                 ltime->tm_year + 1900,
                 ltime->tm_mon  + 1,
                 ltime->tm_mday,
                 ltime->tm_hour,
                 ltime->tm_min,
                 ltime->tm_sec);
        return (buf);
    }
    return ("");
}

static void _fsim_mon_stats(FILE *o, const monsters &mon)
{
    fprintf(o, "Monster   : %s\n", mon.name(DESC_PLAIN, true).c_str());
    fprintf(o, "HD        : %d\n", mon.hit_dice);
    fprintf(o, "AC        : %d\n", mon.ac);
    fprintf(o, "EV        : %d\n", mon.ev);
}

static void _fsim_title(FILE *o, int mon, int ms)
{
    fprintf(o, CRAWL " version %s\n\n", Version::Long().c_str());
    fprintf(o, "Combat simulation: %s %s vs. %s (%ld rounds) (%s)\n",
            species_name(you.species, you.experience_level).c_str(),
            you.class_name,
            menv[mon].name(DESC_PLAIN, true).c_str(),
            Options.fsim_rounds,
            _fsim_time_string().c_str());

    fprintf(o, "Experience: %d\n", you.experience_level);
    fprintf(o, "Strength  : %d\n", you.strength);
    fprintf(o, "Intel.    : %d\n", you.intel);
    fprintf(o, "Dexterity : %d\n", you.dex);
    fprintf(o, "Base speed: %d\n", player_speed());
    fprintf(o, "\n");

    _fsim_mon_stats(o, menv[mon]);

    fprintf(o, "\n");
    fprintf(o, "Weapon    : %s\n", _fsim_weapon(ms).c_str());
    fprintf(o, "Skill     : %s\n", _fsim_wskill(ms).c_str());
    fprintf(o, "\n");
    fprintf(o, "Skill | Accuracy | Av.Dam | Av.HitDam | Eff.Dam | Max.Dam | Av.Time\n");
}

static void _fsim_defence_title(FILE *o, int mon)
{
    fprintf(o, CRAWL " version %s\n\n", Version::Long().c_str());
    fprintf(o, "Combat simulation: %s vs. %s %s (%ld rounds) (%s)\n",
            menv[mon].name(DESC_PLAIN).c_str(),
            species_name(you.species, you.experience_level).c_str(),
            you.class_name,
            Options.fsim_rounds,
            _fsim_time_string().c_str());
    fprintf(o, "Experience: %d\n", you.experience_level);
    fprintf(o, "Strength  : %d\n", you.strength);
    fprintf(o, "Intel.    : %d\n", you.intel);
    fprintf(o, "Dexterity : %d\n", you.dex);
    fprintf(o, "Base speed: %d\n", player_speed());
    fprintf(o, "\n");
    _fsim_mon_stats(o, menv[mon]);
    fprintf(o, "\n");
    fprintf(o, "AC | EV | Dod | Arm | Acc | Av.Dam | Av.HitDam | Eff.Dam | Max.Dam | Av.Time\n");
}

static bool _fsim_mon_hit_you(FILE *ostat, int mindex, int)
{
    _fsim_defence_title(ostat, mindex);

    for (int sk = 0; sk <= 27; ++sk)
    {
        mesclr();
        mprf("Calculating average damage for %s at dodging %d",
             menv[mindex].name(DESC_PLAIN).c_str(),
             sk);

        if (!_fsim_mon_melee(ostat, sk, 0, mindex))
            return (false);

        fflush(ostat);
        // Not checking in the combat loop itself; that would be more responsive
        // for the user, but slow down the sim with all the calls to kbhit().
        if (kbhit() && getch() == 27)
        {
            mprf("Canceling simulation\n");
            return (false);
        }
    }

    for (int sk = 0; sk <= 27; ++sk)
    {
        mesclr();
        mprf("Calculating average damage for %s at armour %d",
             menv[mindex].name(DESC_PLAIN).c_str(),
             sk);

        if (!_fsim_mon_melee(ostat, 0, sk, mindex))
            return (false);

        fflush(ostat);
        // Not checking in the combat loop itself; that would be more responsive
        // for the user, but slow down the sim with all the calls to kbhit().
        if (kbhit() && getch() == 27)
        {
            mprf("Canceling simulation\n");
            return (false);
        }
    }

    mprf("Done defence simulation with %s",
         menv[mindex].name(DESC_PLAIN).c_str());

    return (true);
}

static bool _fsim_you_hit_mon(FILE *ostat, int mindex, int missile_slot)
{
    _fsim_title(ostat, mindex, missile_slot);
    for (int wskill = 0; wskill <= 27; ++wskill)
    {
        mesclr();
        mprf("Calculating average damage for %s at skill %d",
             _fsim_weapon(missile_slot).c_str(), wskill);

        if (!debug_fight_simulate(ostat, wskill, mindex, missile_slot))
            return (false);

        fflush(ostat);
        // Not checking in the combat loop itself; that would be more responsive
        // for the user, but slow down the sim with all the calls to kbhit().
        if (kbhit() && getch() == 27)
        {
            mprf("Canceling simulation\n");
            return (false);
        }
    }
    mprf("Done fight simulation with %s", _fsim_weapon(missile_slot).c_str());
    return (true);
}

static bool debug_fight_sim(int mindex, int missile_slot,
                            bool (*combat)(FILE *, int mind, int mslot))
{
    FILE *ostat = fopen("fight.stat", "a");
    if (!ostat)
    {
        mprf(MSGCH_ERROR, "Can't write fight.stat: %s", strerror(errno));
        return (false);
    }

    bool success = true;
    FixedVector<unsigned char, 50> skill_backup = you.skills;
    int ystr = you.strength,
        yint = you.intel,
        ydex = you.dex;
    int yxp  = you.experience_level;

    for (int i = SK_FIGHTING; i < NUM_SKILLS; ++i)
        you.skills[i] = 0;

    you.experience_level = Options.fsim_xl;
    if (you.experience_level < 1)
        you.experience_level = 1;
    if (you.experience_level > 27)
        you.experience_level = 27;

    you.strength = debug_cap_stat(Options.fsim_str);
    you.intel    = debug_cap_stat(Options.fsim_int);
    you.dex      = debug_cap_stat(Options.fsim_dex);

    combat(ostat, mindex, missile_slot);

    you.skills = skill_backup;
    you.strength = ystr;
    you.intel    = yint;
    you.dex      = ydex;
    you.experience_level = yxp;

    fprintf(ostat, "-----------------------------------\n\n");
    fclose(ostat);

    return (success);
}

int fsim_kit_equip(const std::string &kit)
{
    int missile_slot = -1;

    std::string::size_type ammo_div = kit.find("/");
    std::string weapon = kit;
    std::string missile;
    if (ammo_div != std::string::npos)
    {
        weapon = kit.substr(0, ammo_div);
        missile = kit.substr(ammo_div + 1);
        trim_string(weapon);
        trim_string(missile);
    }

    if (!weapon.empty())
    {
        for (int i = 0; i < ENDOFPACK; ++i)
        {
            if (!you.inv[i].is_valid())
                continue;

            if (you.inv[i].name(DESC_PLAIN).find(weapon) != std::string::npos)
            {
                if (i != you.equip[EQ_WEAPON])
                {
                    wield_weapon(true, i, false);
                    if (i != you.equip[EQ_WEAPON])
                        return -100;
                }
                break;
            }
        }
    }
    else if (you.equip[EQ_WEAPON] != -1)
        unwield_item(false);

    if (!missile.empty())
    {
        for (int i = 0; i < ENDOFPACK; ++i)
        {
            if (!you.inv[i].is_valid())
                continue;

            if (you.inv[i].name(DESC_PLAIN).find(missile) != std::string::npos)
            {
                missile_slot = i;
                break;
            }
        }
    }

    return (missile_slot);
}

// Writes statistics about a fight to fight.stat in the current directory.
// For fight purposes, a punching bag is summoned and given lots of hp, and the
// average damage the player does to the p. bag over 10000 hits is noted,
// advancing the weapon skill from 0 to 27, and keeping fighting skill to 2/5
// of current weapon skill.
void debug_fight_statistics(bool use_defaults, bool defence)
{
    int punching_bag = get_monster_by_name(Options.fsim_mons);
    if (punching_bag == -1 || punching_bag == MONS_NO_MONSTER)
        punching_bag = MONS_WORM;

    int mindex = _create_fsim_monster(punching_bag, 500);
    if (mindex == -1)
    {
        mprf("Failed to create punching bag");
        return;
    }

    you.exp_available = 0;

    if (!use_defaults || defence)
    {
        debug_fight_sim(mindex, -1,
                        defence? _fsim_mon_hit_you : _fsim_you_hit_mon);
    }
    else
    {
        for (int i = 0, size = Options.fsim_kit.size(); i < size; ++i)
        {
            int missile = fsim_kit_equip(Options.fsim_kit[i]);
            if (missile == -100)
            {
                mprf("Aborting sim on %s", Options.fsim_kit[i].c_str());
                break;
            }
            if (!debug_fight_sim(mindex, missile, _fsim_you_hit_mon))
                break;
        }
    }
    monster_die(&menv[mindex], KILL_DISMISSED, NON_MONSTER);
}

#endif