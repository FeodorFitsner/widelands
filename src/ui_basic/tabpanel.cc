/*
 * Copyright (C) 2003, 2006-2010 by the Widelands Development Team
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

#include "ui_basic/tabpanel.h"

#include "graphic/font_handler1.h"
#include "graphic/rendertarget.h"
#include "graphic/text_layout.h"
#include "ui_basic/mouse_constants.h"

namespace UI {

// Button height of tab buttons in pixels. Is also used for width with pictorial buttons.
constexpr int kTabPanelButtonHeight = 34;

// Margin around image. The image will be scaled down to fit into this rectangle with preserving size.
constexpr int kTabPanelImageMargin = 2;

// Left and right margin around text.
constexpr int kTabPanelTextMargin = 4;

//  height of the bar separating buttons and tab contents
constexpr int kTabPanelSeparatorHeight = 4;

// Constant to flag up when we're not at a tab.
constexpr uint32_t kNotFound = std::numeric_limits<uint32_t>::max();

/*
 * =================
 * class Tab
 * =================
 */
Tab::Tab
	(TabPanel* const tab_parent,
	 size_t const tab_id,
	 int32_t x,
	 int32_t w,
	 const std::string& name,
	 const std::string& _title,
	 const Image* _pic,
	 const std::string& tooltip_text,
	 Panel* const contents)
	:
	NamedPanel(tab_parent, name, x, 0, w, kTabPanelButtonHeight, tooltip_text),
	parent(tab_parent),
	id(tab_id),
	pic(_pic),
	title(_title),
	tooltip(tooltip_text),
	panel(contents)
{
}

/**
 * Currently active tab
 */
bool Tab::active() {
	return parent->active_ == id;
}
void Tab::activate() {
	return parent->activate(id);
}

bool Tab::handle_mousepress(uint8_t, int32_t, int32_t) {
	play_click();
	return false;
}

/*
 * =================
 * class TabPanel
 * =================
 */
/**
 * Initialize an empty TabPanel
*/
TabPanel::TabPanel
	(Panel * const parent,
	 int32_t const x, int32_t const y,
	 const Image* background,
	 TabPanel::Type border_type)
	:
	Panel           (parent, x, y, 0, 0),
	active_         (0),
	highlight_      (kNotFound),
	pic_background_ (background),
	border_type_    (border_type)
{}
TabPanel::TabPanel
	(Panel * const parent,
	 int32_t const x, int32_t const y, int32_t const w, int32_t const h,
	 const Image* background,
	 TabPanel::Type border_type)
	:
	Panel           (parent, x, y, w, h),
	active_         (0),
	highlight_      (kNotFound),
	pic_background_ (background),
	border_type_    (border_type)
{}

/**
 * Resize the visible tab based on our actual size.
 */
void TabPanel::layout()
{
	if (active_ < tabs_.size()) {
		Panel * const panel = tabs_[active_]->panel;
		uint32_t h = get_h();

		// avoid excessive craziness in case there is a wraparound
		h = std::min(h, h - (kTabPanelButtonHeight + kTabPanelSeparatorHeight));
		// If we have a border, we will also want some margin to the bottom
		if (border_type_ == TabPanel::Type::kBorder) {
			h = h - kTabPanelSeparatorHeight;
		}
		panel->set_size(get_w(), h);
	}
}

/**
 * Compute our desired size based on the currently selected tab.
 */
void TabPanel::update_desired_size()
{
	// size of button row
	int w = kTabPanelButtonHeight * tabs_.size();
	int h = kTabPanelButtonHeight + kTabPanelSeparatorHeight;

	// size of contents
	if (active_ < tabs_.size()) {
		Panel * const panel = tabs_[active_]->panel;
		int panelw, panelh;

		panel->get_desired_size(&panelw, &panelh);
		// TODO(unknown):  the panel might be bigger -> add a scrollbar in that case
		//panel->set_size(panelw, panelh);

		if (panelw > w)
			w = panelw;
		h += panelh;
	}

	set_desired_size(w, h);

	// This is not redundant, because even if all this doesn't change our
	// desired size, we were typically called because of a child window that
	// changed, and we need to relayout that.
	layout();
}

/**
 * Add a new textual tab
*/
uint32_t TabPanel::add
	(const std::string & name,
	 const std::string & title,
	 Panel             * const panel,
	 const std::string &       tooltip_text)
{
	const Image* pic = UI::g_fh1->render(as_uifont(title));
	return add_tab(std::max(kTabPanelButtonHeight, pic->width() + 2 * kTabPanelTextMargin),
						name,
						title,
						pic,
						tooltip_text,
						panel);
}

