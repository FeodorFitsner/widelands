/*
 * Copyright (C) 2006-2014 by the Widelands Development Team
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

#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <SDL.h>
#include <SDL_ttf.h>

#undef main // No, we do not want SDL_main

#include "base/log.h"
#include "graphic/image_io.h"
#include "graphic/render/sdl_surface.h"
#include "graphic/text/rt_errors.h"
#include "graphic/text/test/render.h"
#include "io/filesystem/filesystem.h"
#include "io/streamwrite.h"

namespace {

std::string read_stdin() {
	std::string txt;
	while (!std::cin.eof()) {
		std::string line;
		getline(std::cin, line);
		txt += line + "\n";
	}
	return txt;
}

std::string read_file(std::string fn) {
	std::string txt;
	std::ifstream f;
	f.open(fn.c_str());
	if (!f.good()) {
		std::cerr << "Could not open " << fn << std::endl;
		return "";
	}

	while (f.good()) {
		std::string line;
		getline(f, line);
		txt += line + "\n";
	}
	return txt;
}

int parse_arguments(
   int argc, char** argv, int32_t* w, std::string& outname,
	std::string& inname, std::set<std::string>& allowed_tags)
{
	if (argc < 4) {
		std::cout << "Usage: render <width in pixels> <outname> <inname> [allowed tag1] [allowed "
		             "tags2] ... < "
		             "input.txt" << std::endl << std::endl
		          << "input.txt should contain a valid rich text formatting" << std::endl;
		return 1;
	}

	*w = strtol(argv[1], 0, 10);
	outname = argv[2];
	inname = argv[3];

	for (int i = 4; i < argc; i++)
		allowed_tags.insert(argv[i]);

	return 0;
}

}  // namespace

int main(int argc, char** argv) {
	int32_t w;
	std::set<std::string> allowed_tags;

	std::string outname, inname;
	if (parse_arguments(argc, argv, &w, outname, inname, allowed_tags))
		return 0;

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cerr << "SDLInit did not succeed: " << SDL_GetError() << std::endl;
		return 1;
	}

	if (TTF_Init() == -1) {
		std::cerr << "True Type library did not initialize: " << TTF_GetError() << std::endl;
		return 1;
	}

	std::string txt;
	if (inname == "-")
		txt = read_stdin();
	else
		txt = read_file(inname);
	if (txt.empty()) {
		return 1;
	}

	StandaloneRenderer standalone_renderer;

	try {
		std::unique_ptr<SDLSurface> surf(
		   static_cast<SDLSurface*>(standalone_renderer.renderer()->render(txt, w, allowed_tags)));

		std::unique_ptr<FileSystem> fs(&FileSystem::Create("."));
		std::unique_ptr<StreamWrite> sw(fs->OpenStreamWrite(outname));
		if (!save_surface_to_png(surf.get(), sw.get())) {
			std::cout << "Could not encode PNG." << std::endl;
		}
	}
	catch (RT::Exception& e) {
		std::cout << e.what() << std::endl;
	}

	SDL_Quit();

	return 0;
}