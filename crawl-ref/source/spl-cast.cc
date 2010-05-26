/*
 *  File:       spl-cast.cc
 *  Summary:    Spell casting and miscast functions.
 *  Written by: Linley Henzell
 */

#include "AppHdr.h"

#include <sstream>
#include <iomanip>

#include "spl-cast.h"

#include "externs.h"
#include "options.h"

#include "beam.h"
#include "coord.h"
#include "coordit.h"
#include "describe.h"
#include "directn.h"
#include "effects.h"
#include "env.h"
#include "map_knowledge.h"
#include "food.h"
#include "format.h"
#include "godabil.h"
#include "goditem.h"
#include "invent.h"
#include "it_use2.h"
#include "item_use.h"
#include "itemname.h"
#include "itemprop.h"
#include "macro.h"
#include "menu.h"
#include "misc.h"
#include "message.h"
#include "mon-cast.h"
#include "mon-place.h"
#include "mon-project.h"
#include "mon-stuff.h"
#include "mon-util.h"
#include "mutation.h"
#include "ouch.h"
#include "player.h"
#include "religion.h"
#include "godconduct.h"
#include "skills.h"
#include "skills2.h"
#include "spells1.h"
#include "spells2.h"
#include "spells3.h"
#include "spells4.h"
#include "spl-book.h"
#include "spl-mis.h"
#include "spl-util.h"
#include "spl-zap.h"
#include "state.h"
#include "stuff.h"
#include "areas.h"
#include "transform.h"
#include "hints.h"
#include "view.h"
#include "shout.h"
#include "colour.h"

static bool _surge_identify_boosters(spell_type spell)
{
    const unsigned int typeflags = get_spell_disciplines(spell);
    if ( (typeflags & SPTYP_FIRE) || (typeflags & SPTYP_ICE) )
    {
        // Must not be wielding an unIDed staff.
        // Note that robes of the Archmagi identify on wearing,
        // so that's less of an issue.
        const item_def* wpn = you.weapon();
        if (wpn == NULL
            || wpn->base_type != OBJ_STAVES
            || item_ident(*wpn, ISFLAG_KNOW_PROPERTIES))
        {
            int num_unknown = 0;
            for (int i = EQ_LEFT_RING; i <= EQ_RIGHT_RING; ++i)
            {
                if (player_wearing_slot(i)
                    && !item_ident(you.inv[you.equip[i]],
                                   ISFLAG_KNOW_PROPERTIES))
                {
                    ++num_unknown;
                }
            }

            // We can also identify cases with two unknown rings, both
            // of fire (or both of ice)...let's skip it.
            if (num_unknown == 1)
            {
                for (int i = EQ_LEFT_RING; i <= EQ_RIGHT_RING; ++i)
                    if (player_wearing_slot(i))
                    {
                        item_def& ring = you.inv[you.equip[i]];
                        if (!item_ident(ring, ISFLAG_KNOW_PROPERTIES)
                            && (ring.sub_type == RING_FIRE
                                || ring.sub_type == RING_ICE))
                        {
                            set_ident_type( ring.base_type, ring.sub_type,
                                            ID_KNOWN_TYPE );
                            set_ident_flags(ring, ISFLAG_KNOW_PROPERTIES);
                            mprf("You are wearing: %s",
                                 ring.name(DESC_INVENTORY_EQUIP).c_str());
                        }
                    }

                return (true);
            }
        }
    }
    return (false);
}

static void _surge_power(spell_type spell)
{
    int enhanced = 0;

    _surge_identify_boosters(spell);
    enhanced += spell_enhancement(get_spell_disciplines(spell));

    if (enhanced)               // one way or the other {dlb}
    {
        mprf("You feel a%s %s",
             (enhanced < -2)  ? "n extraordinarily" :
             (enhanced == -2) ? "n extremely" :
             (enhanced == 2)  ? " strong" :
             (enhanced > 2)   ? " huge"
                              : "",
             (enhanced < 0) ? "numb sensation."
                            : "surge of power!");
    }
}

static std::string _spell_base_description(spell_type spell)
{
    std::ostringstream desc;

    int highlight =  spell_highlight_by_utility(spell, COL_UNKNOWN, true);

    desc << "<" << colour_to_str(highlight) << ">" << std::left;

    // spell name
    desc << std::setw(30) << spell_title(spell);

    // spell schools
    desc << spell_schools_string(spell);

    const int so_far = desc.str().length() - (name_length_by_colour(highlight)+2);
    if (so_far < 60)
        desc << std::string(60 - so_far, ' ');

    // spell fail rate, level
    desc << std::setw(12) << failure_rate_to_string(spell_fail(spell))
         << spell_difficulty(spell);
    desc << "</" << colour_to_str(highlight) <<">";

    return desc.str();
}

static std::string _spell_extra_description(spell_type spell)
{
    std::ostringstream desc;

    int highlight =  spell_highlight_by_utility(spell, COL_UNKNOWN, true);

    desc << "<" << colour_to_str(highlight) << ">" << std::left;

    // spell name
    desc << std::setw(30) << spell_title(spell);

    // spell power, spell range, hunger level, level
    const std::string rangestring = spell_range_string(spell);

    desc << std::setw(14) << spell_power_string(spell)
         << std::setw(16 + tagged_string_tag_length(rangestring)) << rangestring
         << std::setw(12) << spell_hunger_string(spell)
         << spell_difficulty(spell);

    desc << "</" << colour_to_str(highlight) <<">";

    return desc.str();
}

int list_spells(bool toggle_with_I, bool viewing, int minRange,
                spell_selector selector)
{
    if (toggle_with_I && get_spell_by_letter('I') != SPELL_NO_SPELL)
        toggle_with_I = false;

#ifdef USE_TILE
    const bool text_only = false;
#else
    const bool text_only = true;
#endif

    ToggleableMenu spell_menu(MF_SINGLESELECT | MF_ANYPRINTABLE
                              | MF_ALWAYS_SHOW_MORE | MF_ALLOW_FORMATTING,
                              text_only);
#ifdef USE_TILE
    {
        // [enne] - Hack.  Make title an item so that it's aligned.
        ToggleableMenuEntry* me =
            new ToggleableMenuEntry(
                " Your Spells                       Type          "
                "                Success   Level",
                " Your Spells                       Power         "
                "Range           Hunger    Level",
                MEL_ITEM);
        me->colour = BLUE;
        spell_menu.add_entry(me);
    }
#else
    spell_menu.set_title(
        new ToggleableMenuEntry(
            " Your Spells                       Type          "
            "                Success   Level",
            " Your Spells                       Power         "
            "Range           Hunger    Level",
            MEL_TITLE));
#endif
    spell_menu.set_highlighter(NULL);
    spell_menu.set_tag("spell");
    spell_menu.add_toggle_key('!');

    std::string more_str = "Press '!' ";
    if (toggle_with_I)
    {
        spell_menu.add_toggle_key('I');
        more_str += "or 'I' ";
    }
    if (!viewing)
        spell_menu.menu_action = Menu::ACT_EXECUTE;
    more_str += "to toggle spell view.";
    spell_menu.set_more(formatted_string(more_str));

    for (int i = 0; i < 52; ++i)
    {
        const char letter = index_to_letter(i);
        const spell_type spell = get_spell_by_letter(letter);

        // TODO: identify wth 'selector' is, and what
        //       exactly this bit below does with it
        // In tilereg.cc, _spell_selector() does some range checks,
        // possibly duplicated. (jpeg)
        if (is_valid_spell(spell) && selector
            && !(*selector)(spell))
        {
            continue;
        }

        if (spell != SPELL_NO_SPELL)
        {
            ToggleableMenuEntry* me =
                new ToggleableMenuEntry(_spell_base_description(spell),
                                        _spell_extra_description(spell),
                                        MEL_ITEM, 1, letter);

#ifdef USE_TILE
            me->add_tile(tile_def(tileidx_spell(spell), TEX_GUI));
#endif
            spell_menu.add_entry(me);
        }
    }

    while (true)
    {
        std::vector<MenuEntry*> sel = spell_menu.show();
        if (!crawl_state.doing_prev_cmd_again)
            redraw_screen();
        if (sel.empty())
            return 0;

        ASSERT(sel.size() == 1);
        ASSERT(sel[0]->hotkeys.size() == 1);
        if (spell_menu.menu_action == Menu::ACT_EXAMINE)
        {
            describe_spell(get_spell_by_letter(sel[0]->hotkeys[0]));
            redraw_screen();
        }
        else
            return sel[0]->hotkeys[0];
    }
}

