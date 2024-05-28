/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <time.h>

#include "simwin.h"
#include "chat_frame.h"

#include "../tool/simmenu.h"
#include "../sys/simsys.h"

#include "../dataobj/translator.h"
#include "../dataobj/environment.h"
#include "../dataobj/gameinfo.h"
#include "../network/network_cmd_ingame.h"
#include "../player/simplay.h"
#include "../display/viewport.h"
#include "../gui/messagebox.h"


#include "components/gui_textarea.h"

#define CH_PUBLIC  (0)
#define CH_COMPANY (1)
#define CH_WHISPER (2)

static char const* const tab_strings[MAX_CHAT_TABS] =
{
	"public_chat",
	"company_chat",
	"personal_chat"
};


class gui_chat_owner_t :
	public gui_aligned_container_t,
	public gui_scrolled_list_t::scrollitem_t,
	public action_listener_t
{
private:
	plainstring sender;
	button_t bt_whisper_to;
	cbuffer_t buf;

public:
	gui_chat_owner_t(const chat_message_t::chat_node* m)
	{
		sender = m->sender;
		set_table_layout(2, 1);
		set_margin(scr_size(D_H_SPACE, 0), scr_size(D_H_SPACE, 0));
		if (player_t* p = world()->get_player(m->player_nr)) {
			bt_whisper_to.init(button_t::arrowright, NULL);
			bt_whisper_to.set_focusable(false);
			bt_whisper_to.add_listener(this);
			add_component(&bt_whisper_to);
			buf.printf("%s <%s>", sender.c_str(), p->get_name());
			new_component<gui_label_t>(buf.get_str(), color_idx_to_rgb(p->get_player_color1() + env_t::gui_player_color_dark));
		}
		else {
			new_component_span<gui_label_t>(sender.c_str(), 2);
		}
		set_size(get_min_size());
	}

	char const* get_text() const OVERRIDE { return ""; }

	bool action_triggered(gui_action_creator_t* comp, value_t) {
		if (comp == &bt_whisper_to) {
			chat_frame_t* win = dynamic_cast<chat_frame_t*>(win_get_magic(magic_chatframe));
			win->activate_whisper_to(sender.c_str());
		}
		return true;
	}
};


