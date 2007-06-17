import os
import shutil
import fnmatch
import SCons
import time
import glob
from SCons.Script.SConscript import SConsEnvironment
import string

import sys
sys.path.append("build/scons-tools")
from scons_configure import *
from Distribute import *
from detect_revision import *

#Sanity checks
EnsurePythonVersion(2, 4)
EnsureSConsVersion(0, 97)

#Speedup. If you have problems with inconsistent or wrong builds, look here first
SetOption('max_drift', 1)
SetOption('implicit_cache', 1)

# Pretty output
print

########################################## simple glob that works across BUILDDIR
#TODO: optional recursion

def simpleglob(pattern='*', directory='.'):
        entries=[]

	for entry in os.listdir(Dir(directory).srcnode().abspath):
		if fnmatch.fnmatchcase(entry, pattern):
			entries.append(entry)

	return entries

######################################################### find $ROOT -name $GLOB
#TODO: replace with simpleglob

def find(root, glob):
	files=[]
	for file in os.listdir(root):
		file=os.path.join(root, file)
		if fnmatch.fnmatch(file, glob):
			files.append(file)
		if os.path.isdir(file):
			files+=find(file, glob)
	return files

########################### Create a phony target (not (yet) a feature of scons)

# taken from scons' wiki
def PhonyTarget(alias, action):
	"""Returns an alias to a command that performs the
	   action.  This is implementated by a Command with a
	   nonexistant file target.  This command will run on every
	   build, and will never be considered 'up to date'. Acts
	   like a 'phony' target in make."""

	from tempfile import mktemp
	from os.path import normpath

	phony_file = normpath(mktemp(prefix="phony_%s_" % alias, dir="."))
	return Alias(alias, Command(target=phony_file, source=None, action=action))

############################## Functions for setting permissions when installing
# don't forget to set umask
try:
	os.umask(022)
except OSError:     # ignore on systems that don't support umask
	pass

def InstallPerm(env, dest, files, perm):
	obj = env.Install(dest, files)
	for i in obj:
		env.AddPostAction(i, env.Chmod(str(i), perm))

SConsEnvironment.InstallPerm = InstallPerm
SConsEnvironment.InstallProgram = lambda env, dest, files: InstallPerm(env, dest, files, 0755)
SConsEnvironment.InstallData = lambda env, dest, files: InstallPerm(env, dest, files, 0644)

################################################################################
# CLI options setup

def cli_options():
	opts=Options('build/scons-config.py', ARGUMENTS)
	opts.Add('build', 'debug(default) / profile / release', 'debug')
	opts.Add('build_id', 'To get a default value (SVN revision), leave this empty', '')
	opts.Add('sdlconfig', 'On some systems (e.g. BSD) this is called sdl12-config', 'sdl-config')
	opts.Add('paraguiconfig', '', 'paragui-config')
	opts.Add('install_prefix', '', '/usr/local')
	opts.Add('bindir', '(relative to install_prefix)', 'games')
	opts.Add('datadir', '(relative to install_prefix)', 'share/games/widelands')
	opts.Add('extra_include_path', '', '')
	opts.Add('extra_lib_path', '', '')
	opts.Add('extra_compile_flags', '(does not work with build-widelands.sh!)', '')
	opts.Add('extra_link_flags', '(does not work with build-widelands.sh!)', '')
	opts.AddOptions(
		BoolOption('enable_sdl_parachute', 'Enable SDL parachute?', 0),
		BoolOption('enable_efence', 'Use the efence memory debugger?', 0),
		BoolOption('enable_ggz', 'Use the GGZ Gamingzone?', 0),
		)
	return opts

################################################################################
# Environment setup

# write only *one* signature file in a place where we don't care
SConsignFile('build/scons-signatures')

# Create configuration objects

opts=cli_options()

env=Environment(options=opts)
env.Help(opts.GenerateHelpText(env))

conf=env.Configure(conf_dir='#/build/sconf_temp',log_file='#build/config.log',
		   custom_tests={
				'CheckPKGConfig' : CheckPKGConfig,
				'CheckPKG': CheckPKG,
				'CheckSDLConfig': CheckSDLConfig,
				'CheckSDLVersionAtLeast': CheckSDLVersionAtLeast,
				'CheckCompilerAttribute': CheckCompilerAttribute,
				'CheckCompilerFlag': CheckCompilerFlag,
				'CheckLinkerFlag': CheckLinkerFlag,
				'CheckParaguiConfig': CheckParaguiConfig
		   }
)

################################################################################
# Environment setup
#
# Register tools

env.Tool("ctags", toolpath=['build/scons-tools'])
env.Tool("PNGShrink", toolpath=['build/scons-tools'])
env.Tool("astyle", toolpath=['build/scons-tools'])
env.Tool("Distribute", toolpath=['build/scons-tools'])

################################################################################
# Environment setup
#
# Initial debug info

#This makes LIBPATH work correctly - I just don't know why :-(
#Obviously, env.LIBPATH must be forced to be a list instead of a string. Is this
#a scons problem? Or rather our problem???
env.Append(LIBPATH=[])
env.Append(CPPPATH=[])
env.Append(PATH=[])

print 'Platform:         ', env['PLATFORM']
print 'Build type:       ', env['build']

#TODO: should be detected automagically
if env['PLATFORM']!='win32':
	env.Append(PATH=['/usr/bin', '/usr/local/bin'])

#TODO: should be detected automagically
if env['PLATFORM']=='darwin':
	# this is where DarwinPorts puts stuff by default
	env.Append(CPPPATH='/opt/local/include')
	env.Append(LIBPATH='/opt/local/lib')
	env.Append(PATH='/opt/local/bin')
	# and here's for fink
	env.Append(CPPPATH='/sw/include')
	env.Append(LIBPATH='/sw/lib')
	env.Append(PATH='/sw/bin')