/**
 * Add a new pictorial tab
*/
uint32_t TabPanel::add
	(const std::string & name,
	 const Image* pic,
	 Panel             * const panel,
	 const std::string &       tooltip_text)
{
	return add_tab(kTabPanelButtonHeight,
						name,
						"",
						pic,
						tooltip_text,
						panel);
}

/** Common adding function for textual and pictorial tabs. */
uint32_t TabPanel::add_tab(int32_t width,
									const std::string& name,
									const std::string& title,
									const Image* pic,
									const std::string& tooltip_text,
									Panel* panel) {
	assert(panel);
	assert(panel->get_parent() == this);

	size_t id = tabs_.size();
	int32_t x = id > 0 ? tabs_[id - 1]->get_x() + tabs_[id - 1]->get_w() : 0;
	tabs_.push_back(new Tab(this, id, x, width, name, title, pic, tooltip_text, panel));

	// Add a margin if there is a border
	if (border_type_ == TabPanel::Type::kBorder) {
		panel->set_border(kTabPanelSeparatorHeight + 1, kTabPanelSeparatorHeight + 1,
								kTabPanelSeparatorHeight, kTabPanelSeparatorHeight);
		panel->set_pos(Point(0, kTabPanelButtonHeight));
	} else {
		panel->set_pos(Point(0, kTabPanelButtonHeight + kTabPanelSeparatorHeight));
	}

	panel->set_visible(id == active_);
	update_desired_size();

	return id;
}


/**
 * Make a different tab the currently active tab.
*/
void TabPanel::activate(uint32_t idx)
{
	if (active_ < tabs_.size())
		tabs_[active_]->panel->set_visible(false);
	if (idx < tabs_.size())
		tabs_[idx]->panel->set_visible(true);

	active_ = idx;

	update_desired_size();
}

void TabPanel::activate(const std::string & name)
{
	for (uint32_t t = 0; t < tabs_.size(); ++t)
		if (tabs_[t]->get_name() == name)
			activate(t);
}

/**
 * Return the tab names in order
 */
const TabPanel::TabList & TabPanel::tabs() {
	return tabs_;
}