static int _apply_spellcasting_success_boosts(spell_type spell, int chance)
{
    int wizardry = player_mag_abil(false);
    int fail_reduce = 100;
    int wiz_factor = 87;

    if (you.religion == GOD_VEHUMET
        && !player_under_penance() && you.piety >= piety_breakpoint(1)
        && vehumet_supports_spell(spell))
    {
        // [dshaligram] Fail rate multiplier used to be .5, scaled
        // back to 67%.
        fail_reduce = fail_reduce * 67 / 100;
    }

    // [dshaligram] Apply wizardry factor here, rather than mixed into the
    // pre-scaling spell power.
    while (wizardry-- > 0)
    {
        fail_reduce  = fail_reduce * wiz_factor / 100;
        wiz_factor  += (100 - wiz_factor) / 3;
    }

    // Apply Brilliance factor here.
    if (you.duration[DUR_BRILLIANCE])
        fail_reduce = fail_reduce * 67 / 100;

    // Draconians get a boost to dragon-form.
    if (spell == SPELL_DRAGON_FORM && player_genus(GENPC_DRACONIAN))
        fail_reduce = fail_reduce * 70 / 100;

    // Hard cap on fail rate reduction.
    if (fail_reduce < 50)
        fail_reduce = 50;

    return (chance * fail_reduce / 100);
}

int spell_fail(spell_type spell)
{
    int chance = 60;
    int chance2 = 0;

    // Don't cap power for failure rate purposes.
    chance -= 6 * calc_spell_power(spell, false, true, false);
    chance -= (you.intel() * 2);

    const int armour_shield_penalty = player_armour_shield_spell_penalty();
    dprf("Armour+Shield spell failure penalty: %d", armour_shield_penalty);
    chance += armour_shield_penalty;

    if (you.weapon() && you.weapon()->base_type == OBJ_WEAPONS)
    {
        int wpn_penalty = (3 * (property(*you.weapon(), PWPN_SPEED) - 12))/2;

        if (wpn_penalty > 0)
            chance += wpn_penalty;
    }

    switch (spell_difficulty(spell))
    {
    case  1: chance +=   3; break;
    case  2: chance +=  15; break;
    case  3: chance +=  35; break;
    case  4: chance +=  70; break;
    case  5: chance += 100; break;
    case  6: chance += 150; break;
    case  7: chance += 200; break;
    case  8: chance += 260; break;
    case  9: chance += 330; break;
    case 10: chance += 420; break;
    case 11: chance += 500; break;
    case 12: chance += 600; break;
    default: chance += 750; break;
    }

    chance2 = chance;

    const int chance_breaks[][2] = {
        {45, 45}, {42, 43}, {38, 41}, {35, 40}, {32, 38}, {28, 36},
        {22, 34}, {16, 32}, {10, 30}, {2, 28}, {-7, 26}, {-12, 24},
        {-18, 22}, {-24, 20}, {-30, 18}, {-38, 16}, {-46, 14},
        {-60, 12}, {-80, 10}, {-100, 8}, {-120, 6}, {-140, 4},
        {-160, 2}, {-180, 0}
    };

    for ( unsigned int i = 0; i < ARRAYSZ(chance_breaks); ++i )
        if ( chance < chance_breaks[i][0] )
            chance2 = chance_breaks[i][1];

    if (you.duration[DUR_TRANSFORMATION] > 0)
    {
        switch (you.attribute[ATTR_TRANSFORMATION])
        {
        case TRAN_BLADE_HANDS:
            chance2 += 20;
            break;

        case TRAN_SPIDER:
        case TRAN_BAT:
            chance2 += 10;
            break;
        }
    }

    // Apply the effects of Vehumet and items of wizardry.
    chance2 = _apply_spellcasting_success_boosts(spell, chance2);

    if (chance2 > 100)
        chance2 = 100;

    return (chance2);
}                               // end spell_fail()


int calc_spell_power(spell_type spell, bool apply_intel, bool fail_rate_check,
                     bool cap_power)
{
    // When checking failure rates, wizardry is handled after the various
    // stepping calulations.
    int power = (you.skills[SK_SPELLCASTING] / 2)
                 + (fail_rate_check? 0 : player_mag_abil(false));
    int enhanced = 0;

    unsigned int disciplines = get_spell_disciplines( spell );

    //jmf: evil evil evil -- exclude HOLY bit
    disciplines &= (~SPTYP_HOLY);

    int skillcount = count_bits( disciplines );
    if (skillcount)
    {
        for (int ndx = 0; ndx <= SPTYP_LAST_EXPONENT; ndx++)
        {
            unsigned int bit = (1 << ndx);
            if (disciplines & bit)
                power += (you.skills[spell_type2skill(bit)] * 2) / skillcount;
        }
    }

    if (apply_intel)
        power = (power * you.intel()) / 10;

    // [dshaligram] Enhancers don't affect fail rates any more, only spell
    // power. Note that this does not affect Vehumet's boost in castability.
    if (!fail_rate_check)
        enhanced = spell_enhancement( disciplines );

    if (enhanced > 0)
    {
        for (int i = 0; i < enhanced; i++)
        {
            power *= 15;
            power /= 10;
        }
    }
    else if (enhanced < 0)
    {
        for (int i = enhanced; i < 0; i++)
            power /= 2;
    }

    power = stepdown_value( power, 50, 50, 150, 200 );

    const int cap = spell_power_cap(spell);
    if (cap > 0 && cap_power)
        power = std::min(power, cap);

    return (power);
}


int spell_enhancement( unsigned int typeflags )
{
    int enhanced = 0;

    if (typeflags & SPTYP_CONJURATION)
        enhanced += player_spec_conj();

    if (typeflags & SPTYP_ENCHANTMENT)
        enhanced += player_spec_ench();

    if (typeflags & SPTYP_SUMMONING)
        enhanced += player_spec_summ();

    if (typeflags & SPTYP_POISON)
        enhanced += player_spec_poison();

    if (typeflags & SPTYP_NECROMANCY)
        enhanced += player_spec_death() - player_spec_holy();

    if (typeflags & SPTYP_FIRE)
        enhanced += player_spec_fire() - player_spec_cold();

    if (typeflags & SPTYP_ICE)
        enhanced += player_spec_cold() - player_spec_fire();

    if (typeflags & SPTYP_EARTH)
        enhanced += player_spec_earth() - player_spec_air();

    if (typeflags & SPTYP_AIR)
        enhanced += player_spec_air() - player_spec_earth();

    if (you.attribute[ATTR_SHADOWS])
        enhanced -= 2;

    // These are used in an exponential way, so we'll limit them a bit. -- bwr
    if (enhanced > 3)
        enhanced = 3;
    else if (enhanced < -3)
        enhanced = -3;

    return (enhanced);
}                               // end spell_enhancement()

void inspect_spells()
{
    if (!you.spell_no)
    {
        canned_msg(MSG_NO_SPELLS);
        return;
    }

    // Maybe we should honour auto_list here, but if you want the
    // description, you probably want the listing, too.
    list_spells(true, true);
}

void do_cast_spell_cmd(bool force)
{
    if (player_in_bat_form() || you.attribute[ATTR_TRANSFORMATION] == TRAN_PIG)
    {
        canned_msg(MSG_PRESENT_FORM);
        return;
    }

    if (you.stat_zero[STAT_INT])
    {
        mpr("You lack the mental capacity to cast spells.");
        return;
    }

    // Randart weapons.
    if (scan_artefacts(ARTP_PREVENT_SPELLCASTING))
    {
        mpr("Something interferes with your magic!");
        flush_input_buffer(FLUSH_ON_FAILURE);
        return;
    }

    if (Hints.hints_left)
        Hints.hints_spell_counter++;
    if (!cast_a_spell(!force))
        flush_input_buffer(FLUSH_ON_FAILURE);
}

