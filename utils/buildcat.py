#!/usr/bin/env python
# encoding: utf-8

##############################################################################
#
# This script holds the common functions for locale handling & generation.
#
# Usage: add non-iterative catalogs to MAINPOT list, or iterative catalogs
# (i.e., tribes) to ITERATIVEPOTS as explained below
#
##############################################################################

from glob import glob
from itertools import takewhile
import os
import subprocess
import sys
from time import strftime,gmtime

try:
    maketrans = "".maketrans
except AttributeError:
    # fallback for python2
    from string import maketrans

from confgettext import Conf_GetText

# Holds the names of non-iterative catalogs to build and the
# corresponding source paths list. Note that paths MUST be relative to po/pot,
# to let .po[t] comments point to somewhere useful
MAINPOTS = [
    ( "maps/maps", [
        "../../data/maps/*/elemental",
        "../../data/maps/*/*/elemental",
        "../../data/campaigns/*.conf",
        "../../data/campaigns/*/elemental"
    ] ),
    ( "texts/texts", ["../../data/txts/*.lua",
                  "../../data/txts/tips/*.tip"] ),
    ( "widelands/widelands", [
                    "../../src/wlapplication.cc",
                    "../../src/*/*.cc",
                    "../../src/*/*/*.cc",
                    "../../src/*/*/*/*.cc",
                    "../../src/*/*/*/*/*.cc",
                    "../../src/*/*/*/*/*/*.cc",
                    "../../src/wlapplication.h",
                    "../../src/*/*.h",
                    "../../src/*/*/*.h",
                    "../../src/*/*/*/*.h",
                    "../../src/*/*/*/*/*.h",
                    "../../src/*/*/*/*/*/*.h",
                    "../../data/scripting/widelands/*.lua",
    ] ),
    ( "widelands_console/widelands_console", [
                    "../../src/wlapplication_messages.cc",
                    "../../src/wlapplication_messages.h",
    ] ),
    ( "win_conditions/win_conditions", [
        "../../data/scripting/win_conditions/*.lua",
        "../../data/scripting/win_condition_texts.lua",
    ]),
    ("world/world", [
        "../../data/world/*.lua",
        "../../data/world/*/*.lua",
        "../../data/world/*/*/*.lua",
        "../../data/world/*/*/*/*.lua",
        "../../data/world/*/*/*/*/*.lua",
        "../../data/world/*/*/*/*/*/*.lua",
    ]),
    ("tribes/tribes", [
        "../../data/tribes/scripting/starting_conditions/*/*.lua",
        "../../data/tribes/*.lua",
        "../../data/tribes/*/init.lua",
        "../../data/tribes/*/*/init.lua",
        "../../data/tribes/*/*/*/init.lua",
        "../../data/tribes/*/*/*/*/init.lua",
        "../../data/tribes/*/*/*/*/*/init.lua",
    ]),

    ("tribes_encyclopedia/tribes_encyclopedia", [
        "../../data/tribes/scripting/help/*.lua",
        "../../data/tribes/*/helptexts.lua",
        "../../data/tribes/*/*/helptexts.lua",
        "../../data/tribes/*/*/*/helptexts.lua",
        "../../data/tribes/*/*/*/*/helptexts.lua",
        "../../data/tribes/*/*/*/*/*/helptexts.lua",
    ]),
]


# This defines the rules for iterative generation of catalogs. This allows
# to automatically add new .pot files for newly created directories.
#
# This is a list with structure:
#       - target .pot file mask
#       - base directory to scan for catalogs (referred to Widelands' base dir)
#       - List of source paths for catalog creation: tells the program which
#         files to use for building .pot files (referred to
#         "po/pot/<path_to_pot/" dir, so the file pointers inside .pot files
#         actually point somewhere useful)
#
# For every instance found of a given type, '%s' in this values is replaced
# with the name of the instance.
ITERATIVEPOTS = [
    ("scenario_%(name)s/scenario_%(name)s", "data/campaigns/",
         ["../../data/campaigns/%(name)s/extra_data",
          "../../data/campaigns/%(name)s/objective",
          "../../data/campaigns/%(name)s/scripting/*.lua",
          "../../data/scripting/format_scenario.lua"
         ]
    ),
    ("map_%(name)s/map_%(name)s", "data/maps/",
         [ "../../data/maps/%(name)s/scripting/*.lua", ]
    ),
    ("mp_scenario_%(name)s/mp_scenario_%(name)s", "data/maps/MP_Scenarios/",
         [ "../../data/maps/MP_Scenarios/%(name)s/scripting/*.lua", ]
    ),
]


