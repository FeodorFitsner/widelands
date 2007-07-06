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

#include "wlapplication.h"

#include "build_id.h"
#include "editor.h"
#include "error.h"
#include "font_handler.h"
#include "fullscreen_menu_fileview.h"
#include "fullscreen_menu_intro.h"
#include "fullscreen_menu_inet_lobby.h"
#include "fullscreen_menu_inet_server_options.h"
#include "fullscreen_menu_main.h"
#include "fullscreen_menu_netsetup.h"
#include "fullscreen_menu_options.h"
#include "fullscreen_menu_singleplayer.h"
#include "fullscreen_menu_tutorial_select_map.h"
#include "game.h"
#include "game_server_connection.h"
#include "game_server_proto.h"
#include "graphic.h"
#include "i18n.h"
#include "journal.h"
#include "layered_filesystem.h"
#include "network.h"
#include "network_ggz.h"
#include "profile.h"
#include "sound/sound_handler.h"
#include "wexception.h"

#include <iostream>
#include <stdexcept>
#include <string>

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef DEBUG
#ifndef __WIN32__
int WLApplication::pid_me=0;
int WLApplication::pid_peer=0;
volatile int WLApplication::may_run=0;
#include <signal.h>
#endif // WIN32
#endif // DEBUG

int editor_commandline=0; // Enable the User to start the Editor directly.

//Always specifying namespaces is good, but let's not go too far ;-)
using std::cout;
using std::endl;

/**
 * Sets the filelocators default searchpaths (partly OS specific)
 * \todo Handle exception FileType_error
 * \todo Handle case when \e no data can be found
 */
void WLApplication::setup_searchpaths(std::string argv0)
{
	try {
#ifdef __APPLE__
		// on mac, the default Data Dir ist Relative to the current directory
		g_fs->AddFileSystem(FileSystem::Create("Widelands.app/Contents/Resources/"));
#else
		// first, try the data directory used in the last scons invocation
		g_fs->AddFileSystem(FileSystem::Create(INSTALL_DATADIR)); //see config.h
#endif
	}
	catch (FileNotFound_error e) {}
	catch (FileAccessDenied_error e) {
		log("Access denied on %s. Continuing.\n", e.m_filename.c_str());
	}
	catch (FileType_error e) {
		//TODO: handle me
	}

	try {
#ifndef __WIN32__
		// if that fails, search it where the FHS forces us to put it (obviously UNIX-only)
		g_fs->AddFileSystem(FileSystem::Create("/usr/share/games/widelands"));
#else
		//TODO: is there a "default dir" for this on win32 and mac ?
#endif
	}
	catch (FileNotFound_error e) {}
	catch (FileAccessDenied_error e) {
		log("Access denied on %s. Continuing.\n", e.m_filename.c_str());
	}
	catch (FileType_error e) {
		//TODO: handle me
	}

	try {
		// absolute fallback directory is the CWD
		g_fs->AddFileSystem(FileSystem::Create("."));
	}
	catch (FileNotFound_error e) {}
	catch (FileAccessDenied_error e) {
		log("Access denied on %s. Continuing.\n", e.m_filename.c_str());
	}
	catch (FileType_error e) {
		//TODO: handle me
	}

	//TODO: what if all the searching failed? Bail out!

	// the directory the executable is in is the default game data directory
	std::string::size_type slash = argv0.rfind('/');
	std::string::size_type backslash = argv0.rfind('\\');

	if (backslash != std::string::npos && (slash == std::string::npos || backslash > slash))
		slash = backslash;

	if (slash != std::string::npos) {
		argv0.erase(slash);
		if (argv0 != ".") {
			try {
				g_fs->AddFileSystem(FileSystem::Create(argv0));
#ifdef USE_DATAFILE
				argv0.append ("/widelands.dat");
				g_fs->AddFileSystem(new Datafile(argv0.c_str()));
#endif
			}
			catch (FileNotFound_error e) {}
			catch (FileAccessDenied_error e) {
				log("Access denied on %s. Continuing.\n", e.m_filename.c_str());
			}
			catch (FileType_error e) {
				//TODO: handle me
			}
		}
	}

	// finally, the user's config directory
	// TODO: implement this for Windows (yes, NT-based ones are actually multi-user)
#ifndef __WIN32__
	std::string path;
	char *buf=getenv("HOME"); //do not use GetHomedir() to not accidentally create ./.widelands

	if (buf) { // who knows, maybe the user's homeless
		path = std::string(buf) + "/.widelands";
		mkdir(path.c_str(), 0x1FF);
		try {
			g_fs->AddFileSystem(FileSystem::Create(path.c_str()));
		}
		catch (FileNotFound_error e) {}
		catch (FileAccessDenied_error e) {
			log("Access denied on %s. Continuing.\n", e.m_filename.c_str());
		}
		catch (FileType_error e) {
			//TODO: handle me
		}
	} else {
		//TODO: complain
	}
#endif
}