// Returns false if spell failed, and true otherwise.
bool cast_a_spell(bool check_range, spell_type spell)
{
    if (!you.spell_no)
    {
        canned_msg(MSG_NO_SPELLS);
        crawl_state.zero_turns_taken();
        return (false);
    }

    if (you.berserk())
    {
        canned_msg(MSG_TOO_BERSERK);
        return (false);
    }

    if (silenced(you.pos()))
    {
        mpr("You cannot cast spells when silenced!");
        crawl_state.zero_turns_taken();
        more();
        return (false);
    }

    const int minRange = get_dist_to_nearest_monster();

    if (spell == SPELL_NO_SPELL)
    {
        int keyin = 0;

        while (true)
        {
            if (keyin == 0)
            {
                mpr("Cast which spell? (? or * to list) ", MSGCH_PROMPT);

                keyin = get_ch();
            }

            if (keyin == '?' || keyin == '*')
            {
                keyin = list_spells(true, false, minRange);
                if (!keyin)
                    keyin = ESCAPE;

                if (!crawl_state.doing_prev_cmd_again)
                    redraw_screen();

                if (isaalpha(keyin) || keyin == ESCAPE)
                    break;
                else
                    mesclr();

                keyin = 0;
            }
            else
                break;
        }

        if (keyin == ESCAPE)
        {
            canned_msg( MSG_OK );
            return (false);
        }

        if (!isaalpha(keyin))
        {
            mpr("You don't know that spell.");
            crawl_state.zero_turns_taken();
            return (false);
        }

        spell = get_spell_by_letter( keyin );
    }

    if (spell == SPELL_NO_SPELL)
    {
        mpr("You don't know that spell.");
        crawl_state.zero_turns_taken();
        return (false);
    }

    if (spell_mana(spell) > you.magic_points)
    {
        mpr("You don't have enough magic to cast that spell.");
        return (false);
    }

    if (check_range && spell_no_hostile_in_range(spell, minRange))
    {
        // Abort if there are no hostiles within range, but flash the range
        // markers for a short while.
        mpr("There are no visible monsters within range! (Use <w>Z</w> to "
            "cast anyway.)");

        if (Options.darken_beyond_range)
        {
            crawl_state.darken_range = calc_spell_range(spell);
            viewwindow(false, false);
            delay(50);
            crawl_state.darken_range = -1;
            viewwindow(false, false);
        }
        return (false);
    }

    if (!you.is_undead
        && (you.hunger_state == HS_STARVING
            || you.hunger <= spell_hunger( spell )))
    {
        mpr("You don't have the energy to cast that spell.");
        return (false);
    }

    const bool staff_energy = player_energy();
    if (you.confused())
        random_uselessness();
    else
    {
        const spret_type cast_result = your_spells(spell);
        if (cast_result == SPRET_ABORT)
        {
            crawl_state.zero_turns_taken();
            return (false);
        }

        exercise_spell( spell, true, cast_result == SPRET_SUCCESS );
        did_god_conduct( DID_SPELL_CASTING, 1 + random2(5) );
    }

    dec_mp( spell_mana(spell) );

    if (!staff_energy && you.is_undead != US_UNDEAD)
    {
        const int spellh = calc_hunger( spell_hunger(spell) );
        if (spellh > 0)
        {
            make_hungry(spellh, true);
            learned_something_new(HINT_SPELL_HUNGER);
        }
    }

    you.turn_is_over = true;
    alert_nearby_monsters();

    return (true);
}                               // end cast_a_spell()

// "Utility" spells for the sake of simplicity are currently ones with
// enchantments, translocations, or divinations.
static bool _spell_is_utility_spell(spell_type spell_id)
{
    return (spell_typematch(spell_id,
                SPTYP_ENCHANTMENT | SPTYP_TRANSLOCATION));
}

bool maybe_identify_staff(item_def &item, spell_type spell)
{
    if (item_type_known(item))
        return (true);

    int relevant_skill = 0;
    const bool chance = (spell != SPELL_NO_SPELL);

    switch (item.sub_type)
    {
        case STAFF_ENERGY:
            if (!chance) // The staff of energy only autoIDs by chance.
                return (false);
            // intentional fall-through
        case STAFF_WIZARDRY:
            relevant_skill = you.skills[SK_SPELLCASTING];
            break;

        case STAFF_FIRE:
            if (!chance || spell_typematch(spell, SPTYP_FIRE))
                relevant_skill = you.skills[SK_FIRE_MAGIC];
            else if (spell_typematch(spell, SPTYP_ICE))
                relevant_skill = you.skills[SK_ICE_MAGIC];
            break;

        case STAFF_COLD:
            if (!chance || spell_typematch(spell, SPTYP_ICE))
                relevant_skill = you.skills[SK_ICE_MAGIC];
            else if (spell_typematch(spell, SPTYP_FIRE))
                relevant_skill = you.skills[SK_FIRE_MAGIC];
            break;

        case STAFF_AIR:
            if (!chance || spell_typematch(spell, SPTYP_AIR))
                relevant_skill = you.skills[SK_AIR_MAGIC];
            else if (spell_typematch(spell, SPTYP_EARTH))
                relevant_skill = you.skills[SK_EARTH_MAGIC];
            break;

        case STAFF_EARTH:
            if (!chance || spell_typematch(spell, SPTYP_EARTH))
                relevant_skill = you.skills[SK_EARTH_MAGIC];
            else if (spell_typematch(spell, SPTYP_AIR))
                relevant_skill = you.skills[SK_AIR_MAGIC];
            break;

        case STAFF_POISON:
            if (!chance || spell_typematch(spell, SPTYP_POISON))
                relevant_skill = you.skills[SK_POISON_MAGIC];
            break;

        case STAFF_DEATH:
            if (!chance || spell_typematch(spell, SPTYP_NECROMANCY))
                relevant_skill = you.skills[SK_NECROMANCY];
            break;

        case STAFF_CONJURATION:
            if (!chance || spell_typematch(spell, SPTYP_CONJURATION))
                relevant_skill = you.skills[SK_CONJURATIONS];
            break;

        case STAFF_ENCHANTMENT:
            if (!chance || spell_typematch(spell, SPTYP_ENCHANTMENT))
                relevant_skill = you.skills[SK_ENCHANTMENTS];
            break;

        case STAFF_SUMMONING:
            if (!chance || spell_typematch(spell, SPTYP_SUMMONING))
                relevant_skill = you.skills[SK_SUMMONINGS];
            break;
    }

    bool id_staff = false;

    if (chance)
    {
        if (you.skills[SK_SPELLCASTING] > relevant_skill)
            relevant_skill = you.skills[SK_SPELLCASTING];

        if (x_chance_in_y(relevant_skill, 100))
            id_staff = true;
    }
    else if (relevant_skill >= 4)
        id_staff = true;

    if (id_staff)
    {
        item_def& wpn = *you.weapon();
        set_ident_type(wpn, ID_KNOWN_TYPE);
        set_ident_flags( wpn, ISFLAG_IDENT_MASK);
        mprf("You are wielding %s.", wpn.name(DESC_NOCAP_A).c_str());
        more();

        you.wield_change = true;
    }
    return (id_staff);
}

