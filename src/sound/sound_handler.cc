/*
 * Copyright (C) 2005-2009 by the Widelands Development Team
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

#include "sound_handler.h"

#include "io/fileread.h"
#include "logic/game.h"
#include "i18n.h"
#include "wui/interactive_base.h"
#include "io/filesystem/layered_filesystem.h"
#include "logic/map.h"
#include "wui/mapview.h"
#include "wui/mapviewpixelfunctions.h"
#include "profile/profile.h"
#include "songset.h"

#include "log.h"

#include <SDL.h>

#include <cerrno>

#ifdef _WIN32
#include <windows.h>
#endif

#define DEFAULT_MUSIC_VOLUME  64
#define DEFAULT_FX_VOLUME    128

/** The global \ref Sound_Handler object
 * The sound handler is a static object because otherwise it'd be quite
 * difficult to pass the --nosound command line option
 */
Sound_Handler g_sound_handler;

/** This is just a basic constructor. The \ref Sound_Handler must already exist
 * during command line parsing because --nosound needs to be known. At this
 * time, however, all other information is still unknown, so a real
 * initialization cannot take place.
 * \sa Sound_Handler::init()
*/
Sound_Handler::Sound_Handler():
m_nosound             (false),
m_lock_audio_disabling(false),
m_disable_music       (false),
m_disable_fx          (false),
m_random_order        (true),
m_current_songset     ("")
{}

/// Housekeeping: unset hooks. Audio data will be freed automagically by the
/// \ref Songset and \ref FXset destructors, but not the {song|fx}sets
/// themselves.
Sound_Handler::~Sound_Handler()
{
	container_iterate_const  (FXset_map, m_fxs,   i) delete i.current->second;
	container_iterate_const(Songset_map, m_songs, i) delete i.current->second;
}

/** The real initialization for Sound_Handler.
 *
 * \pre The locale must be known before calling this
 *
 * \see Sound_Handler::Sound_Handler()
*/
void Sound_Handler::init()
{
	read_config();
	m_rng.seed(SDL_GetTicks());
	//this RNG will still be somewhat predictable, but it's just to avoid
	//identical playback patterns

	//Windows Music has crickling inside if the buffer has another size
	//than 4k, but other systems work fine with less, some crash
	//with big buffers.
#ifdef WIN32
	const uint16_t bufsize = 4096;
#else
	const uint16_t bufsize = 1024;
#endif

	if
		(SDL_InitSubSystem(SDL_INIT_AUDIO) == -1
		 or
		 Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, bufsize)
		 ==
		 -1)
	{
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		log("WARNING: Failed to initialize sound system: %s\n", Mix_GetError());

		set_disable_music(true);
		set_disable_fx(true);
		m_lock_audio_disabling = true;
		return;
	} else {
		Mix_HookMusicFinished(Sound_Handler::music_finished_callback);
		Mix_ChannelFinished(Sound_Handler::fx_finished_callback);
		load_system_sounds();
		Mix_VolumeMusic(m_music_volume); //  can not do this before InitSubSystem
	}
}

void Sound_Handler::shutdown()
{
	Mix_ChannelFinished(0);
	Mix_HookMusicFinished(0);

	Mix_CloseAudio();
}

/** Read the main config file, load background music and systemwide sound fx
 *
 * \pre The locale must be known before calling this
 */
void Sound_Handler::read_config()
{
	Section & s = g_options.pull_section("global");

	if (m_nosound) {
		set_disable_music(true);
		set_disable_fx(true);
	} else {
		set_disable_music(s.get_bool("disable_music",      false));
		set_disable_fx   (s.get_bool("disable_fx",         false));
		m_music_volume =  s.get_int ("music_volume",       DEFAULT_MUSIC_VOLUME);
		m_fx_volume    =  s.get_int ("fx_volume",          DEFAULT_FX_VOLUME);
	}

	m_random_order    =  s.get_bool("sound_random_order", true);

	register_song("music", "intro");
	register_song("music", "menu");
	register_song("music", "ingame");
}