WLApplication *WLApplication::the_singleton=0;

/**
 * The main entry point for the WLApplication singleton.
 *
 * Regardless of circumstances, this will return the one and only valid
 * WLApplication object when called. If neccessary, a new WLApplication instance
 * is created.
 *
 * While you \e can do the first call to this method without parameters, it does
 * not make much sense.
 *
 * \param argc The number of command line arguments
 * \param argv Array of command line arguments
 * \return An (always valid!) pointer to the WLApplication singleton
 *
 * \todo Return a reference - the return value is always valid anyway
 */
WLApplication * const WLApplication::get(const int argc, const char **argv)
{
	if (the_singleton==0) {
		the_singleton=new WLApplication(argc, argv);
	}

	return the_singleton;
}

/**
 * Initialize an instance of WLApplication.
 *
 * This constructor is protected \e on \e purpose !
 * Use \ref WLApplication::get() instead and look at the class description.
 *
 * For easier access, we repackage argc/argv into an STL map here. If you specify
 * the same option more than once, only the last occurrence is effective.
 *
 * \param argc The number of command line arguments
 * \param argv Array of command line arguments
 */
WLApplication::WLApplication(const int argc, const char **argv):
		m_commandline(std::map<std::string, std::string>()),
		journal               (0),
		m_input_grab          (false),
		m_mouse_swapped       (false),
		m_mouse_speed         (0.0),
		m_mouse_position              (0,                     0),
		m_mouse_maxx          (0),     m_mouse_maxy          (0),
		m_mouse_locked        (0),
		m_mouse_internal_x    (0),     m_mouse_internal_y    (0),
		m_mouse_internal_comp_position(0,                     0),
		m_should_die          (false),
		m_gfx_w               (0),     m_gfx_h               (0),
		m_gfx_fullscreen      (false),
		m_game                (0)
{
	//TODO: EXENAME gets written out on windows!
	m_commandline["EXENAME"]=argv[0];

	for (int i=1; i<argc; ++i) {
		std::string opt=argv[i];
		std::string value;
		SSS_T pos;

		//are we looking at an option at all?
		if (opt.substr(0,2)=="--") {
			//yes. remove the leading "--", just for cosmetics
			opt.erase(0,2);
		} else {
			//no. mark the commandline as faulty and stop parsing
			m_commandline.clear();
			cout << _("Malformed option: ") << opt << endl << endl;
			break;
		}

		//look if this option has a value
		pos=opt.find("=");

		if (pos==std::string::npos) { //if no equals sign found
			value="";
		} else {
			//extract option value
			value=opt.substr(pos+1, opt.size()-pos);

			//remove value from option name
			opt.erase(pos, opt.size()-pos);
		}

		m_commandline[opt]=value;
	}

	//empty commandline means there were _unhandled_ syntax errors
	//(commandline should at least contain "EXENAME"=="widelands" ! )
	//in the commandline. They should've been handled though.
	//TODO: bail out gracefully instead
	assert(!m_commandline.empty());

	//Now that we now what the user wants, initialize the application

	//create the filesystem abstraction first - we wouldn't even find the
	//config file without this
	g_fs=new LayeredFileSystem();
	setup_searchpaths(m_commandline["EXENAME"]);

	init_settings();
	init_hardware();

	g_fh = new Font_Handler();

	//make sure we didn't forget to read any global option
	g_options.check_used();
}

/**
 * Shut down all subsystems in an orderly manner
 * \todo Handle errors that happen here!
 */
WLApplication::~WLApplication()
{
	//make sure there are no memory leaks
	//don't just delete a still existing game, it's a bug if it's still here
	//TODO: bail out gracefully instead
	assert(m_game==0);

	//Do use the opposite order of WLApplication::init()

	shutdown_hardware();
	shutdown_settings();

	assert(g_fh);
	delete g_fh;
	g_fh = 0;

	assert(g_fs!=0);
	delete g_fs;
	g_fs = 0;
}

/**
 * The main loop. Plain and Simple.
 *
 * \todo Refactor the whole mainloop out of class \ref UI::Panel into here.
 * In the future: push the first event on the event queue, then keep
 * dispatching events until it is time to quit.
 */