# Options passed to common external programs
XGETTEXTOPTS ="-k_ --from-code=UTF-8"
XGETTEXTOPTS+=" -F -c\"* TRANSLATORS\""
# escaped double quotes are necessary for windows, as it ignores single quotes
XGETTEXTOPTS+=" --copyright-holder=\"Widelands Development Team\""
XGETTEXTOPTS+=" --msgid-bugs-address=\"https://bugs.launchpad.net/widelands\""

# Options for xgettext when parsing Lua scripts
# Official Lua backend of xgettext does not support pgettext and npgettext right
# off the bat and also expects keywords (besides _) to be prefixed with 'gettext.',
# so we need to specify the keywords we need ourselves.
LUAXGETTEXTOPTS ="-k" # Remove known keywords
LUAXGETTEXTOPTS+=" --keyword=_ --flag=_:1:pass-lua-format"
LUAXGETTEXTOPTS+=" --keyword=ngettext:1,2 --flag=ngettext:1:pass-lua-format --flag=ngettext:2:pass-lua-format"
LUAXGETTEXTOPTS+=" --keyword=pgettext:1c,2 --flag=pgettext:2:pass-lua-format"
LUAXGETTEXTOPTS+=" --keyword=npgettext:1c,2,3 --flag=npgettext:2:pass-lua-format --flag=npgettext:3:pass-lua-format"
LUAXGETTEXTOPTS+=" --language=Lua --from-code=UTF-8 -F -c\" TRANSLATORS:\""

time_now = gmtime()
# This is the header used for Lua+conf potfiles.
# Set it to something sensible, as much as is possible here.
HEAD =  "# Widelands PATH/TO/FILE.PO\n"
HEAD += "# Copyright (C) 2005-" + strftime("%Y", time_now) + " Widelands Development Team\n"
HEAD += "# FIRST AUTHOR <EMAIL@ADDRESS>, YEAR.\n"
HEAD += "#\n"
HEAD += "msgid \"\"\n"
HEAD += "msgstr \"\"\n"
HEAD += "\"Project-Id-Version: Widelands svnVERSION\\n\"\n"
HEAD += "\"Report-Msgid-Bugs-To: https://bugs.launchpad.net/widelands\\n\"\n"
HEAD += "\"POT-Creation-Date: " + strftime("%Y-%m-%d %H:%M+0000", time_now) + "\\n\"\n"
HEAD += "\"PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\\n\"\n"
HEAD += "\"Last-Translator: FULL NAME <EMAIL@ADDRESS>\\n\"\n"
HEAD += "\"Language-Team: LANGUAGE <widelands-public@lists.sourceforge.net>\\n\"\n"
HEAD += "\"MIME-Version: 1.0\\n\"\n"
HEAD += "\"Content-Type: text/plain; charset=UTF-8\\n\"\n"
HEAD += "\"Content-Transfer-Encoding: 8bit\\n\"\n"
HEAD += "\n"

def are_we_in_root_directory():
    """Make sure we are called in the root directory"""
    if (not os.path.isdir("po")):
        print("Error: no 'po/' subdir found.\n")
        print("This script needs to access translations placed " +
            "under 'po/' subdir, but these seem unavailable. Check " +
            "that you called this script from Widelands' main dir.\n")
        sys.exit(1)


def do_makedirs( dirs ):
    """Create subdirectories. Ignore errors"""
    try:
        os.makedirs( dirs )
    except:
        pass