/** Load systemwide sound fx into memory.
 * \note This loads only systemwide fx. Worker/building fx will be loaded by
 * their respective conf-file parsers
*/
void Sound_Handler::load_system_sounds()
{
	load_fx("sound", "click");
	load_fx("sound", "create_construction_site");
	load_fx("sound", "message");
	load_fx("sound/spoken", "under_attack");
	load_fx("sound/spoken", "site_lost");
	load_fx("sound/spoken", "site_defeated");
}

/** Load a sound effect. One sound effect can consist of several audio files
 * named EFFECT_XX.ogg, where XX is between 00 and 99. If
 * BASENAME_XX (without extension) is a directory, effects will be loaded
 * recursively.
 *
 * Subdirectories of and files under BASENAME_XX can be named anything you want.
 *
 * If you want "internationalized" sound effects (e.g. the lumberjack calling
 * "Timber") then append the locale to any file/directory name like this:
 * lumberjack_timber_00.ogg.de_DE
 *
 * \param dir        The directory where the audio files reside
 * \param basename   Name from which filenames will be formed
 *                   (BASENAME_XX.ogg);
 *                   also the name used with \ref play_fx
 * \internal
 * \param recursive  Whether to recurse into subdirectories
*/
void Sound_Handler::load_fx
	(std::string const & dir, std::string const & fxname, bool const recursive)
{
	filenameset_t dirs, files;
	filenameset_t::const_iterator i;

	assert(g_fs);

	g_fs->FindFiles(dir, fxname + "_??.ogg." + i18n::get_locale(), &files);
	if (files.empty())
		g_fs->FindFiles(dir, fxname + "_??.ogg", &files);

	for (i = files.begin(); i != files.end(); ++i) {
		assert(!g_fs->IsDirectory(*i));
		load_one_fx(i->c_str(), fxname);
	}

	if (recursive) {
		g_fs->FindFiles(dir, "*_??." + i18n::get_locale(), &dirs);
		if (dirs.empty())
			g_fs->FindFiles(dir, "*_??", &dirs);

		for (i = dirs.begin(); i != dirs.end(); ++i) {
			assert(g_fs->IsDirectory(*i));
			load_fx(*i, fxname, true);
		}
	}
}

/** Add exactly one file to the given fxset.
 * \param filename  the effect to be loaded
 * \param fx_name   the fxset to add the file to
 * The file format must be ogg. Otherwise this call will complain and
 * not load the file.
 * \note The complete audio file will be loaded into memory and stays there
 * until the game is finished.
*/
void Sound_Handler::load_one_fx
	(char const * const filename, std::string const & fx_name)
{
	FileRead fr;
	if (not fr.TryOpen(*g_fs, filename)) {
		log("WARNING: Could not open %s for reading!\n", filename);
		return;
	}

	if
		(Mix_Chunk * const m =
		 Mix_LoadWAV_RW(SDL_RWFromMem(fr.Data(fr.GetSize(), 0), fr.GetSize()), 1))
	{
		//make sure that requested FXset exists

		if (m_fxs.count(fx_name) == 0)
			m_fxs[fx_name] = new FXset();

		m_fxs[fx_name]->add_fx(m);
	} else
		log
			("Sound_Handler: loading sound effect \"%s\" for FXset \"%s\" "
			 "failed: %s\n",
			 filename, fx_name.c_str(), strerror(errno));
}

/** Calculate  the position of an effect in relation to the visible part of the
 * screen.
 * \param position  where the event happened (map coordinates)
 * \return position in widelands' game window: left=0, right=254, not in
 * viewport = -1
 * \note This function can also be used to check whether a logical coordinate is
 * visible at all
*/
int32_t Sound_Handler::stereo_position(Widelands::Coords const position)
{
	//screen x, y (without clipping applied, might well be invisible)
	int32_t sx, sy;
	//x, y resolutions of game window
	Widelands::FCoords fposition;

	assert(m_the_game);
	assert(position);

	Interactive_Base const & ibase = *m_the_game->get_ibase();
	Point const vp = ibase.get_viewpoint();

	int32_t const xres = ibase.get_xres();
	int32_t const yres = ibase.get_yres();

	MapviewPixelFunctions::get_pix(m_the_game->map(), position, sx, sy);
	sx -= vp.x;
	sy -= vp.y;

	//make sure position is inside viewport

	if (sx >= 0 && sx <= xres && sy >= 0 && sy <= yres)
		return sx * 254 / xres;

	return -1;
}