static void _spellcasting_side_effects(spell_type spell, bool idonly = false)
{
    if (you.weapon() && item_is_staff(*you.weapon()))
        maybe_identify_staff(*you.weapon(), spell);

    if (idonly)
        return;

    // If you are casting while a god is acting, then don't do conducts.
    // (Presumably Xom is forcing you to cast a spell.)
    if (!_spell_is_utility_spell(spell) && !crawl_state.is_god_acting())
        did_god_conduct(DID_SPELL_NONUTILITY, 10 + spell_difficulty(spell));

    if (is_holy_spell(spell) && !crawl_state.is_god_acting())
        did_god_conduct(DID_HOLY, 10 + spell_difficulty(spell));

    if (is_unholy_spell(spell) && !crawl_state.is_god_acting())
        did_god_conduct(DID_UNHOLY, 10 + spell_difficulty(spell));

    if (is_unclean_spell(spell) && !crawl_state.is_god_acting())
        did_god_conduct(DID_UNCLEAN, 10 + spell_difficulty(spell));

    if (is_chaotic_spell(spell) && !crawl_state.is_god_acting())
        did_god_conduct(DID_CHAOS, 10 + spell_difficulty(spell));

    if (is_corpse_violating_spell(spell) && !crawl_state.is_god_acting())
        did_god_conduct(DID_CORPSE_VIOLATION, 10 + spell_difficulty(spell));

    // Linley says: Condensation Shield needs some disadvantages to keep
    // it from being a no-brainer... this isn't much, but its a start. - bwr
    if (spell_typematch(spell, SPTYP_FIRE))
        expose_player_to_element(BEAM_FIRE, 0);

    if (spell_typematch(spell, SPTYP_NECROMANCY)
        && !crawl_state.is_god_acting())
    {
        did_god_conduct(DID_NECROMANCY, 10 + spell_difficulty(spell));

        if (spell == SPELL_NECROMUTATION && is_good_god(you.religion))
            excommunication();
    }

    alert_nearby_monsters();
}

static bool _vampire_cannot_cast(spell_type spell)
{
    if (you.species != SP_VAMPIRE)
        return (false);

    if (you.hunger_state > HS_SATIATED)
        return (false);

    // Satiated or less
    switch (spell)
    {
    case SPELL_ALTER_SELF:
    case SPELL_BERSERKER_RAGE:
    case SPELL_BLADE_HANDS:
    case SPELL_CURE_POISON:
    case SPELL_DRAGON_FORM:
    case SPELL_ICE_FORM:
    case SPELL_RESIST_POISON:
    case SPELL_SPIDER_FORM:
    case SPELL_STATUE_FORM:
    case SPELL_STONESKIN:
    case SPELL_TAME_BEASTS:
        return (true);
    default:
        return (false);
    }
}

bool is_prevented_teleport(spell_type spell)
{
    return ((spell == SPELL_BLINK
           || spell == SPELL_CONTROLLED_BLINK
           || spell == SPELL_TELEPORT_SELF)
           && scan_artefacts(ARTP_PREVENT_TELEPORTATION, false));
}

bool spell_is_uncastable(spell_type spell, std::string &msg)
{
    if (you.undead_or_demonic() && is_holy_spell(spell))
    {
        msg = "You can't use this type of magic!";
        return (true);
    }

    // Normally undead can't memorise these spells, so this check is
    // to catch those in Lich form.  As such, we allow the Lich form
    // to be extended here. - bwr
    if (spell != SPELL_NECROMUTATION && you_cannot_memorise(spell))
    {
        msg = "You cannot cast that spell in your current form!";
        return (true);
    }

    if (spell == SPELL_SYMBOL_OF_TORMENT && player_res_torment(true, false))
    {
        msg = "To torment others, one must first know what torment means.";
        return (true);
    }

    if (_vampire_cannot_cast(spell))
    {
        msg = "Your current blood level is not sufficient to cast that spell.";
        return (true);
    }

    return (false);
}

#ifdef WIZARD
static void _try_monster_cast(spell_type spell, int powc,
                              dist &spd, bolt &beam)
{
    if (monster_at(you.pos()))
    {
        mpr("Couldn't try casting monster spell because you're "
            "on top of a monster.");
        return;
    }

    monsters* mon = get_free_monster();
    if (!mon)
    {
        mpr("Couldn't try casting monster spell because there is "
            "no empty monster slot.");
        return;
    }

    mpr("Invalid player spell, attempting to cast it as monster spell.");

    mon->mname      = "Dummy Monster";
    mon->type       = MONS_HUMAN;
    mon->behaviour  = BEH_SEEK;
    mon->attitude   = ATT_FRIENDLY;
    mon->flags      = (MF_NO_REWARD | MF_JUST_SUMMONED | MF_SEEN
                       | MF_WAS_IN_VIEW | MF_HARD_RESET);
    mon->hit_points = you.hp;
    mon->hit_dice   = you.experience_level;
    mon->set_position(you.pos());
    mon->target     = spd.target;

    if (!spd.isTarget)
        mon->foe = MHITNOT;
    else if (!monster_at(spd.target))
    {
        if (spd.isMe())
            mon->foe = MHITYOU;
        else
            mon->foe = MHITNOT;
    }
    else
        mon->foe = mgrd(spd.target);

    mgrd(you.pos()) = mon->mindex();

    mons_cast(mon, beam, spell);

    mon->reset();
}
#endif // WIZARD

beam_type _spell_to_beam_type(spell_type spell)
{
    switch (spell)
    {
    case SPELL_FREEZE: return BEAM_COLD;
    default: break;
    }
    return BEAM_NONE;
}

int _setup_evaporate_cast()
{
    int rc = prompt_invent_item("Throw which potion?", MT_INVLIST, OBJ_POTIONS);

    if (prompt_failed(rc))
    {
        rc = -1;
    }
    else if (you.inv[rc].base_type != OBJ_POTIONS)
    {
        mpr("This spell works only on potions!");
        rc = -1;
    }
    else
    {
        mprf(MSGCH_PROMPT, "Where do you want to aim %s?",
             you.inv[rc].name(DESC_NOCAP_YOUR).c_str());
    }
    return rc;
}

static bool _can_cast_detect()
{
    if (player_in_mappable_area())
        return true;

    mpr("You feel momentarily disoriented.");
    return false;
}

static void _maybe_cancel_repeat(spell_type spell)
{
    switch (spell)
    {
    case SPELL_DELAYED_FIREBALL:
    case SPELL_TUKIMAS_DANCE:
    case SPELL_ALTER_SELF:
    case SPELL_PORTAL:
        crawl_state.cant_cmd_repeat(make_stringf("You can't repeat %s.",
                                                 spell_title(spell)));
        break;

    default:
        break;
    }
}

static spret_type _do_cast(spell_type spell, int powc,
                           const dist& spd, bolt& beam,
                           god_type god, int potion);