def pot_modify_header( potfile_in, potfile_out, header ):
    """
    Modify the header of a translation catalog read from potfile_in to
    the given header and write out the modified catalog to potfile_out.

    Returns whether or not the header was successfully modified.

    Note: potfile_in and potfile_out must not point to the same file!
    """
    class State:
        (start,
         possibly_empty_msgid,
         search_for_empty_line,
         header_traversed) = range(4)

    st = State.start
    with open(potfile_in, "rt") as potin:
        for line in potin:
            line = line.strip()

            if st == State.start:
                if line.startswith("msgid \"\""):
                    st = State.possibly_empty_msgid
                elif line.startswith("msgid"):
                    # The first entry is not a header entry,
                    # since msgid is not empty.
                    return False
            elif st == State.possibly_empty_msgid:
                if line.startswith("msgstr"):
                    # msgstr right after msgid "", which means msgid must
                    # be empty, therefore we have reached the header entry
                    st = State.search_for_empty_line
                else:
                    # Header check failed.
                    return False
            elif st == State.search_for_empty_line:
                if not line:
                    st = State.header_traversed
                    break;

        if st != State.header_traversed:
            return False

        with open(potfile_out, "wt") as potout:
            potout.write(header)
            potout.writelines(potin)

        return True

def run_msguniq(potfile):
    msguniq_rv = os.system("msguniq \"%s\" -F --output-file=\"%s\"" % (potfile, potfile))
    if (msguniq_rv):
        sys.stderr.write("msguniq exited with errorcode %i\n" % msguniq_rv)
        return False
    return True

def do_compile( potfile, srcfiles ):
    """
    Search Lua and conf files given in srcfiles for translatable strings.
    Merge the results and write out the corresponding pot file.
    """
    files = []
    for i in srcfiles:
        files += glob(i)
    files = set(files)

    lua_files = set([ f for f in files if
        os.path.splitext(f)[-1].lower() == '.lua' ])
    conf_files = files - lua_files

    temp_potfile = potfile + ".tmp"

    if (os.path.exists(temp_potfile)): os.remove(temp_potfile)

    # Find translatable strings in Lua files using xgettext
    xgettext = subprocess.Popen("xgettext %s --files-from=- --output=\"%s\"" % \
        (LUAXGETTEXTOPTS, temp_potfile), shell=True, stdin=subprocess.PIPE, universal_newlines=True)
    try:
        for fname in lua_files:
            xgettext.stdin.write(os.path.normpath(fname) + "\n")
        xgettext.stdin.close()
    except IOError as err_msg:
        sys.stderr.write("Failed to call xgettext: %s\n" % err_msg)
        return False

    xgettext_status = xgettext.wait()
    if (xgettext_status != 0):
        sys.stderr.write("xgettext exited with errorcode %i\n" % xgettext_status)
        return False

    xgettext_found_something_to_translate = os.path.exists(temp_potfile)

    # Find translatable strings in configuration files
    conf = Conf_GetText()
    conf.parse(conf_files)

    if not (xgettext_found_something_to_translate or conf.found_something_to_translate):
        # Found no translatable strings
        return False

    if (xgettext_found_something_to_translate):
        header_fixed = pot_modify_header(temp_potfile, potfile, HEAD)
        os.remove(temp_potfile)

        if not header_fixed:
            return False

        if (conf.found_something_to_translate):
            # Merge the conf POT with Lua POT
            with open(potfile, "at") as p:
                p.write("\n" + conf.toString())

            if not run_msguniq(potfile):
                return False
    elif (conf.found_something_to_translate):
        with open(potfile, "wt") as p:
            p.write(HEAD + conf.toString())

        # Msguniq is run here only to sort POT entries by file
        if not run_msguniq(potfile):
            return False

    return True



def do_compile_src( potfile, srcfiles ):
    """
    Use xgettext for parse the given C++ files in srcfiles. Merge the results
    and write out the given potfile
    """
    # call xgettext and supply source filenames via stdin
    gettext_input = subprocess.Popen("xgettext %s --files-from=- --output=%s" % \
            (XGETTEXTOPTS, potfile), shell=True, stdin=subprocess.PIPE, universal_newlines=True).stdin
    try:
        for one_pattern in srcfiles:
            # 'normpath' is necessary for windows ('/' vs. '\')
            # 'glob' handles filename wildcards
            for one_file in glob(os.path.normpath(one_pattern)):
                gettext_input.write(one_file + "\n")
        return gettext_input.close()
    except IOError as err_msg:
        sys.stderr.write("Failed to call xgettext: %s\n" % err_msg)
        return -1


