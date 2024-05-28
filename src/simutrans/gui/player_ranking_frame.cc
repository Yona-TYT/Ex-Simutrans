/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../macros.h"
#include "../simcolor.h"
#include "../world/simworld.h"
#include "../dataobj/environment.h"
#include "../dataobj/translator.h"
#include "../builder/wegbauer.h"

#include "simwin.h"
#include "money_frame.h" // for the finances

#include "player_ranking_frame.h"


uint8 player_ranking_frame_t::transport_type_option = TT_ALL;

// Text should match that of the finance dialog
static const char* cost_type_name[player_ranking_frame_t::MAX_PLAYER_RANKING_CHARTS] =
{
	"Revenue",
	"Ops Profit",
	"Margin (%%)",
	"Passagiere",
	"Post",
	"Goods",
	"Cash",
	"Net Wealth",
	"Convoys",
//	"Vehicles",
//	"Stops"
// "way_distances" // Way kilometrage
// "travel_distance"
};

static const uint8 cost_type[player_ranking_frame_t::MAX_PLAYER_RANKING_CHARTS] =
{
	gui_chart_t::MONEY,
	gui_chart_t::MONEY,
	gui_chart_t::PERCENT,
	gui_chart_t::STANDARD,
	gui_chart_t::STANDARD,
	gui_chart_t::STANDARD,
	gui_chart_t::MONEY,
	gui_chart_t::MONEY,
	gui_chart_t::STANDARD
};

static const uint8 cost_type_color[player_ranking_frame_t::MAX_PLAYER_RANKING_CHARTS] =
{
	COL_REVENUE,
	COL_PROFIT,
	COL_MARGIN,
	COL_LIGHT_PURPLE,
	COL_TRANSPORTED,
	COL_BROWN,
	COL_CASH,
	COL_WEALTH,
	COL_NEW_VEHICLES
};

// is_atv=1, ATV:vehicle finance record, ATC:common finance record
static const uint8 history_type_idx[player_ranking_frame_t::MAX_PLAYER_RANKING_CHARTS*2] =
{
	1,ATV_REVENUE,
	1,ATV_OPERATING_PROFIT,
	1,ATV_PROFIT_MARGIN,
	1,ATV_TRANSPORTED_PASSENGER,
	1,ATV_TRANSPORTED_MAIL,
	1,ATV_TRANSPORTED_GOOD,
	0,ATC_CASH,
	0,ATC_NETWEALTH,
	0,ATC_ALL_CONVOIS,
//	1,ATV_VEHICLES,
//	0,ATC_HALTS
};

static int compare_atv(uint8 player_nr_a, uint8 player_nr_b, uint8 atv_index) {
	int comp = 0;
	player_t* a_player = world()->get_player(player_nr_a);
	player_t* b_player = world()->get_player(player_nr_b);

	if (a_player && b_player) {
		comp = b_player->get_finance()->get_history_veh_year((transport_type)player_ranking_frame_t::transport_type_option, 1, atv_index) - a_player->get_finance()->get_history_veh_year((transport_type)player_ranking_frame_t::transport_type_option, 1, atv_index);
		if (comp==0) {
			comp = b_player->get_finance()->get_history_veh_year((transport_type)player_ranking_frame_t::transport_type_option, 0, atv_index) - a_player->get_finance()->get_history_veh_year((transport_type)player_ranking_frame_t::transport_type_option, 0, atv_index);
		}
	}
	if (comp==0) {
		comp = player_nr_b - player_nr_a;
	}
	return comp;
}

static int compare_atc(uint8 player_nr_a, uint8 player_nr_b, uint8 atc_index) {
	int comp = 0;
	player_t* a_player = world()->get_player(player_nr_a);
	player_t* b_player = world()->get_player(player_nr_b);

	if (a_player && b_player) {
		comp = b_player->get_finance()->get_history_com_year(1, atc_index) - a_player->get_finance()->get_history_com_year(1, atc_index);
		if (comp == 0) {
			comp = b_player->get_finance()->get_history_com_year(0, atc_index) - a_player->get_finance()->get_history_com_year(0, atc_index);
		}
	}
	if (comp == 0) {
		comp = player_nr_b - player_nr_a;
	}
	return comp;
}

