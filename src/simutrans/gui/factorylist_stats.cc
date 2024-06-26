/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "factorylist_stats.h"
#include "../simskin.h"
#include "../simfab.h"
#include "../world/simworld.h"
#include "../display/viewport.h"
#include "../tool/simmenu.h"
#include "../simskin.h"

#include "../builder/goods_manager.h"
#include "../descriptor/skin_desc.h"
#include "../utils/cbuffer.h"
#include "../utils/simstring.h"


sint16 factorylist_stats_t::sort_mode = factorylist::by_name;
bool factorylist_stats_t::reverse = false;


factorylist_stats_t::factorylist_stats_t(fabrik_t *fab)
{
	this->fab = fab;
	// pos button
	set_table_layout(0,1);
	button_t *b = new_component<button_t>();
	b->set_typ(button_t::posbutton_automatic);
	b->set_targetpos3d(fab->get_pos());
	// indicator bar
	add_component(&indicator);
	indicator.set_max_size(scr_size(D_INDICATOR_WIDTH,D_INDICATOR_HEIGHT));
	// boost images
	if (fab->get_desc()->get_electric_boost() ) {
		boost_electric.set_image(skinverwaltung_t::electricity->get_image_id(0), true);
		add_component(&boost_electric);
	}
	if (fab->get_desc()->get_pax_boost() ) {
		boost_passenger.set_image(skinverwaltung_t::passengers->get_image_id(0), true);
		add_component(&boost_passenger);
	}
	if (fab->get_desc()->get_mail_boost() ) {
		boost_mail.set_image(skinverwaltung_t::mail->get_image_id(0), true);
		add_component(&boost_mail);
	}
	// factory name
	update_label();
	add_component(&label);
}


void factorylist_stats_t::update_label()
{
	cbuffer_t &buf = label.buf();
	buf.append(fab->get_name());
	buf.append(" (");

	if (!fab->get_input().empty()) {
		buf.printf( "%i+%i", fab->get_total_in(), fab->get_total_transit() );
	}
	else {
		buf.append("-");
	}
	buf.append(", ");

	if (!fab->get_output().empty()) {
		buf.append(fab->get_total_out(),0);
	}
	else {
		buf.append("-");
	}
	buf.append(", ");

	buf.append(fab->get_current_production(),0);
	buf.append(") ");
	label.update();
}


bool factorylist_stats_t::is_valid() const
{
	return world()->get_fab_list().is_contained(fab);
}


bool factorylist_stats_t::infowin_event(const event_t * ev)
{
	bool swallowed = gui_aligned_container_t::infowin_event(ev);

	if (!swallowed) {
		// either open dialog or goto (with control or right click)
		if (IS_LEFTRELEASE(ev)) {
			if ((event_get_last_control_shift() ^ tool_t::control_invert) == 2) {
				world()->get_viewport()->change_world_position(fab->get_pos());
			}
			else {
				fab->open_info_window();
			}
			return true;
		}
		if (IS_RIGHTRELEASE(ev)) {
			world()->get_viewport()->change_world_position(fab->get_pos());
			return true;
		}
	}
	return swallowed;
}


void factorylist_stats_t::draw(scr_coord pos)
{
	update_label();

	// boost stuff
	boost_electric.set_transparent(fab->get_prodfactor_electric()>0 ? 0 : TRANSPARENT50_FLAG | OUTLINE_FLAG | SYSCOL_IMAGE_TRANSPARENCY);
	boost_passenger.set_transparent(fab->get_prodfactor_pax()>0 ? 0 : TRANSPARENT50_FLAG | OUTLINE_FLAG | SYSCOL_IMAGE_TRANSPARENCY);
	boost_mail.set_transparent(fab->get_prodfactor_mail()>0 ? 0 : TRANSPARENT50_FLAG | OUTLINE_FLAG | SYSCOL_IMAGE_TRANSPARENCY);
	indicator.set_color( color_idx_to_rgb(fabrik_t::status_to_color[fab->get_status()]) );

	gui_aligned_container_t::draw(pos);
}


bool factorylist_stats_t::compare(const gui_component_t *aa, const gui_component_t *bb)
{
	const factorylist_stats_t* fa = dynamic_cast<const factorylist_stats_t*>(aa);
	const factorylist_stats_t* fb = dynamic_cast<const factorylist_stats_t*>(bb);
	// good luck with mixed lists
	assert(fa != NULL  &&  fb != NULL);
	fabrik_t *a=fa->fab, *b=fb->fab;

	int cmp;
	switch (sort_mode) {
		default:
		case factorylist::by_name:
			cmp = 0;
			break;

		case factorylist::by_input:
		{
			int a_in = a->get_input().empty() ? -1 : (int)a->get_total_in();
			int b_in = b->get_input().empty() ? -1 : (int)b->get_total_in();
			cmp = a_in - b_in;
			break;
		}

		case factorylist::by_transit:
		{
			int a_transit = a->get_input().empty() ? -1 : (int)a->get_total_transit();
			int b_transit = b->get_input().empty() ? -1 : (int)b->get_total_transit();
			cmp = a_transit - b_transit;
			break;
		}

		case factorylist::by_available:
		{
			int a_in = a->get_input().empty() ? -1 : (int)(a->get_total_in()+a->get_total_transit());
			int b_in = b->get_input().empty() ? -1 : (int)(b->get_total_in()+b->get_total_transit());
			cmp = a_in - b_in;
			break;
		}

		case factorylist::by_output:
		{
			int a_out = a->get_output().empty() ? -1 : (int)a->get_total_out();
			int b_out = b->get_output().empty() ? -1 : (int)b->get_total_out();
			cmp = a_out - b_out;
			break;
		}

		case factorylist::by_maxprod:
			cmp = a->get_base_production()*a->get_prodfactor() - b->get_base_production()*b->get_prodfactor();
			break;

		case factorylist::by_status:
			cmp = a->get_status() - b->get_status();
			break;

		case factorylist::by_power:
			cmp = a->get_prodfactor_electric() - b->get_prodfactor_electric();
			break;
	}
	if (cmp == 0) {
		cmp = STRICMP(a->get_name(), b->get_name());
	}
	return reverse ? cmp > 0 : cmp < 0;
}