class gui_chat_balloon_t :
	public gui_scrolled_list_t::scrollitem_t
	//	public action_listener_t
{
private:
	enum {
		no_tail = 0, // for system message
		tail_left = 1, // for others post
		tail_right = 2  // for own post
	};
	uint8 tail_dir;

	time_t msg_time;
	sint8 player_nr;
	PIXVAL bgcolor;
	cbuffer_t text;
	scr_coord_val width;

	gui_label_buf_t lb_time_diff, lb_date, lb_local_time;
	gui_fixedwidth_textarea_t message;
	button_t bt_pos;

	int old_min;

	void update_time_diff(time_t now)
	{
		lb_time_diff.init(SYSCOL_BUTTON_TEXT_DISABLED, gui_label_t::right);
		lb_time_diff.buf().append(" (");
		int diff = difftime(now, msg_time);
		if (diff > 31536000) {
			uint8 years = (uint8)(diff / 31536000);
			if (years == 1) {
				lb_time_diff.buf().append(translator::translate("1 year ago"));
			}
			else {
				lb_time_diff.buf().printf(translator::translate("%u years ago"), years);
			}
		}
		else if (diff > 172800) {
			// 2 days to 365 days
			uint16 days = (uint16)(diff / 86400);
			lb_time_diff.buf().printf(translator::translate("%u days ago"), days);
		}
		else if (diff > 7200) {
			// 2 hours to 48 hours
			uint8 hours = (uint8)(diff / 3600);
			lb_time_diff.buf().printf(translator::translate("%u hours ago"), hours);
		}
		else if (diff > 60) {
			uint8 minutes = (uint8)(diff / 60);
			if (minutes == 1) {
				lb_time_diff.buf().append(translator::translate(" 1 minute ago"));
			}
			else {
				lb_time_diff.buf().printf(translator::translate("%u minutes ago"), minutes);
			}
			lb_time_diff.set_color(SYSCOL_TEXT);
		}
		else {
			lb_time_diff.buf().append(translator::translate("just now"));
			lb_time_diff.set_color(SYSCOL_TEXT);
		}
		lb_time_diff.buf().append(") ");
		lb_time_diff.update();
		lb_time_diff.set_size(lb_time_diff.get_min_size());
	}


public:
	gui_chat_balloon_t(const chat_message_t::chat_node* m, scr_coord_val w) :
		text(m->msg),
		width(w),
		message(&text, 0)
	{
		msg_time = m->local_time;
		player_nr = m->player_nr;
		tail_dir = m->sender == "" ? no_tail
			: (strcmp(m->sender, env_t::nickname.c_str()) == 0) ? tail_right : tail_left;
		old_min = -1;

		bt_pos.set_typ(button_t::posbutton_automatic);
		bt_pos.set_targetpos(m->pos);
		bt_pos.set_visible(m->pos != koord::invalid);

		const bool is_dark_theme = (env_t::gui_player_color_dark >= env_t::gui_player_color_bright);
		const int base_blend_percent = tail_dir == tail_right ? 60 : 80;
		player_t* player = world()->get_player(player_nr);
		const PIXVAL base_color = color_idx_to_rgb(player ? player->get_player_color1() + env_t::gui_player_color_bright : COL_GREY4);
		bgcolor = display_blend_colors(base_color, color_idx_to_rgb(COL_WHITE), is_dark_theme ? (95 - base_blend_percent) : base_blend_percent);
		if (msg_time) { // old save messages does not have date
			update_time_diff(time(NULL));
			char date[18];
			// add the time too
			struct tm* tm_event = localtime(&msg_time);
			if (tm_event) {
				strftime(date, 18, "%m-%d %H:%M", tm_event);
			}
			lb_local_time.buf().append(date);
			lb_local_time.update();
			lb_local_time.set_size(lb_local_time.get_min_size());
		}
		else {
			lb_time_diff.set_size(scr_size(1, 0));
			lb_local_time.set_size(scr_size(1, 0));
		}
		lb_date.buf().append(translator::get_short_date(m->time / 12, m->time % 12));
		lb_date.update();
		lb_date.set_size(lb_time_diff.get_min_size());
		const char* c = m->msg;
		while (c[0] > 0 && c[0] < 32) c++;
		text.clear();
		text.append(c);
		set_size(scr_size(width, 1));
	}

	void set_size(scr_size new_size) OVERRIDE
	{
		scr_coord_val labelwidth = max(lb_date.get_size().w + bt_pos.is_visible() * D_POS_BUTTON_WIDTH, lb_local_time.get_size().w);
		width = new_size.w;
		message.set_width(new_size.w - (D_MARGIN_LEFT + D_MARGIN_RIGHT + D_H_SPACE * 2 + LINESPACE / 2 + 4 + labelwidth));
		new_size.h = max(message.get_size().h + 4 + D_V_SPACE + lb_time_diff.get_size().h, lb_date.get_size().h + D_V_SPACE + lb_local_time.get_size().h);
		gui_component_t::set_size(new_size);
	}

	scr_size get_min_size() const OVERRIDE
	{
		return scr_size(100 + lb_time_diff.get_size().w, message.get_min_size().h + 4 + D_V_SPACE + lb_time_diff.get_size().h);
//		return scr_size(100 + lb_time_diff.get_size().w, size.h);
	}

	scr_size get_max_size() const OVERRIDE
	{
		return scr_size(scr_size::inf.w, message.get_size().h + 4 + D_V_SPACE + lb_time_diff.get_size().h);
	}

	char const* get_text() const { return ""; }

	void draw(scr_coord offset) OVERRIDE
	{
		offset += pos;
		offset.x += D_MARGIN_LEFT;

		if (msg_time) { // old save messages does not have date
			time_t now = time(NULL);
			tm* tm_event = localtime(&now);

			if (old_min != tm_event->tm_min) {
				old_min = tm_event->tm_min;
				update_time_diff(now);
			}
		}

		scr_coord_val labelwidth = max(lb_date.get_size().w + bt_pos.is_visible() * D_POS_BUTTON_WIDTH, lb_local_time.get_size().w);
		scr_size bsize = get_size() - scr_size(D_MARGIN_LEFT + D_MARGIN_RIGHT + D_H_SPACE * 2 + LINESPACE / 2 + labelwidth, D_V_SPACE);
		scr_coord_val off_w = D_H_SPACE;

		if (tail_dir == tail_left) {
			message.set_pos(scr_coord(off_w + 2, 2));
			bt_pos.set_pos(scr_coord(bsize.w + LINESPACE / 2, (lb_date.get_size().h - D_POS_BUTTON_HEIGHT) / 2));
			lb_date.set_pos(scr_coord(bsize.w + LINESPACE / 2 + bt_pos.is_visible() * D_POS_BUTTON_WIDTH, 0));
			lb_local_time.set_pos(scr_coord(bsize.w + LINESPACE / 2, lb_date.get_size().h));
		}
		else {
			off_w = labelwidth;
			bt_pos.set_pos(scr_coord(0, (lb_date.get_size().h - D_POS_BUTTON_HEIGHT) / 2));
			lb_date.set_pos(scr_coord(bt_pos.is_visible() * D_POS_BUTTON_WIDTH, 0));
			lb_local_time.set_pos(scr_coord(0, lb_date.get_size().h));
			message.set_pos(scr_coord(off_w + 2, 2));
		}
		lb_time_diff.set_pos(message.get_pos()+scr_size(bsize.w-D_MARGIN_RIGHT-lb_time_diff.get_size().w-2, message.get_size().h));

		// draw ballon
		display_filled_roundbox_clip(offset.x + off_w + 1, offset.y + 1, bsize.w, bsize.h, display_blend_colors(bgcolor, SYSCOL_SHADOW, 75), false);
		display_filled_roundbox_clip(offset.x + off_w, offset.y, bsize.w, bsize.h, bgcolor, false);
		if (tail_dir) {
			scr_coord_val h = LINESPACE / 2;
			for (scr_coord_val x = 0; x < h; x++) {
				if (tail_dir == tail_right) {
					display_vline_wh_clip_rgb(offset.x + off_w + bsize.w + x, offset.y + bsize.h - 10 - h + x, h - x, bgcolor, true);
				}
				else {
					display_vline_wh_clip_rgb(offset.x + off_w - x, offset.y + h, h - x, bgcolor, true);
				}
			}
			if (tail_dir == tail_right) {
				display_fillbox_wh_clip_rgb(offset.x + off_w + bsize.w, offset.y + bsize.h - 10, h, 1, display_blend_colors(bgcolor, SYSCOL_SHADOW, 75), true);
			}
		}
		scr_coord_val old_h = message.get_size().h;
		message.draw(offset);
		lb_local_time.draw(offset);
		lb_date.draw(offset);
		lb_time_diff.draw(offset);
		bt_pos.draw(offset);
		if (old_h != message.get_size().h) {
			gui_component_t::set_size(scr_size(get_size().w, lb_time_diff.get_size().h + message.get_size().h + 4 + D_V_SPACE));
		}
	}

	bool infowin_event(const event_t* ev)
	{
		bool swallowed = gui_scrolled_list_t::scrollitem_t::infowin_event(ev);
		if (!swallowed && IS_LEFTRELEASE(ev)) {
			event_t ev2 = *ev;
			ev2.move_origin(scr_coord(D_MARGIN_LEFT, 0));
			if (message.getroffen(ev2.click_pos)) {
				// add them to clipboard
				char msg_no_break[258];
				int j;
				for (j = 0; j < 256 && text[j]; j++) {
					msg_no_break[j] = text[j] == '\n' ? ' ' : text[j];
					if (msg_no_break[j] == 0) {
						msg_no_break[j++] = '\n';
						break;
					}
				}
				msg_no_break[j] = 0;
				// copy, if there was anything ...
				if (j) {
					dr_copy(msg_no_break, j);
				}
				create_win(new news_img("Copied."), w_time_delete, magic_none);
				swallowed = true;
			}
			else if (bt_pos.getroffen(ev2.click_pos)) {
				ev2.move_origin(bt_pos.get_pos());
				swallowed = bt_pos.infowin_event(&ev2);
			}
		}
		return swallowed;
	}
};