static int compare_revenue(player_button_t* const& a, player_button_t* const& b) {
	return compare_atv(a->get_player_nr(), b->get_player_nr(), ATV_REVENUE);
}
static int compare_profit(player_button_t* const& a, player_button_t* const& b) {
	return compare_atv(a->get_player_nr(), b->get_player_nr(), ATV_PROFIT);
}
static int compare_transport_pax(player_button_t* const& a, player_button_t* const& b) {
	return compare_atv(a->get_player_nr(), b->get_player_nr(), ATV_TRANSPORTED_PASSENGER);
}
static int compare_transport_mail(player_button_t* const& a, player_button_t* const& b) {
	return compare_atv(a->get_player_nr(), b->get_player_nr(), ATV_TRANSPORTED_MAIL);
}
static int compare_transport_goods(player_button_t* const& a, player_button_t* const& b) {
	return compare_atv(a->get_player_nr(), b->get_player_nr(), ATV_TRANSPORTED_GOOD);
}
static int compare_margin(player_button_t* const& a, player_button_t* const& b) {
	return compare_atv(a->get_player_nr(), b->get_player_nr(), ATV_PROFIT_MARGIN);
}
static int compare_convois(player_button_t* const& a, player_button_t* const& b) {
	return compare_atv(a->get_player_nr(), b->get_player_nr(), ATC_ALL_CONVOIS);
}

static int compare_cash(player_button_t* const &a, player_button_t* const &b) {
	return compare_atc(a->get_player_nr(), b->get_player_nr(), ATC_CASH);
}
static int compare_netwealth(player_button_t* const& a, player_button_t* const& b) {
	return compare_atc(a->get_player_nr(), b->get_player_nr(), ATC_NETWEALTH);
}
/*
static int compare_halts(player_button_t* const& a, player_button_t* const& b) {
	return compare_atc(a->get_player_nr(), b->get_player_nr(), ATC_HALTS);
}
*/

player_button_t::player_button_t(uint8 player_nr_)
{
	player_nr = player_nr_;
	init(button_t::box_state | button_t::flexible, NULL, scr_coord(0,0), D_BUTTON_SIZE);
	set_tooltip("Click to open the finance dialog.");
	update();
}

void player_button_t::update()
{
	player_t* player = world()->get_player(player_nr);
	if (player) {
		set_text(player->get_name());
		background_color = color_idx_to_rgb(player->get_player_color1() + env_t::gui_player_color_bright);
		enable();
		set_visible(true);
	}
	else {
		set_visible(false);
		disable();
	}
}


player_ranking_frame_t::player_ranking_frame_t(uint8 selected_player_nr) :
	gui_frame_t(translator::translate("Player ranking")),
	scrolly(&cont_players, true, true)
{
	selected_player = selected_player_nr;
	last_year = world()->get_last_year();
	set_table_layout(1,0);

	add_table(2,1)->set_alignment(ALIGN_TOP);
	{
		add_component(&chart);
		chart.set_dimension( clamp(welt->get_last_year() - welt->get_settings().get_starting_year(), 2, MAX_PLAYER_HISTORY_YEARS), 10000);
		chart.set_seed(welt->get_last_year());
		chart.set_background(SYSCOL_CHART_BACKGROUND);

		cont_players.set_table_layout(3,0);
		cont_players.set_alignment(ALIGN_CENTER_H);
		cont_players.set_margin(scr_size(0,0), scr_size(D_SCROLLBAR_WIDTH + D_H_SPACE, D_SCROLLBAR_HEIGHT));

		for (int np = 0; np < MAX_PLAYER_COUNT; np++) {
			if (np == PUBLIC_PLAYER_NR) continue;
			if (welt->get_player(np) ) {
				player_button_t* b = new player_button_t(np);
				b->add_listener(this);
				if (np==selected_player) {
					b->pressed = true;
				}
				buttons.append(b);
			}
		}

		sort_player();

		add_table(2,2);
		{
			new_component<gui_label_t>("Last year's ranking", SYSCOL_TEXT);
			transport_type_c.set_focusable(false);

			for (int i = 0, count = 0; i < TT_OTHER; ++i) {
				transport_type_c.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate(finance_t::transport_type_values[i]), SYSCOL_TEXT);
				transport_types[count++] = i;
			}
			transport_type_c.add_listener(this);
			transport_type_c.set_selection(0);
			add_component(&transport_type_c);

			empty.set_visible(false);
			add_component(&empty);

			scrolly.set_maximize(true);
			add_component(&scrolly,2);
		}
		end_table();
	}
	end_table();

	add_table(4,0)->set_force_equal_columns(true);
	{
		for (uint8 i = 0; i < MAX_PLAYER_RANKING_CHARTS; i++) {
			bt_charts[i].init(button_t::box_state | button_t::flexible, cost_type_name[i]);
			bt_charts[i].background_color = color_idx_to_rgb(cost_type_color[i]);
			if (i== selected_item) bt_charts[i].pressed=true;
			bt_charts[i].add_listener(this);
			add_component(&bt_charts[i]);
		}
	}
	end_table();

	update_chart();

	set_windowsize(get_min_windowsize());
	set_resizemode(diagonal_resize);
}