// Returns SPRET_SUCCESS if spell is successfully cast for purposes of
// exercising, SPRET_FAIL otherwise, or SPRET_ABORT if the player canceled
// the casting.
// Not all of these are actually real spells; invocations, decks, rods or misc.
// effects might also land us here.
// Others are currently unused or unimplemented.
spret_type your_spells(spell_type spell, int powc, bool allow_fail)
{
    ASSERT(!crawl_state.game_is_arena());

    const bool wiz_cast = (crawl_state.prev_cmd == CMD_WIZARD && !allow_fail);

    dist spd;
    bolt beam;
    beam.origin_spell = spell;

    // [dshaligram] Any action that depends on the spellcasting attempt to have
    // succeeded must be performed after the switch().

    std::string msg;
    if (!wiz_cast && spell_is_uncastable(spell, msg))
    {
        mpr(msg);
        return (SPRET_ABORT);
    }

    if (is_prevented_teleport(spell)
        && !yesno("You cannot teleport right now. Cast anyway?", true, 'n'))
    {
        return (SPRET_ABORT);
    }

    const unsigned int flags = get_spell_flags(spell);

    ASSERT(wiz_cast || !(flags & SPFLAG_TESTING));
    if ((flags & SPFLAG_TESTING) && !wiz_cast)
    {
        mprf(MSGCH_ERROR, "Spell %s is a testing spell, but you didn't use "
                          "the &Z wizard command; please file a bug report.",
             spell_title(spell));
        return (SPRET_ABORT);
    }

    int potion = -1;

    // XXX: This handles only some of the cases where spells need
    // targeting.  There are others that do their own that will be
    // missed by this (and thus will not properly ESC without cost
    // because of it).  Hopefully, those will eventually be fixed. - bwr
    if ((flags & SPFLAG_TARGETING_MASK) && spell != SPELL_PORTAL_PROJECTILE)
    {
        targ_mode_type targ =
              (testbits(flags, SPFLAG_HELPFUL) ? TARG_FRIEND : TARG_HOSTILE);

        if (testbits(flags, SPFLAG_NEUTRAL))
            targ = TARG_ANY;

        targeting_type dir  =
            (testbits(flags, SPFLAG_TARG_OBJ) ? DIR_TARGET_OBJECT :
             testbits(flags, SPFLAG_TARGET)   ? DIR_TARGET        :
             testbits(flags, SPFLAG_GRID)     ? DIR_TARGET        :
             testbits(flags, SPFLAG_DIR)      ? DIR_DIR           :
                                                DIR_NONE);

        const char *prompt = get_spell_target_prompt(spell);
        if (spell == SPELL_EVAPORATE)
        {
            potion = _setup_evaporate_cast();
            if (potion == -1)
                return (SPRET_ABORT);
        }
        else if (dir == DIR_DIR)
            mpr(prompt ? prompt : "Which direction? ", MSGCH_PROMPT);

        const bool needs_path = (!testbits(flags, SPFLAG_GRID)
                                 && !testbits(flags, SPFLAG_TARGET));

        const bool dont_cancel_me = testbits(flags, SPFLAG_HELPFUL);

        const int range = calc_spell_range(spell, powc, false);

        std::string title = "Casting: <white>";
        title += spell_title(spell);
        title += "</white>";

        if (!spell_direction(spd, beam, dir, targ, range,
                             needs_path, true, dont_cancel_me, prompt,
                             title.c_str(),
                             testbits(flags, SPFLAG_NOT_SELF)))
        {
            return (SPRET_ABORT);
        }

        beam.range = calc_spell_range(spell, powc, true);

        if (testbits(flags, SPFLAG_NOT_SELF) && spd.isMe())
        {
            if (spell == SPELL_TELEPORT_OTHER || spell == SPELL_POLYMORPH_OTHER
                || spell == SPELL_BANISHMENT)
            {
                mpr("Sorry, this spell works on others only.");
            }
            else
                canned_msg(MSG_UNTHINKING_ACT);

            return (SPRET_ABORT);
        }
    }

    // Enhancers only matter for calc_spell_power() and spell_fail().
    // Not sure about this: is it flavour or misleading? (jpeg)
    if (powc == 0 || allow_fail)
        _surge_power(spell);

    // Added this so that the passed in powc can have meaning. - bwr
    // Remember that most holy spells don't yet use powc!
    if (powc == 0)
        powc = calc_spell_power(spell, true);

    const god_type god =
        (crawl_state.is_god_acting()) ? crawl_state.which_god_acting()
                                      : GOD_NO_GOD;

    const int  loudness        = spell_noise(spell);
    const bool sound_at_caster = !(flags & SPFLAG_TARGETING_MASK);

    // Make some noise if it's actually the player casting.
    // NOTE: zappy() sets up noise for beams.
    if (god == GOD_NO_GOD)
        noisy(sound_at_caster ? loudness : 1, you.pos());

    if (allow_fail)
    {
        int spfl = random2avg(100, 3);

        if (you.religion != GOD_SIF_MUNA
            && you.penance[GOD_SIF_MUNA] && one_chance_in(20))
        {
            god_speaks(GOD_SIF_MUNA, "You feel a surge of divine spite.");

            // This will cause failure and increase the miscast effect.
            spfl = -you.penance[GOD_SIF_MUNA];

            // Reduced penance reduction here because casting spells
            // is a player controllable act. - bwr
            if (one_chance_in(12))
                dec_penance(GOD_SIF_MUNA, 1);
        }
        else if (spell_typematch(spell, SPTYP_NECROMANCY)
                 && you.religion != GOD_KIKUBAAQUDGHA
                 && you.penance[GOD_KIKUBAAQUDGHA]
                 && one_chance_in(20))
        {
            // And you thought you'd Necromutate your way out of penance...
            simple_god_message(" does not allow the disloyal to dabble in "
                               "death!", GOD_KIKUBAAQUDGHA);

            // The spell still goes through, but you get a miscast anyway.
            MiscastEffect(&you, -god, SPTYP_NECROMANCY,
                          (you.experience_level / 2) + (spell_mana(spell) * 2),
                          random2avg(88, 3), "the malice of Kikubaaqudgha");
        }

        const int spfail_chance = spell_fail(spell);
        // Divination mappings backfire in Labyrinths and the Abyss.
        if (testbits(env.level_flags, LFLAG_NO_MAGIC_MAP)
            && testbits(flags, SPFLAG_MAPPING))
        {
            mprf(MSGCH_WARN,
                 "The warped magic of this place twists your "
                 "spell in on itself!");
            spfl = spfail_chance / 2 - 1;
        }

        if (spfl < spfail_chance)
        {
            _spellcasting_side_effects(spell, true);

            mpr("You miscast the spell.");
            flush_input_buffer(FLUSH_ON_FAILURE);
            learned_something_new(HINT_SPELL_MISCAST);

            if (you.religion == GOD_SIF_MUNA
                && !player_under_penance()
                && you.piety >= 100 && x_chance_in_y(you.piety + 1, 150))
            {
                canned_msg(MSG_NOTHING_HAPPENS);
                return (SPRET_FAIL);
            }

            // All spell failures give a bit of magical radiation.
            // Failure is a function of power squared multiplied by how
            // badly you missed the spell.  High power spells can be
            // quite nasty: 9 * 9 * 90 / 500 = 15 points of
            // contamination!
            int nastiness = spell_mana(spell) * spell_mana(spell)
                                              * (spfail_chance - spfl) + 250;

            const int cont_points = div_rand_round(nastiness, 500);

            // miscasts are uncontrolled
            contaminate_player(cont_points);

            MiscastEffect(&you, NON_MONSTER, spell, spell_mana(spell),
                          spfail_chance - spfl);

            return (SPRET_FAIL);
        }
    }

    dprf("Spell #%d, power=%d", spell, powc);

    if (crawl_state.prev_cmd == CMD_CAST_SPELL && god == GOD_NO_GOD)
        _maybe_cancel_repeat(spell);

    switch (_do_cast(spell, powc, spd, beam, god, potion))
    {
    case SPRET_SUCCESS:
        _spellcasting_side_effects(spell);
        return (SPRET_SUCCESS);

    case SPRET_FAIL:
        ASSERT(false);
        return (SPRET_FAIL);

    case SPRET_ABORT:
        return (SPRET_ABORT);

    case SPRET_NONE:
#ifdef WIZARD
        if (you.wizard && !allow_fail && is_valid_spell(spell)
            && (flags & SPFLAG_MONSTER))
        {
            _try_monster_cast(spell, powc, spd, beam);
            return (SPRET_SUCCESS);
        }
#endif

        if (is_valid_spell(spell))
            mprf(MSGCH_ERROR, "Spell '%s' is not a player castable spell.",
                spell_title(spell));
        else
            mpr("Invalid spell!", MSGCH_ERROR);

        return (SPRET_ABORT);
    }

    return (SPRET_SUCCESS);
}

// Special-cased preconditions.
static bool _spell_zap_abort(spell_type spell, const bolt& beam)
{
    if (spell == SPELL_BANISHMENT && beam.target == you.pos())
    {
        mpr("You cannot banish yourself!");
        return (true);
    }
    return (false);
}

// Special-cased after-effects.
static void _spell_zap_effect(spell_type spell)
{
    // Casting pain costs 1 hp.
    // Deep Dwarves' damage reduction always blocks at least 1 hp.
    if (spell == SPELL_PAIN && you.species != SP_DEEP_DWARF)
        dec_hp(1, false);
}