void WLApplication::run()
{
	if (editor_commandline) {
		g_sound_handler.start_music("ingame");
		//  FIXME if m_editor_filename.size(), load that file in the editor.
		//  FIXME The editor loading code is unfortunately complicated and hidden
		//  FIXME in Main_Menu_Load_Map::load_map, which is UI code.
		Editor * const e = new Editor(); //  Local variable does not work (sig11).
		e->run();
		delete e;
	} else if (m_loadgame_filename.size()) {
		assert(not m_game);
		m_game = new Game;
		m_game->run_load_game(true, m_loadgame_filename.c_str());
		delete m_game;
		m_game = 0;
	} else if (m_tutorial_filename.size()) {
		assert(not m_game);
		m_game = new Game;
		m_game->run_splayer_map_direct(m_tutorial_filename.c_str(), true);
		delete m_game;
		m_game = 0;
	} else {

		g_sound_handler.start_music("intro");

		{
			Fullscreen_Menu_Intro intro;
			intro.run();
		}

		//TODO: what does this do? where does it belong? read up on GGZ! #fweber
		if(NetGGZ::ref()->used())
		{
			if(NetGGZ::ref()->connect())
			{
				NetGame *netgame;

				if(NetGGZ::ref()->host()) netgame = new NetHost();
				else
				{
					while(!NetGGZ::ref()->ip())
						NetGGZ::ref()->data();

					IPaddress peer;
					SDLNet_ResolveHost (&peer, NetGGZ::ref()->ip(),
					                    WIDELANDS_PORT);
					netgame = new NetClient(&peer);
				}
				netgame->run();
				delete netgame;
			}
		}

		g_sound_handler.change_music("menu", 1000);
		mainmenu();

	}

	g_sound_handler.stop_music(500);

	return;

	//----------------------------------------------------------------------
	//everything below here is unfinished work. please don't modify #fweber

	while(!m_should_die) {
		SDL_Event e;

		//get an event from the queue; on empty queue, create an idle event
		if (SDL_PollEvent(&e)==0) {
			e.type=SDL_USEREVENT;
			e.user.code=IDLE;
			SDL_PushEvent(&e);
			//TODO: handle error
		}

		if(journal->is_recording()) journal->record_event(&e);
		//TODO: playback

		switch(e.type) {
		case SDL_MOUSEMOTION:
			break;
		case SDL_KEYDOWN:
			break;
		case SDL_SYSWMEVENT:
			break;
		case SDL_QUIT:
			m_should_die=true;
			break;
		case SDL_USEREVENT:
			switch(e.user.code) {
			case IDLE:
				//perhaps sleep a little?
				break;
			case CHANGE_MUSIC:
				g_sound_handler.change_music();
				break;
			}
			break;
		}
		break;
	}
}

/**
 * Get an event from the SDL queue, just like SDL_PollEvent.
 * Perform the meat of playback/record stuff when needed.
 *
 * Throttle is a hack to stop record files from getting extremely huge.
 * If it is set to true, we will idle loop if we can't get an SDL_Event
 * returned immediately if we're recording. If there is no user input,
 * the actual mainloop will be throttled to 100fps.
 *
 * \param ev the retrieved event will be put here
 * \param throttle Limit recording to 100fps max (not the event loop itself!)
 * \return true if an event was returned inside \ref ev, false otherwise
 * \todo Catch Journalfile_error
 */
const bool WLApplication::poll_event(SDL_Event *ev, const bool throttle)
{
	bool haveevent=false;

restart:
	//inject synthesized events into the event queue when playing back
	if (journal->is_playingback())
		haveevent=journal->read_event(ev);
	else {
		haveevent=SDL_PollEvent(ev);

		if (haveevent)
		{
			// We edit mouse motion events in here, so that
			// differences caused by GrabInput or mouse speed
			// settings are invisible to the rest of the code
			switch(ev->type) {
			case SDL_MOUSEMOTION:
				ev->motion.xrel += m_mouse_internal_comp_position.x;
				ev->motion.yrel += m_mouse_internal_comp_position.y;
				m_mouse_internal_comp_position = Point(0, 0);

				if (m_mouse_locked) {
					set_mouse_pos(m_mouse_position);

					ev->motion.x = m_mouse_position.x;
					ev->motion.y = m_mouse_position.y;
				}

				break;
			case SDL_USEREVENT:
				if (ev->user.code==CHANGE_MUSIC)
					g_sound_handler.change_music();

				break;
			}
		}
	}

	// log all events into the journal file
	if (journal->is_recording()) {
		if (haveevent)
			journal->record_event(ev);
		else {
			// Implement the throttle to avoid very quick inner mainloops when
			// recoding a session
			if (throttle && journal->is_playingback()) {
				static int lastthrottle = 0;
				int time = SDL_GetTicks();

				if (time - lastthrottle < 10)
					goto restart;

				lastthrottle = time;
			}

			journal->set_idle_mark();
		}
	}
	else
	{ //not recording
		if (haveevent)
		{
			// Eliminate any unhandled events to make sure record
			// and playback are _really_ the same.
			// Yes I know, it's overly paranoid but hey...
			switch(ev->type) {
			case SDL_KEYDOWN:
			case SDL_KEYUP:
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
			case SDL_MOUSEMOTION:
			case SDL_QUIT:
				break;
			default:
				goto restart;
			}
		}
	}

	return haveevent;
}