player_ranking_frame_t::~player_ranking_frame_t()
{
	while (!buttons.empty()) {
		player_button_t* b = buttons.remove_first();
		cont_players.remove_component(b);
		delete b;
	}
}


void player_ranking_frame_t::sort_player()
{
	cont_players.remove_all();
	switch (selected_item)
	{
		case PR_REVENUE:
			buttons.sort(compare_revenue);
			break;
		case PR_PROFIT:
			buttons.sort(compare_profit);
			break;
		case PR_TRANSPORT_PAX:
			buttons.sort(compare_transport_pax);
			break;
		case PR_TRANSPORT_MAIL:
			buttons.sort(compare_transport_mail);
			break;
		case PR_TRANSPORT_GOODS:
			buttons.sort(compare_transport_goods);
			break;
		case PR_CASH:
			buttons.sort(compare_cash);
			break;
		case PR_NETWEALTH:
			buttons.sort(compare_netwealth);
			break;
		case PR_MARGIN:
			buttons.sort(compare_margin);
			break;
		default:
		case PR_CONVOIS:
			buttons.sort(compare_convois);
			break;
	}
	uint8 count = 0;
	for (uint i = 0; i < buttons.get_count(); i++) {
		const uint8 player_nr = buttons.at(i)->get_player_nr();
		// Exclude players who are not in the competition
		if( player_nr==PUBLIC_PLAYER_NR || is_chart_table_zero( player_nr ) ) {
			continue;
		}
		count++;

		switch (i)
		{
			case 0:
				cont_players.new_component<gui_label_t>("1", color_idx_to_rgb(COL_YELLOW), gui_label_t::centered);
				break;
			case 1:
				cont_players.new_component<gui_label_t>("2", 0, gui_label_t::centered);
				break;
			case 2:
				cont_players.new_component<gui_label_t>("3", 0, gui_label_t::centered);
				break;

			default:
				gui_label_buf_t* lb = cont_players.new_component<gui_label_buf_t>(SYSCOL_TEXT, gui_label_t::centered);
				lb->buf().printf("%u", count);
				lb->update();
				lb->set_min_width(lb->get_min_size().w);
				break;
		}

		if( player_nr != selected_player ) {
			buttons.at(i)->pressed = false;
		}
		cont_players.add_component(buttons.at(i));

		cont_players.add_component(&lb_player_val[player_nr]);
	}


	if (count) {
		// update labels
		const bool is_atv = history_type_idx[selected_item * 2];

		// check totals.
		sint64 total=0;

		for (int np = 0; np < MAX_PLAYER_COUNT - 1; np++) {
			if( player_t* player = welt->get_player(np) ) {
				const finance_t* finance = player->get_finance();
				const bool is_atv = history_type_idx[selected_item * 2];
				PIXVAL color = SYSCOL_TEXT;
				sint64 value = is_atv ? finance->get_history_veh_year((transport_type)player_ranking_frame_t::transport_type_option, 1, history_type_idx[selected_item * 2 + 1])
					: finance->get_history_com_year(1, history_type_idx[selected_item * 2 + 1]);
				switch (cost_type[selected_item]) {
					case gui_chart_t::MONEY:
						lb_player_val[np].buf().append_money(value / 100.0);
						color = value >= 0 ? (value > 0 ? MONEY_PLUS : SYSCOL_TEXT_UNUSED) : MONEY_MINUS;
						break;
					case gui_chart_t::PERCENT:
						lb_player_val[np].buf().append(value / 100.0, 2);
						lb_player_val[np].buf().append("%");
						color = value >= 0 ? (value > 0 ? MONEY_PLUS : SYSCOL_TEXT_UNUSED) : MONEY_MINUS;
						break;
					case gui_chart_t::STANDARD:
					default:
						lb_player_val[np].buf().append(value,0);
						break;
				}
				if (total) {
					lb_player_val[np].buf().printf(" (%.1f%%)", (double)value*100/total);
				}
				lb_player_val[np].set_color(color);
				lb_player_val[np].set_align(gui_label_t::right);
				lb_player_val[np].update();
			}
		}
		scrolly.set_visible(true);
		cont_players.set_size(cont_players.get_min_size());
		scrolly.set_size(cont_players.get_min_size());
		scrolly.set_min_width(cont_players.get_min_size().w);
	}
	else {
		cont_players.new_component<gui_fill_t>(); // keep the size so that the combo box position does not move
		scrolly.set_visible(false);
	}
}