chat_frame_t::chat_frame_t() :
	gui_frame_t(translator::translate("Chat"))
{
	ibuf[0] = 0;
	ibuf_name[0] = 0;

	set_table_layout(1, 0);
	set_focusable(false);
	cont_tab_whisper.set_focusable(false);
	add_table(3, 1);
	{
		opaque_bt.set_focusable(false);
		opaque_bt.init(button_t::square_state, translator::translate("transparent background"));
		opaque_bt.add_listener(this);
		add_component(&opaque_bt);
		new_component<gui_fill_t>();
		lb_now_online.set_focusable(false);
		lb_now_online.set_align(gui_label_t::right);
		add_component(&lb_now_online);
	}
	end_table();

	for (int i = 0; i < MAX_CHAT_TABS; i++) {
		cont_chat_log[i].set_show_scroll_x(false);
		cont_chat_log[i].set_scroll_amount_y(LINESPACE * 3);
		cont_chat_log[i].set_maximize(true);
		// add tabs for classifying messages
		tabs.add_tab(cont_chat_log + i, translator::translate(tab_strings[i]));
	}

	tabs.add_listener(this);
	add_component(&tabs);

	add_table(4, 1);
	{
		bt_send_pos.init(button_t::posbutton | button_t::state, NULL);
		bt_send_pos.set_tooltip("Attach current coordinate to the comment");
		bt_send_pos.add_listener(this);
		add_component(&bt_send_pos);

		lb_channel.set_visible(tabs.get_active_tab_index() != CH_PUBLIC);
		lb_channel.set_rigid(false);
		add_component(&lb_channel);

		inp_destination.set_text(ibuf_name, lengthof(ibuf_name));
		inp_destination.set_visible(tabs.get_active_tab_index() == CH_WHISPER);
		inp_destination.set_notify_all_changes_delay(1000);
		inp_destination.add_listener(this);
		add_component(&inp_destination);

		input.set_text(ibuf, lengthof(ibuf));
		input.add_listener(this);
		add_component(&input);
	}
	end_table();

	set_resizemode(diagonal_resize);

	last_count = 0;
	reset_min_windowsize();
}