/**
 * Pump the event queue, get packets from the network, etc...
 */
void WLApplication::handle_input(const InputCallback *cb)
{
	bool gotevents = false;
	SDL_Event ev; //  Valgrind says:
	// Conditional jump or move depends on uninitialised value(s)
	// at 0x407EEDA: (within /usr/lib/libSDL-1.2.so.0.11.0)
	// by 0x407F78F: (within /usr/lib/libSDL-1.2.so.0.11.0)
	// by 0x404FB12: SDL_PumpEvents (in /usr/lib/libSDL-1.2.so.0.11.0)
	// by 0x404FFC3: SDL_PollEvent (in /usr/lib/libSDL-1.2.so.0.11.0)
	// by 0x8252545: WLApplication::poll_event(SDL_Event*, bool) (wlapplication.cc:309)
	// by 0x8252EB6: WLApplication::handle_input(InputCallback const*) (wlapplication.cc:459)
	// by 0x828B56E: UI::Panel::run() (ui_panel.cc:148)
	// by 0x8252FAB: WLApplication::run() (wlapplication.cc:212)
	// by 0x81427A6: main (main.cc:39)

	NetGGZ::ref()->data();
	NetGGZ::ref()->datacore();

	// We need to empty the SDL message queue always, even in playback mode
	// In playback mode, only F10 for premature exiting works
	if (journal->is_playingback()) {
		while(SDL_PollEvent(&ev)) {
			switch(ev.type) {
			case SDL_KEYDOWN:
				if (ev.key.keysym.sym == SDLK_F10) // TEMP - get out of here quick
					m_should_die = true;
				break;
			case SDL_QUIT:
				m_should_die = true;
				break;
			}
		}
	}

	// Usual event queue
	while(poll_event(&ev, !gotevents)) {

		gotevents = true;

		// CAREFUL: Record files do not save the entire SDL_Event structure.
		// Therefore, playbacks are incomplete. When you change the following
		// code so that it uses previously unused fields in SDL_Event,
		// please also take a look at Sys_PollEvent()

		switch(ev.type) {
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			if (ev.key.keysym.sym == SDLK_F10) // TEMP - get out of here quick
			{
				if (ev.type == SDL_KEYDOWN)
					m_should_die = true;
				break;
			}
			if (ev.key.keysym.sym == SDLK_F11) // take screenshot
			{
				if (ev.type == SDL_KEYDOWN) for (uint nr = 0; nr < 10000; ++nr) {
					char buffer[256];
					snprintf(buffer, sizeof(buffer), "shot%04u.bmp", nr);
					if (g_fs->FileExists(buffer)) continue;
					g_gr->screenshot(*buffer);
						break;
				}
				break;
			}
			if (cb && cb->key) {
				int c;

				c = ev.key.keysym.unicode;
				if (c < 32 || c >= 128)
					c = 0;

				cb->key
				(ev.type == SDL_KEYDOWN,
				 ev.key.keysym.sym,
				 static_cast<const char>(c));
			}
			break;
		case SDL_MOUSEBUTTONDOWN:
			if (cb and cb->mouse_press) {
				if (m_mouse_swapped) {
					switch (ev.button.button) {
					case SDL_BUTTON_LEFT: ev.button.button = SDL_BUTTON_RIGHT; break;
					case SDL_BUTTON_RIGHT: ev.button.button = SDL_BUTTON_LEFT;
					}
				}
				assert(ev.button.state == SDL_PRESSED);
				cb->mouse_press(ev.button.button, ev.button.x, ev.button.y);
			}
			break;
		case SDL_MOUSEBUTTONUP:
			if (cb and cb->mouse_release) {
				if (m_mouse_swapped) {
					switch (ev.button.button) {
					case SDL_BUTTON_LEFT: ev.button.button = SDL_BUTTON_RIGHT; break;
					case SDL_BUTTON_RIGHT: ev.button.button = SDL_BUTTON_LEFT;
					}
				}
				assert(ev.button.state == SDL_RELEASED);
				cb->mouse_release(ev.button.button, ev.button.x, ev.button.y);
			}
			break;

		case SDL_MOUSEMOTION:
			// All the interesting stuff is now in Sys_PollEvent()

			m_mouse_position = Point(ev.motion.x,ev.motion.y);

			if ((ev.motion.xrel or ev.motion.yrel) and cb and cb->mouse_move)
				cb->mouse_move
				(ev.motion.state,
				 ev.motion.x,    ev.motion.y,
				 ev.motion.xrel, ev.motion.yrel);
			break;

		case SDL_QUIT:
			m_should_die = true;
			break;
		}
	}
}

