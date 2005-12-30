import SCons.Builder
import SCons.Action

def complain_ctags(target, source, env):
	print 'INFORMATION: ctags binary was not found (see above). Tags have not been built.'

def generate(env):
	env['CTAGS']=find_ctags(env)

	if env['CTAGS']!=None:
		env['CTAGSCOM']='$CTAGS $SOURCES'
		env['BUILDERS']['ctags']=SCons.Builder.Builder(action=env['CTAGSCOM'])
	else:
		env['BUILDERS']['ctags']=SCons.Builder.Builder(action=env.Action(complain_ctags))

def find_ctags(env):
	b=env.WhereIs('ctags')
	if b==None:
		print 'WARNING: Could not find ctags. Tags will not be built.'
	else:
		print 'Found ctags: ', b
	return b

def exists(env):
	if find_ctags(env)==None:
		return 0
	return 1