/** Find out whether to actually play a certain effect right now or rather not
 * (to avoid "sonic overload").
 * \todo What is the selection algorithm? cf class documentation
*/
bool Sound_Handler::play_or_not
	(std::string const &       fx_name,
	 int32_t             const stereo_pos,
	 uint8_t             const priority)
{
	bool allow_multiple = false; //  convenience for easier code reading
	float evaluation; //temporary to calculate single influences
	float probability; //weighted total of all influences

	//probability that this fx gets played; initially set according to priority

	//  float division! not integer
	probability = (priority % PRIO_ALLOW_MULTIPLE) / 128.0;

	//TODO: what to do with fx that happen offscreen?
	//TODO: reduce volume? reduce priority? other?
	if (stereo_pos == -1) {
		return false;
	}

	//TODO: check for a free channel

	if (priority == PRIO_ALWAYS_PLAY) {
		//TODO: if there is no free channel, kill a running fx and complain
		return true;
	}

	if (priority >= PRIO_ALLOW_MULTIPLE)
		allow_multiple = true;

	//find out if an fx called fx_name is already running
	bool already_running = false;
	const std::map<uint32_t, std::string>::const_iterator active_fx_end =
		m_active_fx.end();
	for
		(std::map<uint32_t, std::string>::const_iterator it =
		 m_active_fx.begin();
		 it != active_fx_end;
		 ++it)
	{
		// Crashed here! it is not valid and m_active_fx is empty. 
		// Probably threading problem with erase() in handle_channel_finished()
		if (it->second == fx_name) {
			already_running = true;
			break;
		}
	}

	if (!allow_multiple && already_running)
		return false;

	//TODO: long time since any play increases weighted_priority
	//TODO: high general frequency reduces weighted priority
	//TODO: deal with "coupled" effects like throw_net and retrieve_net

	uint32_t const ticks_since_last_play =
		SDL_GetTicks() - m_fxs[fx_name]->m_last_used;

	//  reward an fx for being silent
	if (ticks_since_last_play > SLIDING_WINDOW_SIZE) {
		evaluation = 1; //  arbitrary value; 0 -> no change, 1 -> probability = 1

		//  "decrease improbability"
		probability = 1 - ((1 - probability) * (1 - evaluation));
	} else { //penalize an fx for playing in short succession
		evaluation =
			static_cast<float>(ticks_since_last_play) / SLIDING_WINDOW_SIZE;
		probability *= evaluation; //  decrease probability
	}

	//printf("XXXXX %s ticks: %i ev: %f prob: %f\n",
	//       fx_name.c_str(), ticks_since_last_play, evaluation, probability);

	//finally: the decision
	//  float division! not integer
	return (m_rng.rand() % 255) / 255.0 <= probability;
}

/** Play (one of multiple) sound effect(s) with the given name. The effect(s)
 * must have been loaded before with \ref load_fx.
 * \param fx_name  The identifying name of the sound effect, see \ref load_fx .
 * \param map_position  Map coordinates where the event takes place
 * \param priority      How important is it that this FX actually gets played?
 *         (see \ref FXset::m_priority)
*/
void Sound_Handler::play_fx
	(std::string const &       fx_name,
	 Widelands::Coords   const map_position,
	 uint8_t             const priority)
{
	play_fx(fx_name, stereo_position(map_position), priority);
}