void chat_frame_t::fill_list()
{
	uint8 chat_mode = tabs.get_active_tab_index();

	player_t* current_player = world()->get_active_player();
	const scr_coord_val width = get_windowsize().w;

	gameinfo_t gi(world());

	lb_now_online.buf().printf(translator::translate("%u Client(s)\n"), (unsigned)gi.get_clients());
	lb_now_online.update();

	old_player_nr = current_player->get_player_nr();
	scr_coord_val old_scroll_y = cont_chat_log[chat_mode].get_scroll_y();
	scr_coord_val old_log_h = cont_chat_log[chat_mode].get_size().h;

	cont_chat_log[chat_mode].clear_elements();
	last_count = welt->get_chat_message()->get_list().get_count();

	plainstring prev_poster = "";
	sint8 prev_company = -1;
	bool player_locked = current_player->is_locked();
	for (chat_message_t::chat_node* const i : world()->get_chat_message()->get_list()) {

		// fillter
		switch (chat_mode) {
		case CH_COMPANY:
			if (i->channel_nr != current_player->get_player_nr()) {
				// other company's message
				continue;
			}
			if (player_locked && strcmp(env_t::nickname.c_str(), i->sender.c_str()) != 0) {
				// no permission but visible messages you sent
				continue;
			}

			break;
		case CH_WHISPER:
			if (i->destination == NULL || i->sender == NULL) {
				continue;
			}
			if (i->destination == "" || i->sender == "") {
				continue;
			}
			if (!strcmp(env_t::nickname.c_str(), i->destination.c_str()) && !strcmp(env_t::nickname.c_str(), i->sender.c_str())) {
				continue;
			}

			// direct chat mode
			if (ibuf_name[0] != 0 && (strcmp(ibuf_name, i->destination.c_str()) && strcmp(ibuf_name, i->sender.c_str()))) {
				continue;
			}

			if (strcmp(env_t::nickname.c_str(), i->destination.c_str())) {
				chat_history.append_unique(i->destination);
			}
			if (strcmp(env_t::nickname.c_str(), i->sender.c_str())) {
				chat_history.append_unique(i->sender);
			}

			break;
		default:
		case CH_PUBLIC:
			// system message and public chats
			if (i->channel_nr != -1) {
				continue; // other company's message
			}
			if (i->destination != NULL && i->destination != "") {
				continue; // direct message
			}
			break;
		}

		// Name display is omitted for unnamed system messages,
		//  self-messages, or consecutive messages by the same client from same company.
		const bool skip_name = i->sender == "" || (strcmp(i->sender, env_t::nickname.c_str()) == 0)
			|| (strcmp(i->sender.c_str(), prev_poster.c_str()) == 0 && (i->player_nr == prev_company));
		if (!skip_name) {
			// new chat owner element
			cont_chat_log[chat_mode].new_component<gui_chat_owner_t>(i);
		}
		cont_chat_log[chat_mode].new_component<gui_chat_balloon_t>(i, width);
		prev_poster = i->sender;
		prev_company = i->player_nr;
	}

	inp_destination.set_visible(tabs.get_active_tab_index() == CH_WHISPER);

	switch (chat_mode) {
	default:
	case CH_PUBLIC:
		// system message and public chats
		lb_channel.set_visible(false);
		env_t::chat_unread_public = 0;
		break;
	case CH_COMPANY:
		lb_channel.buf().append(current_player->get_name());
		lb_channel.set_color(color_idx_to_rgb(current_player->get_player_color1() + env_t::gui_player_color_dark));
		lb_channel.set_visible(true);
		lb_channel.update();
		env_t::chat_unread_company = 0;
		break;
	case CH_WHISPER:
		if (cb_direct_chat_targets.count_elements() != chat_history.get_count()) {
			cb_direct_chat_targets.clear_elements();
			for (uint32 i = 0; i < chat_history.get_count(); i++) {
				cb_direct_chat_targets.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(chat_history[i], SYSCOL_TEXT);
			}
		}
		cb_direct_chat_targets.set_visible(chat_history.get_count() && cb_direct_chat_targets.count_elements());
		env_t::chat_unread_whisper = 0;
		break;
	}
	cont_chat_log[chat_mode].show_bottom();
	cont_chat_log[chat_mode].set_maximize(true);
}



