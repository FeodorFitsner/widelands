/*
 * Copyright (C) 2002, 2006-2011 by the Widelands Development Team
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

#include "fileview.h"

#include "constants.h"
#include "io/filesystem/filesystem.h"
#include "i18n.h"
#include "profile/profile.h"


Fullscreen_Menu_TextView::Fullscreen_Menu_TextView
	(std::string const & filename)
	:
	Fullscreen_Menu_Base("fileviewmenu.jpg"),

	title (this, get_w() * 3 / 50, get_h() / 10),

	textview
		(this,
		 get_w() *   3 /   80, get_h() * 283 / 1000,
		 get_w() * 919 / 1000, get_h() *  11 /   20),

	close_button
		(this, "close",
		 get_w() * 3 / 8, get_h() * 9 / 10, get_w() / 4, get_h() * 9 / 200,
		 g_gr->imgcache().get("pics/but0.png", true),
		 _("Close"), std::string(), true, false)
{
	close_button.sigclicked.connect(boost::bind(&Fullscreen_Menu_TextView::end_modal, boost::ref(*this), 0));

	close_button.set_font(font_small());

	Profile prof(filename.c_str(), "global", "texts"); //  section-less file
	Section & section = prof.get_safe_section("global");

	title   .set_text(section.get_safe_string("title"));
	textview.set_text(section.get_safe_string("text"));

	title.set_font(ui_fn(), fs_big(), UI_FONT_CLR_FG);
	title.set_pos
		(Point((get_inner_w() - title.get_w()) / 2, get_h() * 167 / 1000));

	textview.set_font(PROSA_FONT, PROSA_FONT_CLR_FG);
}

void Fullscreen_Menu_TextView::set_text(const std::string & text)
{
	textview.set_text(text);
}

Fullscreen_Menu_FileView::Fullscreen_Menu_FileView(const std::string & filename)
: Fullscreen_Menu_TextView(filename)
{}


struct FileViewWindow : public UI::UniqueWindow {
	FileViewWindow
		(UI::Panel                  & parent,
		 UI::UniqueWindow::Registry & reg,
		 std::string          const & filename);
private:
	UI::Multiline_Textarea textview;
};

FileViewWindow::FileViewWindow
	(UI::Panel                  & parent,
	 UI::UniqueWindow::Registry & reg,
	 std::string          const & filename)
	:
	UI::UniqueWindow(&parent, "file_view", &reg, 0, 0, ""),
	textview(this, 0, 0, 560, 240)
{
	Profile prof(filename.c_str(), "global", "texts"); // section-less file
	Section & s = prof.get_safe_section("global");

	set_title(s.get_safe_string("title"));

	textview.set_text(s.get_safe_string("text"));
	textview.set_font(PROSA_FONT, PROSA_FONT_CLR_FG);

	set_inner_size(560, 240);

	if (get_usedefaultpos())
		center_to_parent();
}


/**
 * Display the contents of a text file in a scrollable window.
*/
void fileview_window
	(UI::Panel                  & parent,
	 UI::UniqueWindow::Registry & reg,
	 std::string          const & filename)
{
	new FileViewWindow(parent, reg, filename);
}