################################################################################
# Autoconfiguration
#
# Parse build type

env.debug=0
env.optimize=0
env.strip=0
env.efence=0
env.profile=0

if env['build'] not in ['debug', 'profile', 'release']:
	print "\nERROR: unknown buildtype:", env['build']
	print "       Please specify a valid build type."
	Exit(1)

if env['build']=='debug':
	env.debug=1
	env.Append(CCFLAGS='-DNOPARACHUTE')

if env['build']=='profile':
	env.debug=1
	env.optimize=1
	env.profile=1
	env.Append(CCFLAGS='-DNOPARACHUTE')

if env['build']=='release':
	env.optimize=1
	env.strip=1
	SDL_PARACHUTE=1

if env.debug==1:
	env.Append(CCFLAGS='-DDEBUG')
else:
	env.Append(CCFLAGS='-DNDEBUG')

if env['enable_efence']=='1':
	env.efence=1

################################################################################

TARGET=parse_cli(env)

env.Append(CPPPATH=env['extra_include_path'])
env.Append(LIBPATH=env['extra_lib_path'])
env.AppendUnique(CCFLAGS=Split(env['extra_compile_flags']))
env.AppendUnique(LINKFLAGS=env['extra_link_flags'])

BINDIR= os.path.join(env['install_prefix'], env['bindir'])
DATADIR=os.path.join(env['install_prefix'], env['datadir'])

################################################################################

#build_id must be saved *before* it might be set to a fixed date
opts.Save('build/scons-config.py',env)

#This is just a default, don't change it here in the code.
#Use the commandline option 'build_id' instead
if env['build_id']=='':
	env['build_id']='svn'+detect_revision()
print 'Build ID:          '+env['build_id']

config_h=write_configh_header()
do_configure(config_h, conf, env)
write_configh_footer(config_h, env['install_prefix'], BINDIR, DATADIR)
write_buildid(env['build_id'])

env=conf.Finish()

# Pretty output
print

################################################################### Build things

BUILDDIR='build/'+TARGET+'-'+env['build']
Export('env', 'BUILDDIR', 'PhonyTarget', 'simpleglob')

#######################################################################

SConscript('build/SConscript')
SConscript('campaigns/SConscript')
SConscript('doc/SConscript')
SConscript('fonts/SConscript')
SConscript('game_server/SConscript')
SConscript('maps/SConscript')
SConscript('music/SConscript')
SConscript('pics/SConscript')
buildlocale=SConscript('po/SConscript')
SConscript('sound/SConscript')
SConscript('src/SConscript.dist')
thebinary=SConscript('src/SConscript', build_dir=BUILDDIR, duplicate=0)
SConscript('tribes/SConscript')
SConscript('txts/SConscript')
SConscript('utils/SConscript')
SConscript('worlds/SConscript')

Default(thebinary)
if env['build']=='release':
	Default(buildlocale)

########################################################################### tags

S=find('src', '*.h')
S+=find('src', '*.cc')
Alias('tags', env.ctags(source=S, target='tags'))
Default('tags')

################################################################## PNG shrinking

# findfiles takes quite long, so don't execute it if it's unneccessary
if ('shrink' in BUILD_TARGETS):
	print "Assembling file list for image compactification..."
	shrink=env.PNGShrink(find('.', '*.png'))
	Alias("shrink", shrink)

########################################################## Install and uninstall

instadd(env, 'ChangeLog', 'doc')
instadd(env, 'COPYING', 'doc')
instadd(env, 'CREDITS', 'doc')
instadd(env, 'widelands', filetype='binary')

install=env.Install('installtarget', 'build-widelands.sh') # the second argument is a (neccessary) dummy
Alias('install', install)
AlwaysBuild(install)
env.AddPreAction(install, Action(buildlocale))

uninstall=env.Uninstall('uninstalltarget', 'build-widelands.sh') # the second argument is a (neccessary) dummy
Alias('uninstall', uninstall)
Alias('uninst', uninstall)
AlwaysBuild(uninstall)

##################################################################### Distribute

distadd(env, 'ChangeLog')
distadd(env, 'COPYING')
distadd(env, 'CREDITS')
distadd(env, 'Makefile')
distadd(env, 'SConstruct')
distadd(env, 'build-widelands.sh')

dist=env.DistPackage('widelands-'+env['build_id'], 'build-widelands.sh') # the second argument is a (neccessary) dummy
Alias('dist', dist)
AlwaysBuild(dist)

###################################################################### longlines

longlines=PhonyTarget("longlines", 'utils/count-longlines.py')

###################################################################### precommit

#Alias('precommit', 'indent')
Alias('precommit', 'longlines')

################################################################## Documentation

PhonyTarget('doc', 'doxygen doc/Doxyfile')

########################################################################## Clean

distcleanactions=[
	Delete('build/config.log'),
	Delete('build/native-*'),
	Delete('build/scons-config.py'),
#	Delete('build/scons-signatures.dblite')) # This can not work, how to get rid of this file?
	Delete('build/sconf_temp'),
	Delete('build/scons-tools/*.pyc'),
	Delete('locale/??_??'),
	Delete('po/pot'),
	Delete('src/build_id.h'),
	Delete('src/config.h'),
	Delete('tags'),
	Delete('utils/*.pyc'),
	Delete('utils/scons-LICENSE'),
	Delete('utils/scons-README'),
	Delete('utils/scons-local-*'),
	Delete('utils/scons.py'),
	Delete('utils/scons-time.py'),
	Delete('utils/sconsign.py'),
	Delete('widelands'),
]

distclean=PhonyTarget("distclean", distcleanactions)