/**
 * Return the current time, in milliseconds
 * \todo Convert return value from int to Uint32, SDL's native time resolution
 */
const int WLApplication::get_time()
{
	Uint32 time;

	time=SDL_GetTicks();
	journal->timestamp_handler(&time); //might change the time when playing back!

	return time;
}

/**
 * Move the mouse cursor.
 * No mouse moved event will be issued.
 */
void WLApplication::set_mouse_pos(const Point p)
{
	m_mouse_position = p;
	m_mouse_internal_x = p.x;
	m_mouse_internal_y = p.y;

	do_warp_mouse(p); // sync mouse positions
}

/**
 * Changes input grab mode.
 */
void WLApplication::set_input_grab(bool grab)
{
	if (journal->is_playingback())
		return; // ignore in playback mode

	m_input_grab = grab;

	if (grab) {
		SDL_WM_GrabInput(SDL_GRAB_ON);
		m_mouse_internal_x = m_mouse_position.x;
		m_mouse_internal_y = m_mouse_position.y;
	} else {
		SDL_WM_GrabInput(SDL_GRAB_OFF);
		do_warp_mouse(m_mouse_position);
	}
}

/**
 * Set a new mouse speed
 */
void WLApplication::set_mouse_speed(float speed)
{
	if (speed <= 0.1 || speed >= 10.0)
		speed = 1.0;
	m_mouse_speed = speed;
}

/**
 * Set the mouse boundary after a change of resolution
 * This is manually imported by graphic.cc
 */
void WLApplication::set_max_mouse_coords(const int x, const int y)
{
	m_mouse_maxx = x;
	m_mouse_maxy = y;
}

/**
 * Warp the SDL mouse cursor to the given position.
 * Store the delta mouse_internal_compx/y, so that the resulting motion
 * event can be eliminated.
 */
void WLApplication::do_warp_mouse(const Point p) {
	if (not journal->is_playingback()) { //  don't warp anything during playback
		Point cur_position;
		SDL_GetMouseState(&cur_position.x, &cur_position.y);
		if (cur_position != p) {
			m_mouse_internal_comp_position += cur_position - p;
			SDL_WarpMouse(p.x, p.y);
		}
	}
}

/**
 * Initialize the graphics subsystem (or shutdown, if system == GFXSYS_NONE)
 * with the given resolution.
 * Throws an exception on failure.
 *
 * \note Because of the way pictures are handled now, this function must not be
 * called while UI elements are active.
 *
 * \todo Ensure that calling this with active UI elements does barf
 * \todo Document parameters
 */
void WLApplication::init_graphics(const int w, const int h,
                                  const int bpp, const bool fullscreen)
{
	if (w == m_gfx_w && h == m_gfx_h && fullscreen == m_gfx_fullscreen)
		return;

	if (g_gr)
	{
		delete g_gr;
		g_gr = 0;
	}

	m_gfx_w = w;
	m_gfx_h = h;
	m_gfx_fullscreen = fullscreen;

	// If we are not to be shut down
	if( w && h ) {
		g_gr = new Graphic(w, h, bpp, fullscreen);
		set_max_mouse_coords(w, h);
	}
}

/**
 * Read the config file, parse the commandline and give all other internal
 * parameters sensible default values
 */
const bool WLApplication::init_settings()
{
	Section *s=0;

	//create a journal so that parse_command_line can open the journal files
	journal=new Journal();

	//read in the configuration file
	g_options.read("config", "global");
	s=g_options.pull_section("global");

	// Set Locale and grab default domain
	i18n::set_locale( s->get_string("language", "en_EN"));
	i18n::grab_textdomain("widelands");

	//then parse the commandline - overwrites conffile settings
	parse_command_line();

	// Input
	m_should_die = false;
	m_input_grab = false;
	m_mouse_swapped = false;
	m_mouse_locked = false;
	m_mouse_speed = 1.0;
	m_mouse_position = Point(0, 0);
	m_mouse_maxx = m_mouse_maxy = 0;
	m_mouse_internal_x = m_mouse_internal_y = 0;
	m_mouse_internal_comp_position = Point(0, 0);

	set_input_grab(s->get_bool("inputgrab", false));
	set_mouse_swap(s->get_bool("swapmouse", false));
	set_mouse_speed(s->get_float("mousespeed", 1.0));

	m_gfx_fullscreen=s->get_bool("fullscreen", false);

	// KLUDGE!
	// Without this, xres, yres and workareapreview get dropped by
	// check_used().
	// Profile needs support for a Syntax definition to solve this in a
	// sensible way
	s->get_string("xres");
	s->get_string("yres");
	s->get_bool("workareapreview");
	s->get_bool("nozip");
	s->get_int("border_snap_distance");
	s->get_int("panel_snap_distance");
	s->get_bool("snap_windows_only_when_overlapping");
	s->get_bool("dock_windows_to_edges");
	// KLUDGE!

	return true;
}

