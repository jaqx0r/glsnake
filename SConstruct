EnsureSConsVersion(0, 95)
SConsignFile('.sconsign')

env = Environment()


# configure
if not env.GetOption("clean"):
	conf = Configure(env)

	# check for GL
	if conf.CheckLib('GL', 'glLightfv'):
		if conf.CheckCHeader('GL/gl.h'):
			conf.env.AppendUnique(CPPFLAGS=['-DHAVE_GL'])
		else:
			print "GL header file not found!"
			Exit(1)
	else:
		print "GL library not found!"
		Exit(1)
	# check for GLU
	if conf.CheckLib('GLU', 'gluOrtho2D'):
		if conf.CheckCHeader('GL/glu.h'):
			conf.env.AppendUnique(CPPFLAGS=['-DHAVE_GLU'])
		else:
			print "GLU header file not found!"
			Exit(1)
	else:
		print "GLU library not found!"
		Exit(1)
	# check for GLUT
	if conf.CheckLib('glut', 'glutMainLoop'):
		if conf.CheckCHeader('GL/glut.h'):
			conf.env.AppendUnique(CPPFLAGS=['-DHAVE_GLUT'])
		else:
			print "GLUT header file not found!"
			Exit(1)
	else:
		print "GLUT library not found!"
		Exit(1)
	# check for libm
	if conf.CheckLib('m', 'fmod'):
		if conf.CheckCHeader('math.h'):
			conf.env.AppendUnique(CPPFLAGS=['-DHAVE_GLUT'])
		else:
			print "GLUT header file not found!"
			Exit(1)
	else:
		print "GLUT library not found!"
		Exit(1)

	# check whether gettimeofday() exists, and how many arguments it has
	print "Checking for gettimeofday() semantics...",
	if conf.TryCompile("""#include <stdlib.h>
#include <sys/time.h>
int main() {
 struct timeval tv;
 struct timezone tzp;
 gettimeofday(&tv, &tzp);
}
	""", '.c'):
		conf.env.AppendUnique(CPPFLAGS=['-DHAVE_GETTIMEOFDAY',
										'-DGETTIMEOFDAY_TWO_ARGS'])
		print "two args"
	else:
		if conf.TryCompile("""#include <stdlib.h>
		#include <sys/time.h>
		int main() {
		struct timeval tv; gettimeofday(&tv);
		}""", '.c'):
			conf.env.AppendUnique(CPPFLAGS=['-DHAVE_GETTIMEOFDAY'])
			print "one arg"
		else:
			print "unknown"
			print "gettimeofday() has unknown number of arguments"
			Exit(1)
	
# set warning flags
warnings = ['',
			'all',
			'error',
			'aggregate-return',
			'cast-align',
			'cast-qual',
			'nested-externs',
			'shadow',
			'bad-function-cast',
			'write-strings'
			]
env.AppendUnique(CCFLAGS=['-W%s' % (w,) for w in warnings])

glsnake_sources = 'glsnake.c'

glsnake = env.Program('glsnake', glsnake_sources,
					  LIBS=['glut', 'GLU', 'GL', 'm'])
