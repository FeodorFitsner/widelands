/*
 * Copyright (C) 2002-2004, 2006-2013 by the Widelands Development Team
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

#ifndef WL_GRAPHIC_GRAPHIC_H
#define WL_GRAPHIC_GRAPHIC_H

#include <map>
#include <memory>
#include <vector>

#include <SDL.h>

#include "base/rect.h"
#include "graphic/image_cache.h"
#include "graphic/image_catalog.h"
#include "notifications/notifications.h"
#include "notifications/note_ids.h"

#define MAX_RECTS 20

class AnimationManager;
class RenderTarget;
class Surface;
class TextureCache;
class StreamWrite;
struct TerrainTexture;

// Will be send whenever the resolution changes.
struct GraphicResolutionChanged {
	CAN_BE_SEND_AS_NOTE(NoteId::GraphicResolutionChanged)

	// New width and height in pixels.
	int width;
	int height;
};

/**
 * This class is a kind of Swiss Army knife for your graphics need.
 * It initializes the graphic system and provides access to
 * resolutions. It owns an Animation, Image and Surface cache. It
 * also offers functionality to save a screenshot.
 */
class Graphic {
public:
	// Creates a new graphic mode with the given resolution if fullscreen is
	// false, otherwise a window that fills the screen.
	Graphic(int window_mode_w, int window_mode_height, bool fullscreen);
	~Graphic();

	// Gets and sets the resolution.
	void change_resolution(int w, int h);
	int get_xres();
	int get_yres();

	// Changes the window to be fullscreen or not.
	bool fullscreen();
	void set_fullscreen(bool);

	RenderTarget * get_render_target();
	void update();
	bool need_update() const;
	void refresh();
	SDL_Window* get_sdlwindow() {return m_sdl_window;}

	TextureCache& textures() const {return *texture_cache_.get();}

	// Returns all cached images. Use cataloged_image instead of this function
	// if the image is registered there.
	ImageCache& images() const {return *image_cache_.get();}

	// Uses catalog 'key' to fetch an image from images().
	const Image* cataloged_image(ImageCatalog::Keys key);
	ImageCatalog& image_catalog() const {return *image_catalog_.get();}

	AnimationManager& animations() const {return *animation_manager_.get();}

	void save_png(const Image*, StreamWrite*) const;

	// Creates a new TerrainTexture() with the given 'frametime' and using the given
	// 'texture_files' as the images for it and returns it id.
	uint32_t new_maptexture(const std::vector<std::string>& texture_files, uint32_t frametime);
	void animate_maptextures(uint32_t time);

	void screenshot(const std::string& fname) const;
	TerrainTexture * get_maptexture_data(uint32_t id);

private:
	// Called when the resolution (might) have changed.
	void resolution_changed();

	// The height & width of the window should we be in window mode.
	int m_window_mode_width;
	int m_window_mode_height;

	/// This is the main screen Surface.
	/// A RenderTarget for this can be retrieved with get_render_target()
	std::unique_ptr<Surface> screen_;
	/// This saves a copy of the screen SDL_Surface. This is needed for
	/// opengl rendering as the SurfaceOpenGL does not use it. It allows
	/// manipulation the screen context.
	SDL_Window * m_sdl_window;
	SDL_GLContext m_glcontext;
	/// A RenderTarget for screen_. This is initialized during init()
	std::unique_ptr<RenderTarget> m_rendertarget;
	/// This marks the complete screen for updating.
	bool m_update;

	/// Volatile cache of Hardware dependant textures.
	std::unique_ptr<TextureCache> texture_cache_;
	/// Non-volatile cache of hardware independent images. The use the
	/// texture_cache_ to cache their pixel data.
	std::unique_ptr<ImageCache> image_cache_;
	/// Maps images from keys to filenames.
	std::unique_ptr<ImageCatalog> image_catalog_;
	/// This holds all animations.
	std::unique_ptr<AnimationManager> animation_manager_;

	std::vector<std::unique_ptr<TerrainTexture>> m_maptextures;
};

extern Graphic * g_gr;

#endif  // end of include guard: WL_GRAPHIC_GRAPHIC_H