/**
 * Remember the last settings: write them into the config file
 */
void WLApplication::shutdown_settings()
{
	// To be proper, release our textdomain
	i18n::release_textdomain();

	// overwrite the old config file
	g_options.write("config", true);

	assert(journal);
	delete journal;
	journal=0;
}

/**
 * Start the hardware: switch to graphics mode, start sound handler
 *
 * \pre The locale must be known before calling this
 *
 * \return true if there were no fatal errors that prevent the game from running
 */
const bool WLApplication::init_hardware()
{
	Uint32 sdl_flags=0;
	Section *s = g_options.pull_section("global");

	//Start the SDL core
	if(s->get_bool("coredump", false))
		sdl_flags=SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE;
	else
		sdl_flags=SDL_INIT_VIDEO;

	if (SDL_Init(sdl_flags) == -1) {
		//TODO: that's not handled yet!
		throw wexception("Failed to initialize SDL: %s", SDL_GetError());
	}

	SDL_ShowCursor(SDL_DISABLE);
	SDL_EnableUNICODE(1); // useful for e.g. chat messages
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

	uint xres = 800;
	uint yres = 600;
	if (m_loadgame_filename.size() or m_tutorial_filename.size()) {
		// main menu will not be shown, so set in-game resolution
		xres = s->get_int("xres",xres);
		yres = s->get_int("yres",yres);
	}

	init_graphics(xres, yres, s->get_int("depth",16), m_gfx_fullscreen);

	// Start the audio subsystem
	// must know the locale before calling this!
	g_sound_handler.init(); //  FIXME memory leak!

	return true;
}

/**
 * Shut the hardware down: stop graphics mode, stop sound handler
 */
void WLApplication::shutdown_hardware()
{
	g_sound_handler.shutdown();
	SDL_QuitSubSystem(SDL_INIT_AUDIO);

	if (g_gr) {
		cout<<"WARNING: Hardware shutting down although graphics system"
		<<" ist still alive!"<<endl;
	}
	init_graphics(0, 0, 0, false);

	SDL_Quit();
}

/**
 * Parse the command line given in \ref m_commandline
 *
 * \return false if there were errors during parsing \e or if "--help" was given,
 * true otherwise.
*/
void WLApplication::parse_command_line() throw(Parameter_error)
{
	if(m_commandline.count("help")>0 || m_commandline.count("version")>0) {
		throw Parameter_error(); //no message on purpose
	}

	if(m_commandline.count("ggz")>0) {
		NetGGZ::ref()->init();
		m_commandline.erase("ggz");
	}

	if(m_commandline.count("nosound")>0) {
		g_sound_handler.m_nosound=true;
		m_commandline.erase("nosound");
	}

	if(m_commandline.count("nozip")>0) {
		g_options.pull_section("global")->create_val("nozip","true");
		m_commandline.erase("nozip");
	}

	if(m_commandline.count("double")>0) {
		#ifdef DEBUG
		#ifndef __WIN32__
		init_double_game();
		#else
		cout << endl << _("Sorry, no double-instance debugging on WIN32.")
				<< endl << endl;
		#endif
		#else
		cout << _("--double is disabled. This is not a debug build!") << endl;
		#endif

		m_commandline.erase("double");
	}

	if(m_commandline.count("editor")>0) {
		m_editor_filename = m_commandline["editor"];
		editor_commandline=1;
		m_commandline.erase("editor");
	}

	if (m_commandline.count("loadgame") > 0) {
		m_loadgame_filename = m_commandline["loadgame"];
		m_commandline.erase("loadgame");
	}

	if (m_commandline.count("tutorial") > 0) {
		m_tutorial_filename = m_commandline["tutorial"];
		m_commandline.erase("tutorial");
	}

	//Note: it should be possible to record and playback at the same time,
	//but why would you?
	if(m_commandline.count("record")>0) {
		if (m_commandline["record"].empty())
			throw Parameter_error("ERROR: --record needs a filename!");

		try {
			journal->start_recording(m_commandline["record"]);
		}
		catch (Journalfile_error e) {
			//TODO: tell the user
		}

		m_commandline.erase("record");
	}

	if(m_commandline.count("playback")>0) {
		if (m_commandline["playback"].empty())
			throw Parameter_error("ERROR: --playback needs a filename!");

		try {
			journal->start_playback(m_commandline["playback"]);
		}
		catch (Journalfile_error e) {
			//TODO: tell the user
		}

		m_commandline.erase("playback");
	}

	//If it hasn't been handled yet it's probably an attempt to
	//override a conffile setting
	//With typos, this will create invalid config settings. They
	//will be taken care of (==ignored) when saving the options

	const std::map<std::string, std::string>::const_iterator commandline_end =
	   m_commandline.end();
	for
	(std::map<std::string, std::string>::const_iterator it =
	         m_commandline.begin();
	      it != commandline_end;
	      ++it)
	{
		//TODO: barf here on unkown option; the list of known options
		//TODO: needs to be centralized

		g_options.pull_section("global")->create_val
		(it->first.c_str(), it->second.c_str());
	}
}

