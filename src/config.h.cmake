#ifndef __CONFIG_H__
#define __CONFIG_H__

// This is the path where the data files are located; If this is an absolute
// path (i.e. starting with a /), it will be interpreted as such. If it is a
// relative path it will be interpreted to be relative to the executable. For a
// standard Linux installation, this should be
// "/usr/local/share/games/widelands" or something along the lines.
#define INSTALL_DATADIR "@WL_INSTALL_DATADIR@"

// True if getenv has been found.
#define HAS_GETENV

#endif /* __CONFIG_H__ */