##############################################################################
#
# Build a list of catalogs from iterative rules above. Returns a list of
# type ("catalog_name", ["source_paths_list"])
#
##############################################################################
def do_find_iterative(prefix, basedir, srcmasks):
    res = []

    directories = sorted(
        d for d in os.listdir(basedir) if
            os.path.isdir(os.path.normpath("%s/%s" % (basedir, d))) and
            not os.path.basename(d).startswith('.')
    )
    for filename in directories:
        srcfiles = []
        for p in srcmasks:
            srcfiles.append(p % { "name": filename })
        name = prefix % { "name": filename }
        res.append((name, srcfiles))

    return res


##############################################################################
#
# Regenerate all .pot files specified above and place them under pot/ tree
#
##############################################################################
def do_update_potfiles():
        print("Generating reference catalogs:")

        # Build the list of catalogs to generate
        potfiles = MAINPOTS
        for prefix, basedir, srcfiles in ITERATIVEPOTS:
            potfiles += do_find_iterative(prefix, basedir, srcfiles)

        # Generate .pot catalogs
        dangerous_chars = "'\" " # Those chars are replaced via '_'
        for pot, srcfiles in potfiles:
            pot = pot.lower().translate(maketrans(dangerous_chars, len(dangerous_chars)*"_"))
            path = os.path.normpath("po/" + os.path.dirname(pot))
            do_makedirs(path)
            oldcwd = os.getcwd()
            os.chdir(path)
            potfile = os.path.basename(pot) + '.pot'
            if pot.startswith('widelands'):
                # This catalogs can be built with xgettext
                do_compile_src(potfile , srcfiles )
                succ = True
            else:
                succ = do_compile(potfile, srcfiles)

            os.chdir(oldcwd)

            if succ:
                print("\tpo/%s.pot" % pot)
            else:
                os.rmdir(path)

        print("")


##############################################################################
#
# Compile a target .po file from source "po" and "pot" catalogs. Dump result
# to "dst" file.
#
##############################################################################
def do_buildpo(po, pot, dst):
    rv = os.system("msgmerge -q --no-wrap %s %s -o %s" % (po, pot, dst))
    if rv:
        raise RuntimeError("msgmerge exited with errorcode %i!" % rv)
    return rv


##############################################################################
#
# Modify source .po file to suit project specific needs. Dump result to a
# different destination file
#
##############################################################################
def do_tunepo(src, dst):
    """
    Try to reduce the differences in generated po files: we keep the generated
    header, but do some modifications in comments to make diffs smaller
    """
    outlines = []

    # Copy header verbatim.
    input_iter = open(src)
    for line in takewhile(lambda l: l.strip(), input_iter):
        outlines.append(line)
    outlines.append("\n")

    for line in input_iter:
        # Some comments in .po[t] files show filenames and line numbers for
        # reference in platform-dependent form (slash/backslash). We
        # standarize them to slashes, since this results in smaller SVN diffs
        if line.startswith("#:"):
            line = line.replace('\\', '/')

        outlines.append(line)

    open(dst, 'w').writelines(outlines)


##############################################################################
#
# Update .po files for a given language dir, or create empty ones if there's
# no translation present.
#
##############################################################################
def do_update_po(lang, files):
    sys.stdout.write("\t%s:\t" % lang)

    for f in files:
        # File names to use
        pot = os.path.normpath("po/%s" % f)
        po = os.path.join(os.path.dirname(pot), lang + '.po')
        tmp = "tmp.po"

        if not (os.path.exists(po)):
            # No need to call msgmerge if there's no translation
            # to merge with. We can use .pot file as input file
            # below, but we need to make sure the target dir is
            # ready.
            do_makedirs(os.path.dirname(po))
            tmp = pot
            fail = 0
        else:
            fail = do_buildpo(po, pot, tmp)

        if not fail:
            # tmp file is ready, but we need to tune some aspects
            # of it
            do_tunepo(tmp, po)

            if tmp == "tmp.po":
                os.remove("tmp.po")

            sys.stdout.write(".")
            sys.stdout.flush()

    sys.stdout.write("\n")


if __name__ == "__main__":
    # Sanity checks
    are_we_in_root_directory()

    # Make sure .pot files are up to date.
    do_update_potfiles()

    print("")