/**
 * Print usage information
 */
void WLApplication::show_usage()
{
	char buffer[80];
	snprintf(buffer, sizeof(buffer), _("This is Widelands-%s").c_str(), BUILD_ID);
	cout << buffer << endl << endl;
	cout << _("Usage: widelands <option0>=<value0> ... <optionN>=<valueN>")
		<< endl << endl;
	cout << _("Options:") << endl << endl;
	cout << _(" --<config-entry-name>=value overwrites any config file setting\n\n"
			  " --record=FILENAME    Record all events to the given filename for later playback\n"
			  " --playback=FILENAME  Playback given filename (see --record)\n\n"
			  " --coredump=[yes|no]  Generates a core dump on segfaults instead of using the SDL\n")
			  << endl;
#ifdef USE_GGZ
	cout << _(" --ggz                Starts game as GGZ Gaming Zone client (don't use!)")
			<< endl;
#endif
	cout << _(" --nosound            Starts the game with sound disabled\n"
			  " --nozip              Do not save files as binary zip archives.\n\n"
			  " --editor             Directly starts the Widelands editor.\n")
			  << endl;
#ifdef DEBUG
#ifndef __WIN32__
	cout << _(" --double             Start the game twice (for localhost network testing)\n")
			<< endl;
#endif
#endif
	cout << _(" --help               Show this help\n") << endl;
	cout << _("Bug reports? Suggestions? Check out the project website:\n"
			  "        http://www.sourceforge.net/projects/widelands\n\n"
			  "Hope you enjoy this game!\n") << endl;
}

#ifdef DEBUG
#ifndef __WIN32__
/**
 * Fork off a second game to test network gaming
 *
 * \warning You must call this \e before any hardware initialization - most
 * notably before \ref SDL_Init()
 */
void WLApplication::init_double_game ()
{
	if (pid_me!=0)
		return;

	pid_me=getpid();
	pid_peer=fork();
	//TODO: handle fork errors

	assert (pid_peer>=0);

	if (pid_peer==0) {
		pid_peer=pid_me;
		pid_me=getpid();

		may_run=1;
	}

	signal (SIGUSR1, signal_handler);

	atexit (quit_handler);
}

/**
 * On SIGUSR1, allow ourselves to continue running
 */
void WLApplication::signal_handler(int) {may_run++;}

/**
 * Kill the other instance when exiting
 *
 * \todo This works but is not very clean (each process killing each other)
 */
void WLApplication::quit_handler()
{
	kill (pid_peer, SIGTERM);
	sleep (2);
	kill (pid_peer, SIGKILL);
}

/**
 * Voluntarily yield to the second Widelands process. This was implemented
 * because some machines got horrible responsiveness when using --double, so we
 * forced good reponsiveness by using cooperative multitasking (between the two
 * Widelands instances, that is)
 */
void WLApplication::yield_double_game()
{
	if (pid_me==0)
		return;

	if (may_run>0) {
		may_run--;
		kill (pid_peer, SIGUSR1);
	}

	if (may_run==0)
		usleep (500000);

	// using sleep instead of pause avoids a race condition
	// and a deadlock during connect
}
#endif
#endif

/**
 * Run the main menu
 */
void WLApplication::mainmenu()
{
	bool done=false;

	while(!done) {
		Fullscreen_Menu_Main mm;
		switch (mm.run()) {
		case Fullscreen_Menu_Main::mm_singleplayer:
			mainmenu_singleplayer();
			break;

		case Fullscreen_Menu_Main::mm_multiplayer:
			mainmenu_multiplayer();
			break;

		case Fullscreen_Menu_Main::mm_options: {
				Section *s = g_options.pull_section("global");
				Options_Ctrl om(s);
			}
			break;

		case Fullscreen_Menu_Main::mm_readme: {
			Fullscreen_Menu_FileView ff("txts/README");
			ff.run();
			}
			break;

		case Fullscreen_Menu_Main::mm_license: {
			Fullscreen_Menu_FileView ff("txts/COPYING");
			ff.run();
			}
			break;

		case Fullscreen_Menu_Main::mm_editor:
			{
				Editor* e=new Editor(); //  Local variable does not work (sig11).
				e->run();
				delete e;
				break;
			}

		default:
		case Fullscreen_Menu_Main::mm_exit:
			done=true;
			break;
		}
	}
}