bool chat_frame_t::action_triggered(gui_action_creator_t* comp, value_t v)
{
	if (comp == &input && ibuf[0] != 0) {
		const sint8 channel = tabs.get_active_tab_index() == CH_COMPANY ? (sint8)world()->get_active_player_nr() : -1;
		const sint8 sender_company_nr = welt->get_active_player()->is_locked() ? -1 : welt->get_active_player()->get_player_nr();
		plainstring dest = tabs.get_active_tab_index() == CH_WHISPER ? ibuf_name : 0;

		if (dest != 0 && strcmp(dest.c_str(), env_t::nickname.c_str()) == 0) {
			return true; // message to myself
		}

		// Send chat message to server for distribution
		nwc_chat_t* nwchat = new nwc_chat_t(ibuf, sender_company_nr, (sint8)channel, env_t::nickname.c_str(), dest.c_str(), bt_send_pos.pressed ? world()->get_viewport()->get_world_position() : koord::invalid);
		network_send_server(nwchat);

		// FIXME?: Once the destination and sender clients are closed, those comments will not be left anywhere...
		// visible messages you sent
		if (tabs.get_active_tab_index() == CH_WHISPER) {
			world()->get_chat_message()->add_chat_message(ibuf, channel, sender_company_nr, env_t::nickname.c_str(), dest, bt_send_pos.pressed ? world()->get_viewport()->get_world_position() : koord::invalid);
		}

		ibuf[0] = 0;
	}
	else if (comp == &inp_destination && tabs.get_active_tab_index() == CH_WHISPER) {
		lb_whisper_target.buf().printf("To: %s", ibuf_name);
		lb_whisper_target.update();
		fill_list();
	}
	else if (comp == &opaque_bt) {
		if (!opaque_bt.pressed && env_t::chat_window_transparency != 100) {
			set_transparent(100 - env_t::chat_window_transparency, gui_theme_t::gui_color_chat_window_network_transparency);
			cont_chat_log[0].set_skin_type(gui_scrolled_list_t::transparent);
			cont_chat_log[1].set_skin_type(gui_scrolled_list_t::transparent);
			cont_chat_log[2].set_skin_type(gui_scrolled_list_t::transparent);
		}
		else {
			set_transparent(0, gui_theme_t::gui_color_chat_window_network_transparency);
			cont_chat_log[0].set_skin_type(gui_scrolled_list_t::windowskin);
			cont_chat_log[1].set_skin_type(gui_scrolled_list_t::windowskin);
			cont_chat_log[2].set_skin_type(gui_scrolled_list_t::windowskin);
		}
		opaque_bt.pressed ^= 1;
	}
	else if (comp == &bt_send_pos) {
		bt_send_pos.pressed ^= 1;
	}
	else if (comp == &cb_direct_chat_targets) {
		if (!cb_direct_chat_targets.count_elements()) return true;
		activate_whisper_to(cb_direct_chat_targets.get_selected_item()->get_text());
		fill_list();
	}
	else if (comp == &tabs) {
		fill_list();
		inp_destination.set_visible(tabs.get_active_tab_index() == CH_WHISPER);
		if (tabs.get_active_tab_index() == CH_WHISPER) {
			lb_channel.buf().append(translator::translate("direct_chat_to:"));
			lb_channel.set_color(SYSCOL_TEXT);
			lb_channel.set_visible(true);
			lb_channel.update();
		}
		resize(scr_size(0, 0));
	}
	return true;
}



