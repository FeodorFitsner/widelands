/*
 * Copyright (C) 2002-2004, 2006-2008, 2010 by the Widelands Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "wui/login_box.h"

#include "base/i18n.h"
#include "profile/profile.h"
#include "ui_basic/button.h"
#include "ui_basic/messagebox.h"

LoginBox::LoginBox(Panel & parent)
:
Window(&parent, "login_box", 0, 0, 500, 220, _("Metaserver login"))
{
	center_to_parent();

	int32_t margin = 10;

	ta_nickname = new UI::Textarea(this, margin, margin, _("Nickname:"));
	eb_nickname =
		new UI::EditBox
			(this, 150, margin, 330, 20,
			 ImageCatalog::Key::kButton2, UI::Align_Left);

	ta_password = new UI::Textarea(this, margin, 40, _("Password:"));
	eb_password =
		new UI::EditBox
			(this, 150, 40, 330, 20,
			 ImageCatalog::Key::kButton2, UI::Align_Left);

	pwd_warning =
		new UI::MultilineTextarea
			(this, margin, 65, 505, 50,
			 _("WARNING: Password will be shown and saved readable!"),
			 UI::Align_Left);

	cb_register = new UI::Checkbox(this, Point(margin, 110));
	ta_register =
		new UI::Textarea(this, 40, 110, _("Log in to a registered account"));

	cb_auto_log = new UI::Checkbox(this, Point(margin, 135));
	ta_auto_log = new UI::MultilineTextarea
		(this, 40, 135, get_inner_w() - cb_auto_log->get_w() - margin, 35,
		 _("Automatically use this login information from now on."));

	UI::Button * loginbtn = new UI::Button
		(this, "login",
		 (get_inner_w() / 2 - 200) / 2, get_inner_h() - 20 - margin,
		 200, 20,
		 ImageCatalog::Key::kButton0,
		 _("Login"));
	loginbtn->sigclicked.connect(boost::bind(&LoginBox::pressed_login, boost::ref(*this)));
	UI::Button * cancelbtn = new UI::Button
		(this, "cancel",
		 (get_inner_w() / 2 - 200) / 2 + get_inner_w() / 2, loginbtn->get_y(), 200, 20,
		 ImageCatalog::Key::kButton1,
		 _("Cancel"));
	cancelbtn->sigclicked.connect(boost::bind(&LoginBox::pressed_cancel, boost::ref(*this)));

	Section & s = g_options.pull_section("global");
	eb_nickname->set_text(s.get_string("nickname", _("nobody")));
	eb_password->set_text(s.get_string("password", ""));
	cb_register->set_state(s.get_bool("registered", false));
}


/// called, if "login" is pressed
void LoginBox::pressed_login()
{
	// Check if all needed input fields are valid
	if (eb_nickname->text().empty()) {
		UI::WLMessageBox mb
			(this, _("Empty Nickname"), _("Please enter a nickname!"),
			 UI::WLMessageBox::OK);
		mb.run();
		return;
	}
	if (eb_nickname->text().find(' ') <= eb_nickname->text().size()) {
		UI::WLMessageBox mb
			(this, _("Space in Nickname"),
			 _("Sorry, but spaces are not allowed in nicknames!"),
			 UI::WLMessageBox::OK);
		mb.run();
		return;
	}
	if (eb_password->text().empty() && cb_register->get_state()) {
		UI::WLMessageBox mb
			(this, _("Empty Password"), _("Please enter your password!"),
			 UI::WLMessageBox::OK);
		mb.run();
		return;
	}
	end_modal(1);
}


/// Called if "cancel" was pressed
void LoginBox::pressed_cancel()
{
	end_modal(0);
}