/**
 * Run the singleplayer menu
 */
void WLApplication::mainmenu_singleplayer()
{

	for (bool done = false; not done;) {
		int code;
		{
			Fullscreen_Menu_SinglePlayer single_player_menu;
			code = single_player_menu.run();
		}

		//  This is the code returned by UI::Panel::run() when the panel is dying.
		//  Make sure that the program exits when the window manager says so.
		assert(Fullscreen_Menu_SinglePlayer::Back == UI::Panel::dying_code);

		if (code == Fullscreen_Menu_SinglePlayer::Back) break;

		m_game = new Game;

		switch(code) {
		case Fullscreen_Menu_SinglePlayer::New_Game:
			if (m_game->run_single_player())
				done=true;
			break;

		case Fullscreen_Menu_SinglePlayer::Load_Game:
			if (m_game->run_load_game(true))
				done=true;
			break;

		case Fullscreen_Menu_SinglePlayer::Tutorial_Campaign:
			{
				const char * filename = 0;
				{
					Fullscreen_Menu_TutorialSelectMap select_tutorial;
					if (const uint i = select_tutorial.run())
						filename = select_tutorial.get_mapname(i);
				}
				if (filename) m_game->run_splayer_map_direct(filename, true);
			}
			break;
		}

		delete m_game;
		m_game = 0;
	}

}

/**
 * Run the multiplayer menu
 */
void WLApplication::mainmenu_multiplayer()
{
	NetGame* netgame = 0;
	Fullscreen_Menu_NetSetup ns;

	if (NetGGZ::ref()->tables().size() > 0) ns.fill(NetGGZ::ref()->tables());

	switch (ns.run()) {
	case Fullscreen_Menu_NetSetup::HOSTGAME:
		netgame=new NetHost();
		break;
	case Fullscreen_Menu_NetSetup::JOINGAME: {
		IPaddress peer;

		//if (SDLNet_ResolveHost (&peer, ns->get_host_address(), WIDELANDS_PORT) < 0)
		//throw wexception("Error resolving hostname %s: %s\n", ns->get_host_address(), SDLNet_GetError());
		ulong addr;
		ushort port;

		if (not ns.get_host_address(addr, port))
			throw wexception("Address of game server is no good");

		peer.host=addr;
		peer.port=port;

		netgame=new NetClient(&peer);
	}
		break;
	case Fullscreen_Menu_NetSetup::INTERNETGAME: {
		Fullscreen_Menu_InetServerOptions igo;
		const int igo_code = igo.run();

		// Get informations here
		const std::string host   = igo.get_server_name();
		const std::string player = igo.get_player_name();

		if (igo_code) {
			Game_Server_Connection csc(host, GAME_SERVER_PORT);

			try {
				csc.connect();
			} catch(...) {
				// TODO: error handling here
				throw;
			}

			csc.set_username(player.c_str());

			// Wowi, we are connected. Let's start the lobby
			Fullscreen_Menu_InetLobby il(&csc);
			il.run();
		}
	}
		break;
	case Fullscreen_Menu_NetSetup::HOSTGGZGAME:
		NetGGZ::ref()->launch();
		//  fallthrough
	case Fullscreen_Menu_NetSetup::JOINGGZGAME: {
		if(NetGGZ::ref()->host()) netgame = new NetHost();

		else {
			while(!NetGGZ::ref()->ip()) NetGGZ::ref()->data();

			IPaddress peer;
			SDLNet_ResolveHost (&peer, NetGGZ::ref()->ip(), WIDELANDS_PORT);
			netgame = new NetClient(&peer);
		}
	}
	}

	if (netgame!=0) {
		netgame->run();
		delete netgame;
	}

}

/**
* Try to save if possible
 */
void WLApplication::emergency_save(const std::string &) {
	if (NULL == m_game)
		return;

	try {
		time_t t;
		time(&t);
		SaveHandler * save_handler = m_game->get_save_handler();
		char * current_time = ctime(&t);
		// remove trailing newline character
		std::string time_string (current_time, strlen(current_time)-1);
		SSS_T pos = std::string::npos;
		// ':' is not a valid file name character under Windows
		while ((pos = time_string.find (':')) != std::string::npos) {
			time_string[pos] = '.';
		}

		std::string filename = save_handler->create_file_name
			(save_handler->get_base_dir(), time_string);
		save_handler->save_game(m_game, filename);
	} catch (...) {
		log ("Emergency save failed");
		throw;
	}
}
