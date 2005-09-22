EnsureSConsVersion(0, 95)

env = Environment()

# set warning flags
warnings = ['',
			'all',
			'error',
#			'aggregate-return',
#			'cast-align',
#			'cast-qual',
#			'nested-externs',
#			'shadow',
#			'bad-function-cast',
#			'write-strings'
			]
env.AppendUnique(CCFLAGS=['-W%s' % (w,) for w in warnings])

glsnake_sources = 'glsnake.c'

env.AppendUnique(CCFLAGS=['-DHAVE_CONFIG_H'])

glsnake = env.Program('glsnake', glsnake_sources,
					  LIBS=['glut'])