void chat_frame_t::draw(scr_coord pos, scr_size size)
{
	if (welt->get_chat_message()->get_list().get_count() != last_count || old_player_nr != world()->get_active_player_nr()) {
		fill_list();
	}
	gui_frame_t::draw(pos, size);
}


void chat_frame_t::rdwr(loadsave_t* file)
{
	// window size
	scr_size size = get_windowsize();
	size.rdwr(file);

	tabs.rdwr(file);
	file->rdwr_str(ibuf, lengthof(ibuf));
}

void chat_frame_t::activate_whisper_to(const char* recipient)
{
	if (strcmp(recipient, env_t::nickname.c_str()) == 0) {
		return; // message to myself
	}
	sprintf(ibuf_name, "%s", recipient);
	lb_whisper_target.buf().printf("To: %s", recipient);
	lb_whisper_target.update();

	inp_destination.set_text(ibuf_name, lengthof(ibuf_name));
	inp_destination.set_size(scr_size(proportional_string_width(ibuf_name), D_EDIT_HEIGHT));

	tabs.set_active_tab_index(CH_WHISPER);
	inp_destination.set_visible(true);
	lb_channel.buf().append("direct_chat_to:");
	lb_channel.set_color(SYSCOL_TEXT);
	lb_channel.set_visible(true);
	lb_channel.update();
	//set_focus(&input); // this cause the crash
	//fill_list();
	resize(scr_size(0, 0));
}