/** \overload
 * \param fx_name  The identifying name of the sound effect, see \ref load_fx .
 * \param stereo_position  position in widelands' game window, see
 *                         \ref stereo_position
 * \param priority         How important is it that this FX actually gets
 *                         played? (see \ref FXset::m_priority)
*/
void Sound_Handler::play_fx
	(std::string const &       fx_name,
	 int32_t             const stereo_pos,
	 uint8_t             const priority)
{
	assert(stereo_pos >= -1);
	assert(stereo_pos <= 254);

	if (get_disable_fx())
		return;

	if (m_fxs.count(fx_name) == 0) {
		log
			("Sound_Handler: sound effect \"%s\" does not exist!\n",
			 fx_name.c_str());
		return;
	}

	//see if the FX should be played
	if (!play_or_not(fx_name, stereo_pos, priority))
		return;

	//  retrieve the fx and play it if it's valid
	if (Mix_Chunk * const m = m_fxs[fx_name]->get_fx()) {
		const int32_t chan = Mix_PlayChannel(-1, m, 0);
		if (chan == -1)
			log("Sound_Handler: Mix_PlayChannel failed\n");
		else {
			Mix_SetPanning(chan, 254 - stereo_pos, stereo_pos);
			Mix_Volume(chan, get_fx_volume());
			m_active_fx[chan] = fx_name;
		}
	} else
		log
			("Sound_Handler: sound effect \"%s\" exists but contains no files!\n",
			 fx_name.c_str());
}

/** Load a background song. One "song" can consist of several audio files named
 * FILE_XX.ogg, where XX is between 00 and 99.
 * \param dir        The directory where the audio files reside.
 * \param basename   Name from which filenames will be formed
 *                   (BASENAME_XX.ogg); also the name used with \ref play_fx .
 * \param recursive  \internal Used for traversing subdirectories
 * This just registers the song, actual loading takes place when
 * \ref Songset::get_song() is called, i.e. when the song is about to be
 * played. The song will automatically be removed from memory when it has
 * finished playing.\n
 * Supported file formats are ogg. If BASENAME_XX
 * (with any extension) is a directory, effects will be registered recursively.
 * Subdirectories of and files under BASENAME_XX can be named anything you want.
*/
void Sound_Handler::register_song
	(std::string const & dir, std::string const & basename,
	 bool const recursive)
{
	filenameset_t files;
	filenameset_t::const_iterator i;

	assert(g_fs);

	if (recursive)
		g_fs->FindFiles(dir, "*", &files);
	else
		g_fs->FindFiles(dir, basename + "_??*", &files);

	for (i = files.begin(); i != files.end(); ++i) {
		if (g_fs->IsDirectory(*i)) {
			register_song(*i, basename, true);
		} else {
			if (m_songs.count(basename) == 0)
				m_songs[basename] = new Songset();

			m_songs[basename]->add_song(*i);
		}
	}
}

/** Start playing a songset.
 * \param songset_name  The songset to play a song from.
 * \param fadein_ms     Song will fade from 0% to 100% during fadein_ms
 *                      milliseconds starting from now.
 * \note When calling start_music() while music is still fading out from
 * \ref stop_music()
 * or \ref change_music() this function will block until the fadeout is complete
*/
void Sound_Handler::start_music
	(std::string const & songset_name, int32_t fadein_ms)
{
	if (get_disable_music())
		return;

	if (fadein_ms == 0) fadein_ms = 50; //  avoid clicks

	if (Mix_PlayingMusic())
		change_music(songset_name, 0, fadein_ms);

	if (m_songs.count(songset_name) == 0)
		log
			("Sound_Handler: songset \"%s\" does not exist!\n",
			 songset_name.c_str());
	else {
		if (Mix_Music * const m = m_songs[songset_name]->get_song()) {
			Mix_FadeInMusic(m, 1, fadein_ms);
			m_current_songset = songset_name;
		} else
			log
				("Sound_Handler: songset \"%s\" exists but contains no files!\n",
				 songset_name.c_str());
	}
	Mix_VolumeMusic(m_music_volume);
}

/** Stop playing a songset.
 * \param fadeout_ms Song will fade from 100% to 0% during fadeout_ms
 *                   milliseconds starting from now.
*/
void Sound_Handler::stop_music(int32_t fadeout_ms)
{
	if (get_disable_music())
		return;

	if (fadeout_ms == 0) fadeout_ms = 50; //  avoid clicks

	Mix_FadeOutMusic(fadeout_ms);
}

/** Play an other piece of music.
 * This is a member function provided for convenience. It is a wrapper around
 * \ref start_music and \ref stop_music.
 * \param fadeout_ms  Old song will fade from 100% to 0% during fadeout_ms
 *                    milliseconds starting from now.
 * \param fadein_ms   New song will fade from 0% to 100% during fadein_ms
 *                    milliseconds starting from now.
 * If songset_name is empty, another song from the currently active songset will
 * be selected
*/
void Sound_Handler::change_music
	(std::string const & songset_name,
	 int32_t const fadeout_ms, int32_t const fadein_ms)
{
	std::string s = songset_name;

	if (s == "")
		s = m_current_songset;
	else
		m_current_songset = s;

	if (Mix_PlayingMusic())
		stop_music(fadeout_ms);
	else
		start_music(s, fadein_ms);
}