// Spell failure check has been passed successfully.
// Returns SPRET_SUCCESS, SPRET_ABORT or SPRET_NONE (not a player spell).
static spret_type _do_cast(spell_type spell, int powc,
                           const dist& spd, bolt& beam,
                           god_type god, int potion)
{
    // First handle the zaps.
    zap_type zap = spell_to_zap(spell);
    if (zap != NUM_ZAPS)
    {
        if (_spell_zap_abort(spell, beam))
            return (SPRET_ABORT);
        if (!zapping(zap, spell_zap_power(spell, powc), beam, true))
            return (SPRET_ABORT);

        _spell_zap_effect(spell);
        return (SPRET_SUCCESS);
    }

    switch (spell)
    {
    // spells using burn_freeze()
    case SPELL_FREEZE:
        if (!burn_freeze(powc, _spell_to_beam_type(spell),
                         monster_at(you.pos() + spd.delta)))
        {
            return (SPRET_ABORT);
        }
        break;

    case SPELL_SANDBLAST:
        if (!cast_sandblast(powc, beam))
            return (SPRET_ABORT);
        break;

    case SPELL_BONE_SHARDS:
        if (!cast_bone_shards(powc, beam))
            return (SPRET_ABORT);
        break;

    case SPELL_VAMPIRIC_DRAINING:
        vampiric_drain(powc, spd);
        break;

    case SPELL_IOOD:
        if (!player_tracer(ZAP_IOOD, powc, beam))
            return (SPRET_ABORT);
        if (!cast_iood(&you, powc, &beam))
            return (SPRET_ABORT);
        break;

    // Clouds and explosions.
    case SPELL_MEPHITIC_CLOUD:
        if (!stinking_cloud(powc, beam))
            return (SPRET_ABORT);
        break;

    case SPELL_EVAPORATE:
        if (!cast_evaporate(powc, beam, potion))
            return SPRET_ABORT;
        break;

    case SPELL_POISONOUS_CLOUD:
        cast_big_c(powc, CLOUD_POISON, KC_YOU, beam);
        break;

    case SPELL_FREEZING_CLOUD:
        cast_big_c(powc, CLOUD_COLD, KC_YOU, beam);
        break;

    case SPELL_FIRE_STORM:
        if (!cast_fire_storm(powc, beam))
            return (SPRET_ABORT);
        break;

    case SPELL_HELLFIRE_BURST:
        if (!cast_hellfire_burst(powc, beam))
            return (SPRET_ABORT);
        break;

    case SPELL_FIREBALL:
        if (!fireball(powc, beam))
            return (SPRET_ABORT);
        break;

    case SPELL_DELAYED_FIREBALL:
        // This spell has two main advantages over Fireball:
        //
        // (1) The release is instantaneous, so monsters will not
        //     get an action before the player... this allows the
        //     player to hit monsters with a double fireball (this
        //     is why we only allow one delayed fireball at a time,
        //     if you want to allow for more, then the release should
        //     take at least some amount of time).
        //
        //     The casting of this spell still costs a turn.  So
        //     casting Delayed Fireball and immediately releasing
        //     the fireball is only slightly different from casting
        //     a regular Fireball (monsters act in the middle instead
        //     of at the end).  This is why we allow for the spell
        //     level discount so that Fireball is free with this spell
        //     (so that it only costs 7 levels instead of 13 to have
        //     both).
        //
        // (2) When the fireball is released, it is guaranteed to
        //     go off... the spell only fails at this point.  This can
        //     be a large advantage for characters who have difficulty
        //     casting Fireball in their standard equipment.  However,
        //     the power level for the actual fireball is determined at
        //     release, so if you do swap out your enhancers you'll
        //     get a less powerful ball when it's released. - bwr
        //
        if (!you.attribute[ATTR_DELAYED_FIREBALL])
        {
            // Okay, this message is weak but functional. - bwr
            mpr("You feel magically charged.");
            you.attribute[ATTR_DELAYED_FIREBALL] = 1;
        }
        else
        {
            mpr("You are already charged.");
            return (SPRET_ABORT);
        }
        break;

    // LOS spells
    case SPELL_SMITING:
        if (!cast_smiting(powc, beam.target))
            return (SPRET_ABORT);
        break;

    case SPELL_AIRSTRIKE:
        airstrike(powc, spd);
        break;

    case SPELL_FRAGMENTATION:
        if (!cast_fragmentation(powc, spd))
            return (SPRET_ABORT);
        break;

    case SPELL_PORTAL_PROJECTILE:
        if (!cast_portal_projectile(powc))
            return SPRET_ABORT;
        break;

    // other effects
    case SPELL_DISCHARGE:
        cast_discharge(powc);
        break;

    case SPELL_CHAIN_LIGHTNING:
        cast_chain_lightning(powc, &you);
        break;

    case SPELL_DISPERSAL:
        cast_dispersal(powc);
        break;

    case SPELL_SHATTER:
        cast_shatter(powc);
        break;

    case SPELL_SYMBOL_OF_TORMENT:
        torment(TORMENT_SPELL, you.pos());
        break;

    case SPELL_OZOCUBUS_REFRIGERATION:
        cast_refrigeration(powc);
        break;

    case SPELL_IGNITE_POISON:
        cast_ignite_poison(powc);
        break;

    // Summoning spells, and other spells that create new monsters.
    // If a god is making you cast one of these spells, any monsters
    // produced will count as god gifts.
    case SPELL_SUMMON_BUTTERFLIES:
        cast_summon_butterflies(powc, god);
        break;

    case SPELL_SUMMON_SMALL_MAMMALS:
        cast_summon_small_mammals(powc, god);
        break;

    case SPELL_STICKS_TO_SNAKES:
        cast_sticks_to_snakes(powc, god);
        break;

    case SPELL_SUMMON_SCORPIONS:
        cast_summon_scorpions(powc, god);
        break;

    case SPELL_SUMMON_SWARM:
        cast_summon_swarm(powc, god);
        break;

    case SPELL_CALL_CANINE_FAMILIAR:
        cast_call_canine_familiar(powc, god);
        break;

    case SPELL_SUMMON_ELEMENTAL:
        if (!cast_summon_elemental(powc, god))
            return (SPRET_ABORT);
        break;

    case SPELL_SUMMON_ICE_BEAST:
        cast_summon_ice_beast(powc, god);
        break;

    case SPELL_SUMMON_UGLY_THING:
        cast_summon_ugly_thing(powc, god);
        break;

    case SPELL_SUMMON_DRAGON:
        cast_summon_dragon(powc, god);
        break;

    case SPELL_TUKIMAS_DANCE:
        // Temporarily turns a wielded weapon into a dancing weapon.
        cast_tukimas_dance(powc, god);
        break;

    case SPELL_CONJURE_BALL_LIGHTNING:
        cast_conjure_ball_lightning(powc, god);
        break;

    case SPELL_CALL_IMP:
        cast_call_imp(powc, god);
        break;

    case SPELL_SUMMON_DEMON:
        cast_summon_demon(powc, god);
        break;

    case SPELL_DEMONIC_HORDE:
        cast_demonic_horde(powc, god);
        break;

    case SPELL_SUMMON_GREATER_DEMON:
        cast_summon_greater_demon(powc, god);
        break;

    case SPELL_SHADOW_CREATURES:
        cast_shadow_creatures(god);
        break;

    case SPELL_SUMMON_HORRIBLE_THINGS:
        cast_summon_horrible_things(powc, god);
        break;

    case SPELL_ANIMATE_SKELETON:
        mpr("You attempt to give life to the dead...");

        if (animate_remains(you.pos(), CORPSE_SKELETON, BEH_FRIENDLY,
                            MHITYOU, &you, "", god) < 0)
        {
            mpr("There is no skeleton here to animate!");
        }
        break;

    case SPELL_ANIMATE_DEAD:
        mpr("You call on the dead to rise...");

        animate_dead(&you, powc + 1, BEH_FRIENDLY, MHITYOU, &you, "", god);
        break;

    case SPELL_SIMULACRUM:
        cast_simulacrum(powc, god);
        break;

    case SPELL_TWISTED_RESURRECTION:
        cast_twisted_resurrection(powc, god);
        break;

    case SPELL_HAUNT:
        cast_haunt(powc, beam.target, god);
        break;

    case SPELL_DEATH_CHANNEL:
        cast_death_channel(powc, god);
        break;

    // Enchantments.
    case SPELL_CONFUSING_TOUCH:
        cast_confusing_touch(powc);
        break;

    case SPELL_CAUSE_FEAR:
        mass_enchantment(ENCH_FEAR, powc, MHITYOU);
        break;

    case SPELL_TAME_BEASTS:
        cast_tame_beasts(powc);
        break;

    case SPELL_INTOXICATE:
        cast_intoxicate(powc);
        break;

    case SPELL_MASS_CONFUSION:
        mass_enchantment(ENCH_CONFUSION, powc, MHITYOU);
        break;

    case SPELL_ENGLACIATION:
        cast_mass_sleep(powc);
        break;

    case SPELL_CONTROL_UNDEAD:
        mass_enchantment(ENCH_CHARM, powc, MHITYOU);
        break;

    case SPELL_ABJURATION:
        abjuration(powc);
        break;

    case SPELL_OLGREBS_TOXIC_RADIANCE:
        cast_toxic_radiance();
        break;

    case SPELL_MINOR_HEALING:
        if (cast_healing(5) < 0)
            return (SPRET_ABORT);
        break;

    case SPELL_MAJOR_HEALING:
        if (cast_healing(25) < 0)
            return (SPRET_ABORT);
        break;

    // Self-enchantments. (Spells that can only affect the player.)
    // Resistances.
    case SPELL_INSULATION:
        cast_insulation(powc);
        break;

    case SPELL_RESIST_POISON:
        cast_resist_poison(powc);
        break;

    case SPELL_SEE_INVISIBLE:
        cast_see_invisible(powc);
        break;

    case SPELL_CONTROL_TELEPORT:
        cast_teleport_control(powc);
        break;

    // Healing.
    case SPELL_CURE_POISON:
        cast_cure_poison(powc);
        break;

    // Weapon brands.
    case SPELL_SURE_BLADE:
        cast_sure_blade(powc);
        break;

    case SPELL_TUKIMAS_VORPAL_BLADE:
        if (!brand_weapon(SPWPN_VORPAL, powc))
            canned_msg(MSG_SPELL_FIZZLES);
        break;

    case SPELL_FIRE_BRAND:
        if (!brand_weapon(SPWPN_FLAMING, powc))
            canned_msg(MSG_SPELL_FIZZLES);
        break;

    case SPELL_FREEZING_AURA:
        if (!brand_weapon(SPWPN_FREEZING, powc))
            canned_msg(MSG_SPELL_FIZZLES);
        break;

    case SPELL_MAXWELLS_SILVER_HAMMER:
        if (!brand_weapon(SPWPN_DUMMY_CRUSHING, powc))
            canned_msg(MSG_SPELL_FIZZLES);
        break;

    case SPELL_POISON_WEAPON:
        if (!brand_weapon(SPWPN_VENOM, powc))
            canned_msg(MSG_SPELL_FIZZLES);
        break;

    case SPELL_EXCRUCIATING_WOUNDS:
        if (!brand_weapon(SPWPN_PAIN, powc))
            canned_msg(MSG_SPELL_FIZZLES);
        break;

    case SPELL_LETHAL_INFUSION:
        if (!brand_weapon(SPWPN_DRAINING, powc))
            canned_msg(MSG_SPELL_FIZZLES);
        break;

    case SPELL_WARP_BRAND:
        if (!brand_weapon(SPWPN_DISTORTION, powc))
            canned_msg(MSG_SPELL_FIZZLES);
        break;

    // Transformations.
    case SPELL_BLADE_HANDS:
        if (!transform(powc, TRAN_BLADE_HANDS))
            return (SPRET_ABORT);
        break;

    case SPELL_SPIDER_FORM:
        if (!transform(powc, TRAN_SPIDER))
            return (SPRET_ABORT);
        break;

    case SPELL_STATUE_FORM:
        if (!transform(powc, TRAN_STATUE))
            return (SPRET_ABORT);
        break;

    case SPELL_ICE_FORM:
        if (!transform(powc, TRAN_ICE_BEAST))
            return (SPRET_ABORT);
        break;

    case SPELL_DRAGON_FORM:
        if (!transform(powc, TRAN_DRAGON))
            return (SPRET_ABORT);
        break;

    case SPELL_NECROMUTATION:
        if (!transform(powc, TRAN_LICH))
            return (SPRET_ABORT);
        break;

    case SPELL_ALTER_SELF:
        if (!enough_hp(you.hp_max / 2, true))
        {
            mpr("Your body is in too poor a condition for this spell "
                 "to function.");
            return (SPRET_ABORT);
        }

        mpr("Your body is suffused with transfigurative energy!");

        set_hp(1 + random2(you.hp), false);

        if (mutate(RANDOM_MUTATION, false))
            did_god_conduct(DID_DELIBERATE_MUTATING, 2 + random2(3));
        else
            mpr("Odd... you don't feel any different.");
        break;

    // General enhancement.
    case SPELL_BERSERKER_RAGE:
        if (!berserk_check_wielded_weapon())
           return (SPRET_ABORT);

        cast_berserk();
        break;

    case SPELL_REGENERATION:
        cast_regen(powc);
        break;

    case SPELL_REPEL_MISSILES:
        missile_prot(powc);
        break;

    case SPELL_DEFLECT_MISSILES:
        deflection(powc);
        break;

    case SPELL_SWIFTNESS:
        cast_swiftness(powc);
        break;

    case SPELL_LEVITATION:
        potion_effect(POT_LEVITATION, powc);
        break;

    case SPELL_FLY:
        cast_fly(powc);
        break;

    case SPELL_STONESKIN:
        cast_stoneskin(powc);
        break;

    case SPELL_STONEMAIL:
        stone_scales(powc);
        break;

    case SPELL_CONDENSATION_SHIELD:
        cast_condensation_shield(powc);
        break;

    case SPELL_OZOCUBUS_ARMOUR:
        ice_armour(powc, false);
        break;

    case SPELL_PHASE_SHIFT:
        cast_phase_shift(powc);
        break;

    case SPELL_SILENCE:
        cast_silence(powc);
        break;

    // other
    case SPELL_SELECTIVE_AMNESIA:
        crawl_state.cant_cmd_repeat("You can't repeat selective amnesia.");

        // Sif Muna power calls with true
        if (!cast_selective_amnesia(false))
            return (SPRET_ABORT);
        break;

    case SPELL_EXTENSION:
        extension(powc);
        break;

    case SPELL_BORGNJORS_REVIVIFICATION:
        cast_revivification(powc);
        break;

    case SPELL_SUBLIMATION_OF_BLOOD:
        cast_sublimation_of_blood(powc);
        break;

    case SPELL_DEATHS_DOOR:
        cast_deaths_door(powc);
        break;

    case SPELL_RING_OF_FLAMES:
        cast_ring_of_flames(powc);
        break;

    // Escape spells.
    case SPELL_BLINK:
        random_blink(god != GOD_XOM);
        break;

    case SPELL_TELEPORT_SELF:
        you_teleport();
        break;

    case SPELL_CONTROLLED_BLINK:
        if (blink(powc, true) == -1)
            return (SPRET_ABORT);
        break;

    // Utility spells.
    case SPELL_DETECT_SECRET_DOORS:
        if (_can_cast_detect())
            cast_detect_secret_doors(powc);
        break;

    case SPELL_DETECT_TRAPS:
        if (_can_cast_detect())
            mprf("You detect %s", (detect_traps(powc) > 0) ? "traps!"
                                                           : "nothing.");
        break;

    case SPELL_DETECT_ITEMS:
        if (_can_cast_detect())
            mprf("You detect %s", (detect_items(powc) > 0) ? "items!"
                                                           : "nothing.");
        break;

    case SPELL_DETECT_CREATURES:
    {
        if (!_can_cast_detect())
            break;

        const int prev_detected = count_detected_mons();
        const int num_creatures = detect_creatures(powc);

        if (!num_creatures)
            mpr("You detect nothing.");
        else if (num_creatures == prev_detected)
        {
            // This is not strictly true. You could have cast
            // Detect Creatures with a big enough fuzz that the detected
            // glyph is still on the map when the original one has been
            // killed. Then another one is spawned, so the number is
            // the same as before. There's no way we can check this however.
            mpr("You detect no further creatures.");
        }
        else
            mpr("You detect creatures!");
        break;
    }

    case SPELL_PROJECTED_NOISE:
        project_noise();
        break;

    case SPELL_CONJURE_FLAME:
        if (!conjure_flame(powc, beam.target))
            return (SPRET_ABORT);
        break;

    case SPELL_PASSWALL:
        if (!cast_passwall(spd.delta, powc))
            return (SPRET_ABORT);
        break;

    case SPELL_APPORTATION:
        if (!cast_apportation(powc, beam.target))
            return (SPRET_ABORT);
        break;

    case SPELL_RECALL:
        recall(0);
        break;

    case SPELL_PORTAL:
        if (crawl_state.game_is_sprint() || (portal() == -1))
            return (SPRET_ABORT);
        break;

    case SPELL_CORPSE_ROT:
        corpse_rot();
        break;

    case SPELL_FULSOME_DISTILLATION:
        if (!cast_fulsome_distillation(powc))
            return (SPRET_ABORT);
        break;

    default:
        return (SPRET_NONE);
    }

    return (SPRET_SUCCESS);
}