/**
 * Draw the buttons and the tab
*/
void TabPanel::draw(RenderTarget & dst)
{
	// draw the background
	static_assert(2 < kTabPanelButtonHeight, "assert(2 < kTabPanelButtonSize) failed.");
	static_assert(4 < kTabPanelButtonHeight, "assert(4 < kTabPanelButtonSize) failed.");

	if (pic_background_) {
		if (!tabs_.empty()) {
			dst.tile
				(Rect(Point(0, 0), tabs_.back()->get_x() + tabs_.back()->get_w(), kTabPanelButtonHeight - 2),
				 pic_background_,
				 Point(get_x(), get_y()));
		}
		assert(kTabPanelButtonHeight - 2 <= get_h());
		dst.tile
			(Rect(Point(0, kTabPanelButtonHeight - 2), get_w(), get_h() - kTabPanelButtonHeight + 2),
			 pic_background_,
			 Point(get_x(), get_y() + kTabPanelButtonHeight - 2));
	}

	RGBColor black(0, 0, 0);

	// draw the buttons
	int32_t x = 0;
	int tab_width = 0;
	for (size_t idx = 0; idx < tabs_.size(); ++idx) {
		x = tabs_[idx]->get_x();
		tab_width = tabs_[idx]->get_w();

		if (highlight_ == idx) {
			dst.brighten_rect(Rect(Point(x, 0), tab_width, kTabPanelButtonHeight), MOUSE_OVER_BRIGHT_FACTOR);
		}

		assert(tabs_[idx]->pic);

		// If the title is empty, we will assume a pictorial tab
		if (tabs_[idx]->title.empty()) {
			// Scale the image down if needed, but keep the ratio.
			constexpr int kMaxImageSize = kTabPanelButtonHeight - 2 * kTabPanelImageMargin;
			double image_scale =
				std::min(1.,
							std::min(static_cast<double>(kMaxImageSize) / tabs_[idx]->pic->width(),
										static_cast<double>(kMaxImageSize) / tabs_[idx]->pic->height()));

			uint16_t picture_width = image_scale * tabs_[idx]->pic->width();
			uint16_t picture_height = image_scale * tabs_[idx]->pic->height();
			dst.blitrect_scale(Rect(x + (kTabPanelButtonHeight - picture_width) / 2,
											(kTabPanelButtonHeight - picture_height) / 2,
											picture_width,
											picture_height),
									 tabs_[idx]->pic,
									 Rect(0, 0, tabs_[idx]->pic->width(), tabs_[idx]->pic->height()),
									 1.,
									 BlendMode::UseAlpha);
		} else {
			dst.blit(Point(x + kTabPanelTextMargin, (kTabPanelButtonHeight - tabs_[idx]->pic->height()) / 2),
						tabs_[idx]->pic,
						BlendMode::UseAlpha,
						UI::Align::kLeft);
		}

		// Draw top part of border
		dst.brighten_rect
			(Rect(Point(x, 0), tab_width, 2), BUTTON_EDGE_BRIGHT_FACTOR);
		dst.brighten_rect
			(Rect(Point(x, 2), 2, kTabPanelButtonHeight - 4),
			 BUTTON_EDGE_BRIGHT_FACTOR);
		dst.fill_rect
			(Rect(Point(x + tab_width - 2, 2), 1, kTabPanelButtonHeight - 4),
			 black);
		dst.fill_rect
			(Rect(Point(x + tab_width - 1, 1), 1, kTabPanelButtonHeight - 3),
			 black);

		// Draw bottom part
		if (active_ != idx)
			dst.brighten_rect
				(Rect(Point(x, kTabPanelButtonHeight - 2), tab_width, 2),
				 2 * BUTTON_EDGE_BRIGHT_FACTOR);
		else {
			dst.brighten_rect
				(Rect(Point(x, kTabPanelButtonHeight - 2), 2, 2),
				 BUTTON_EDGE_BRIGHT_FACTOR);

			dst.brighten_rect
				(Rect(Point(x + tab_width - 2, kTabPanelButtonHeight - 2), 2, 2),
				 2 * BUTTON_EDGE_BRIGHT_FACTOR);
			dst.fill_rect
				(Rect(Point(x + tab_width - 2, kTabPanelButtonHeight - 1), 1, 1),
				 black);
			dst.fill_rect
				(Rect(Point(x + tab_width - 2, kTabPanelButtonHeight - 2), 2, 1),
				 black);
		}
	}

	// draw the remaining separator
	assert(x <= get_w());
	dst.brighten_rect
		(Rect(Point(x + tab_width, kTabPanelButtonHeight - 2), get_w() - x, 2),
		 2 * BUTTON_EDGE_BRIGHT_FACTOR);

	// Draw border around the main panel
	if (border_type_ == TabPanel::Type::kBorder) {
		//  left edge
		dst.brighten_rect
			(Rect(Point(0, kTabPanelButtonHeight), 2, get_h() - 2), BUTTON_EDGE_BRIGHT_FACTOR);
		//  bottom edge
		dst.fill_rect(Rect(Point(2, get_h() - 2), get_w() - 2, 1), black);
		dst.fill_rect(Rect(Point(1, get_h() - 1), get_w() - 1, 1), black);
		//  right edge
		dst.fill_rect(Rect(Point(get_w() - 2, kTabPanelButtonHeight - 1), 1, get_h() - 2), black);
		dst.fill_rect(Rect(Point(get_w() - 1, kTabPanelButtonHeight - 2), 1, get_h() - 1), black);
	}
}


/**
 * Cancel all highlights when the mouse leaves the panel
*/
void TabPanel::handle_mousein(bool inside)
{
	if (!inside && highlight_ != kNotFound) {
		highlight_ = kNotFound;
	}
}


/**
 * Update highlighting
*/
bool TabPanel::handle_mousemove
	(uint8_t, int32_t const x, int32_t const y, int32_t, int32_t)
{
	size_t hl = find_tab(x, y);

	if (hl != highlight_) {
		highlight_ = hl;
		set_tooltip(highlight_ != kNotFound ? tabs_[highlight_]->tooltip : "");
	}
	return true;
}


/**
 * Change the active tab if a tab button has been clicked
*/
bool TabPanel::handle_mousepress(const uint8_t btn, int32_t x, int32_t y) {
	if (btn == SDL_BUTTON_LEFT) {
		size_t id = find_tab(x, y);
		if (id != kNotFound) {
			activate(id);
			return true;
		}
	}
	return false;
}

bool TabPanel::handle_mouserelease(uint8_t, int32_t, int32_t)
{
	return false;
}


/**
 * Find the tab at the coordinates x, y
 * Returns kNotFound if no tab was found
 */
size_t TabPanel::find_tab(int32_t x, int32_t y) const {
	if (y < 0 || y >= kTabPanelButtonHeight) {
		return kNotFound;
	}

	int32_t width = 0;
	for (size_t id = 0; id < tabs_.size(); ++id) {
		width += tabs_[id]->get_w();
		if (width > x) {
			return id;
		}
	}
	return kNotFound;
}


}