bool Sound_Handler::get_disable_music() const throw () {return m_disable_music;}
bool Sound_Handler::get_disable_fx   () const throw () {return m_disable_fx;}
int32_t  Sound_Handler::get_music_volume() const throw ()
{
	return m_music_volume;
}
int32_t  Sound_Handler::get_fx_volume() const throw ()
{
	return m_fx_volume;
}


/** Normal set_* function, but the music must be started/stopped accordingly
 * Also, the new value is written back to the config file right away. It might
 * get lost otherwise.
 */
void Sound_Handler::set_disable_music(bool const disable)
{
	if (m_lock_audio_disabling || m_disable_music == disable)
		return;

	if (disable) {
		stop_music();
		m_disable_music = true;
	} else {
		m_disable_music = false;
		start_music(m_current_songset);
	}

	g_options.pull_section("global").set_bool("disable_music", disable);
}

/** Normal set_* function
 * Also, the new value is written back to the config file right away. It might
 * get lost otherwise.
*/
void Sound_Handler::set_disable_fx(bool const disable)
{
	if (m_lock_audio_disabling)
		return;

	m_disable_fx = disable;

	g_options.pull_section("global").set_bool("disable_fx", disable);
}

/**
 * Normal set_* function.
 * Set the music volume between 0 (muted) and \ref get_max_volume().
 * The new value is written back to the config file.
 *
 * \param volume The new music volume.
 */
void Sound_Handler::set_music_volume(int32_t volume) {
	if (not m_lock_audio_disabling) {
		m_music_volume = volume;
		Mix_VolumeMusic(volume);
		g_options.pull_section("global").set_int("music_volume", volume);
	}
}

/**
 * Normal set_* function
 * Set the FX sound volume between 0 (muted) and \ref get_max_volume().
 * The new value is written back to the config file.
 *
 * \param volume The new music volume.
 */
void Sound_Handler::set_fx_volume(int32_t volume) {
	if (not m_lock_audio_disabling) {
		m_fx_volume = volume;
		Mix_Volume(-1, volume);
		g_options.pull_section("global").set_int("fx_volume", volume);
	}
}

/** Callback to notify \ref Sound_Handler that a song has finished playing.
 * Usually, another song from the same songset will be started.\n
 * There is a special case for the intro screen's music: only one song will be
 * played. If the user has not clicked the mouse or pressed escape when the song
 * finishes, Widelands will automatically go on to the main menu.
*/
void Sound_Handler::music_finished_callback()
{
	//DO NOT CALL SDL_mixer FUNCTIONS OR SDL_LockAudio FROM HERE !!!

	SDL_Event event;
	if (g_sound_handler.m_current_songset == "intro") {
		//special case for splashscreen: there, only one song is ever played
		event.type           = SDL_KEYDOWN;
		event.key.state      = SDL_PRESSED;
		event.key.keysym.sym = SDLK_ESCAPE;
	} else {
		//else just play the next song - see general description for
		//further explanation
		event.type           = SDL_USEREVENT;
		event.user.code      = CHANGE_MUSIC;
	}
	SDL_PushEvent(&event);
}

/** Callback to notify \ref Sound_Handler that a sound effect has finished
 * playing.
*/
void Sound_Handler::fx_finished_callback(int32_t const channel)
{
	//DO NOT CALL SDL_mixer FUNCTIONS OR SDL_LockAudio FROM HERE !!!

	assert(0 <= channel);
	g_sound_handler.handle_channel_finished(static_cast<uint32_t>(channel));
}

/** Remove a finished sound fx from the list of currently playing ones
 * This is part of \ref fx_finished_callback
 */
void Sound_Handler::handle_channel_finished(uint32_t channel)
{
	// TODO: Needs locking
	m_active_fx.erase(channel);
}