bool player_ranking_frame_t::is_chart_table_zero(uint8 player_nr) const
{
	// search for any non-zero values
	if( player_t* player = welt->get_player(player_nr) ) {
		const finance_t* finance = player->get_finance();
		const bool is_atv = history_type_idx[selected_item*2];
		for (int y = 0; y < MAX_PLAYER_HISTORY_MONTHS; y++) {
			sint64 val = is_atv ? finance->get_history_veh_year((transport_type)player_ranking_frame_t::transport_type_option, y, history_type_idx[selected_item * 2 + 1])
				: finance->get_history_com_year(y, history_type_idx[selected_item * 2 + 1]);
			if (val) return false;
		}
	}

	return true;
}

/**
 * This method is called if an action is triggered
 */
bool player_ranking_frame_t::action_triggered( gui_action_creator_t *comp,value_t p )
{
	// Check the GUI list of buttons
	// player filter
	for (auto bt : buttons) {
		if (comp==bt) {
			if (player_t* p = welt->get_player(bt->get_player_nr())) {
				create_win(new money_frame_t(p), w_info, magic_finances_t + p->get_player_nr());
			}
			return true;
		}
	}

	// chart selector
	for (uint8 i = 0; i < MAX_PLAYER_RANKING_CHARTS; i++) {
		if (comp == &bt_charts[i]) {
			if (bt_charts[i].pressed) {
				// nothing to do
				return true;
			}
			bt_charts[i].pressed ^= 1;
			selected_item = i;
			update_chart();
			return true;
		}
	}

	if(  comp == &transport_type_c) {
		int tmp = transport_type_c.get_selection();
		if((0 <= tmp) && (tmp < transport_type_c.count_elements())) {
			transport_type_option = (uint8)tmp;
			transport_type_option = transport_types[tmp];
			update_chart();
		}
		return true;
	}

	return true;
}


void player_ranking_frame_t::update_chart()
{
	// deselect chart buttons
	for (uint8 i=0; i < MAX_PLAYER_RANKING_CHARTS; i++) {
		if (i != selected_item) {
			bt_charts[i].pressed = false;
		}
	}

	// need to clear the chart once to update the suffix and digit
	chart.remove_curves();
	for (int np = 0; np < MAX_PLAYER_COUNT - 1; np++) {
		if (np == PUBLIC_PLAYER_NR) continue;
		if ( player_t* player = welt->get_player(np) ) {
			if (is_chart_table_zero(np)) continue;
			// create chart
			const int curve_type = (int)cost_type[selected_item];
			const int curve_precision = (curve_type == gui_chart_t::STANDARD) ? 0 : (curve_type == gui_chart_t::MONEY || curve_type == gui_chart_t::PERCENT) ? 2 : 1;
//			gui_chart_t::chart_marker_t marker = (np==selected_player) ? gui_chart_t::square : gui_chart_t::none;
			chart.add_curve(color_idx_to_rgb( player->get_player_color1()+3), *p_chart_table, MAX_PLAYER_COUNT-1, np, MAX_PLAYER_HISTORY_YEARS, curve_type, true, false, curve_precision, NULL);
		}
	}

	const bool is_atv = history_type_idx[selected_item*2];
	for (int np = 0; np < MAX_PLAYER_COUNT - 1; np++) {
		if ( player_t* player = welt->get_player(np) ) {
			// update chart records
			for (int y = 0; y < MAX_PLAYER_HISTORY_YEARS; y++) {
				const finance_t* finance = player->get_finance();
				p_chart_table[y][np] = is_atv ? finance->get_history_veh_year((transport_type)player_ranking_frame_t::transport_type_option, y, history_type_idx[selected_item*2+1])
					: finance->get_history_com_year(y, history_type_idx[selected_item*2+1]);
			}
			chart.show_curve(np);
		}
	}
	transport_type_c.set_visible(is_atv);
	empty.set_visible(!is_atv);

	sort_player();

	reset_min_windowsize();
}


void player_ranking_frame_t::update_buttons()
{
	for (auto bt : buttons) {
		bt->update();
	}
	scrolly.set_min_width(cont_players.get_min_size().w);
}

/**
 * Draw the component
 */
void player_ranking_frame_t::draw(scr_coord pos, scr_size size)
{
	if (last_year != world()->get_last_year()) {
		last_year = world()->get_last_year();
		chart.set_seed(last_year);
		update_chart();
	}

	gui_frame_t::draw(pos, size);
}