void exercise_spell(spell_type spell, bool spc, bool success)
{
    // (!success) reduces skill increase for miscast spells
    int skill;
    int exer = 0;
    int workout = 0;

    unsigned int disciplines = get_spell_disciplines(spell);

    //jmf: evil evil evil -- exclude HOLY bit
    disciplines &= (~SPTYP_HOLY);

    int skillcount = count_bits( disciplines );

    if (!success)
        skillcount += 4 + random2(10);

    const int diff = spell_difficulty(spell);

    // Fill all disciplines into a vector, then shuffle the vector, and
    // exercise skills in that random order. That way, small xp pools
    // don't always train exclusively the first skill.
    std::vector<int> disc;
    for (int ndx = 0; ndx <= SPTYP_LAST_EXPONENT; ndx++)
    {
        if (!spell_typematch( spell, 1 << ndx ))
            continue;

        disc.push_back(ndx);
    }
    std::random_shuffle(disc.begin(), disc.end());

    for (unsigned int k = 0; k < disc.size(); ++k)
    {
        int ndx = disc[k];
        skill = spell_type2skill( 1 << ndx );
        workout = (random2(1 + diff) / skillcount);

        if (!one_chance_in(5))
            workout++;       // most recently, this was an automatic add {dlb}

        const int exercise_amount = exercise( skill, workout );
        exer      += exercise_amount;
    }

    /* ******************************************************************
       Other recent formulae for the above:

       * workout = random2(spell_difficulty(spell_ex)
       * (10 + (spell_difficulty(spell_ex) * 2 )) / 10 / spellsy + 1);

       * workout = spell_difficulty(spell_ex)
       * (15 + spell_difficulty(spell_ex)) / 15 / spellsy;

       spellcasting had also been generally exercised at the same time
       ****************************************************************** */

    if (spc)
    {
        const int exercise_amount =
            exercise(SK_SPELLCASTING, one_chance_in(3) ? 1
                            : random2(1 + random2(diff)));
        exer      += exercise_amount;
    }

    if (exer)
        did_god_conduct(DID_SPELL_PRACTISE, exer);
}

