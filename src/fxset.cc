/*
 * Copyright (C) 2006 by the Widelands Development Team
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "fxset.h"
#include "sound_handler.h"

/** Create an FXset and set it's \ref m_priority
 * \param[in] prio	The desired priority (optional)
 */
FXset::FXset(Uint8 prio)
{
	m_priority = prio;
	m_last_used = 0;
}

/// Delete all fxs to avoid memory leaks. This also frees the audio data.
FXset::~FXset()
{
	std::vector < Mix_Chunk * >::iterator i = m_fxs.begin();

	while (i != m_fxs.end()) {
		Mix_FreeChunk(*i);
		i++;
	}

	m_fxs.clear();
}

/** Append a sound effect to the end of the fxset
 * \param[in] fx	The sound fx to append
 * \param[in] prio	Set previous \ref m_priority to new value (optional)
 */
void FXset::add_fx(Mix_Chunk * fx, Uint8 prio)
{
	assert(fx);

	m_priority = prio;
	m_fxs.push_back(fx);
}

/** Get a sound effect from the fxset. \e Which variant of the fx is actually
 * given out is determined at random
 * \return	a pointer to the chosen effect; NULL if sound effects are
 * disabled or no fx is registered
 */
Mix_Chunk *FXset::get_fx()
{
	int fxnumber;

	if (g_sound_handler.get_disable_fx() || m_fxs.empty())
		return NULL;

	fxnumber = g_sound_handler.m_rng.rand() % m_fxs.size();

	m_last_used = SDL_GetTicks();

	return m_fxs.at(fxnumber);
}
