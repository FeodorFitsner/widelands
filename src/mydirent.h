#ifndef __S__DIRENT_H
#define __S__DIRENT_H

#ifndef WIN32
#include <dirent.h>
#include <sys/types.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct dirent
{
	char d_name[MAX_PATH];
};

// ich gehe davon aus, das DIR in der orginal-dirent.h ein handle ist, dessen
// inhalt niemanden interessiert und das es deshalb auch niemanden st�rt, wenn
// da ganz was anderes drinsteht.

typedef class Dir DIR;
class Dir
{
	friend DIR* opendir(const char*);
	friend dirent* readdir(DIR*);
	friend void closedir(DIR*);

// ich weiss nicht, wie das im original geregelt ist. der fileloc.cpp entnehme
// ich, dass der r�ckgabepointer von readfile nicht gel�scht werden muss; da
// ein globaler "dirent ret;" ziemlich dumm w�re (mit dem DIR-handle kann man
// mehrere Verzeichnisse parallel durchsuchen, mit einem globalen ret aber nur
// eine Datei abfragen), l�se ich das erstmal so: man kann f�r jeden verzeich-
// nisdurchlauf je eine datei betrachten.
	dirent ret;
	char* name;
	HANDLE find;
	
	Dir(const char* n);
	~Dir();
};

DIR* opendir(const char* name);
dirent* readdir(DIR* dir);
void closedir(DIR* dir);

#endif //WIN32

#endif // __S__DIRENT_H 