const char* failure_rate_to_string( int fail )
{
    return (fail == 100) ? "Useless"   : // 0% success chance
           (fail > 77)   ? "Terrible"  : // 0-5%
           (fail > 71)   ? "Cruddy"    : // 5-10%
           (fail > 64)   ? "Bad"       : // 10-20%
           (fail > 59)   ? "Very Poor" : // 20-30%
           (fail > 50)   ? "Poor"      : // 30-50%
           (fail > 40)   ? "Fair"      : // 50-70%
           (fail > 35)   ? "Good"      : // 70-80%
           (fail > 28)   ? "Very Good" : // 80-90%
           (fail > 22)   ? "Great"     : // 90-95%
           (fail >  0)   ? "Excellent"   // 95-100%
                         : "Perfect";    // 100%
}

static unsigned int _breakpoint_rank(int val, const int breakpoints[],
                            unsigned int num_breakpoints)
{
    unsigned int result = 0;
    while (result < num_breakpoints && val >= breakpoints[result])
        ++result;

    return result;
}

const char* spell_hunger_string(spell_type spell)
{
    if (you.is_undead == US_UNDEAD)
        return ("N/A");

    const int hunger = spell_hunger(spell);

    // Spell hunger is "Fruit" if casting the spell five times costs at
    // most one "Fruit".
    const char* hunger_descriptions[] = {
        "None", "Sultana", "Strawberry", "Choko", "Honeycomb", "Ration"
    };

    const int breakpoints[] = { 1, 15, 41, 121, 401 };

    return (hunger_descriptions[_breakpoint_rank(hunger, breakpoints,
                                                 ARRAYSZ(breakpoints))]);
}

int spell_power_colour(spell_type spell)
{
    const int powercap = spell_power_cap(spell);
    if (powercap == 0)
        return DARKGREY;
    const int power = calc_spell_power(spell, true);
    if (power >= powercap)
        return WHITE;
    if (power * 3 < powercap)
        return RED;
    if (power * 3 < powercap * 2)
        return YELLOW;
    return GREEN;
}

static int _power_to_barcount( int power )
{
    if (power == -1)
        return -1;

    const int breakpoints[] = { 5, 10, 15, 25, 35, 50, 75, 100, 150 };
    return (_breakpoint_rank(power, breakpoints, ARRAYSZ(breakpoints)) + 1);
}

int spell_power_bars( spell_type spell )
{
    const int cap = spell_power_cap(spell);
    if (cap == 0)
        return -1;
    const int power = std::min(calc_spell_power(spell, true), cap);
    return _power_to_barcount(power);
}

#ifdef WIZARD
static std::string _wizard_spell_power_numeric_string(spell_type spell)
{
    const int cap = spell_power_cap(spell);
    if (cap == 0)
        return "N/A";
    const int power = std::min(calc_spell_power(spell, true), cap);
    return make_stringf("%d (%d)", power, cap);
}
#endif

std::string spell_power_string(spell_type spell)
{
#ifdef WIZARD
    if (you.wizard)
        return _wizard_spell_power_numeric_string(spell);
#endif

    const int numbars = spell_power_bars(spell);
    const int capbars = _power_to_barcount(spell_power_cap(spell));
    ASSERT(numbars <= capbars);
    if (numbars < 0)
        return "N/A";
    else
        return std::string(numbars, '#') + std::string(capbars - numbars, '.');
}

int calc_spell_range(spell_type spell, int power, bool real_cast)
{
    if (power == 0)
        power = calc_spell_power(spell, true);
    const int range = spell_range(spell, power, real_cast);

    return (range);
}

std::string spell_range_string(spell_type spell)
{
    const int cap      = spell_power_cap(spell);
    const int range    = calc_spell_range(spell);
    const int maxrange = spell_range(spell, cap, false);

    if (range < 0)
        return "N/A";
    else
    {
        return std::string("@") + std::string(range - 1, '-')
               + std::string(">") + std::string(maxrange - range, '.');
    }
}

std::string spell_schools_string(spell_type spell)
{
    std::string desc;

    bool already = false;
    for (int i = 0; i <= SPTYP_LAST_EXPONENT; ++i)
    {
        if (spell_typematch(spell, (1<<i)))
        {
            if (already)
                desc += "/";
            desc += spelltype_long_name(1 << i);
            already = true;
        }
    }

    return (desc);
}
