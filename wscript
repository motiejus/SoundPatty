# vim:ts=4:sw=4:noet:ft=python:
APPNAME='SoundPatty'
VERSION='0.2'
import os, Scripting
from subprocess import Popen, PIPE

def set_options(opt):
	opt.tool_options('compiler_cxx')
	opt.add_option('--disable', action='store')

def configure(conf):
	print('→ configuring the project')

	conf.check_cfg(atleast_pkgconfig_version='0.0.0')
	conf.check_cfg(package='jack', args='--libs', uselib_store="JACK")
	conf.check_cfg(package='sox', args='--libs', uselib_store="SOX")

	conf.check_tool('compiler_cxx')
	conf.write_config_header('config.h')

def build(bld):
	cxxflags = ['-Wall', '-g', '-I', 'default/']
	uselib_local = []

	bld(features		= ['cxx', 'cstaticlib'],
		source			= 'src/soundpatty.cpp',
		target			= 'soundpatty',
		cxxflags		= cxxflags,
	)
	uselib_local += ['soundpatty']

	bld(features		= ['cxx', 'cstaticlib'],
		source			= 'src/wavinput.cpp',
		target			= 'wavinput',
		cxxflags		= cxxflags,
	)
	uselib_local += ['wavinput']

	bld(features		= ['cxx', 'cstaticlib'],
		source			= 'src/logger.cpp',
		target			= 'logger',
		cxxflags		= cxxflags,
	)
	uselib_local += ['logger']

	uselib = []
	if bld.env.HAVE_JACK:
		bld(features		= ['cxx', 'cstaticlib'],
			source			= 'src/jackinput.cpp',
			target			= 'jackinput',
			cxxflags		= cxxflags,
		)
		uselib_local 		+= ['jackinput']
		uselib 				+= ['JACK']

	if bld.env.HAVE_SOX:
		bld(features		= ['cxx', 'cstaticlib'],
			source			= 'src/fileinput.cpp',
			target			= 'fileinput',
			cxxflags		= cxxflags,
		)
		uselib_local 		+= ['fileinput']
		uselib				+= ['SOX']


	# and the executables
	bld(features		= ['cxx', 'cprogram'],
		source			= 'src/main.cpp',
		target			= 'main',
		uselib			= uselib,
		cxxflags		= cxxflags,
		uselib_local	= uselib_local,
		install_path	= '../',
	)

	if bld.env.HAVE_JACK:
		bld(features		= ['cxx', 'cprogram'],
			source			= 'src/controller.cpp',
			target			= 'controller',
			uselib			= uselib,
			cxxflags		= cxxflags,
			uselib_local	= uselib_local,
			install_path	= '../',
		)

def test(sc):
	Scripting.commands += 'build install proc_test'.split()

def proc_test(sc):
	download("sample.wav",
			"http://github.com/downloads/Motiejus/SoundPatty/sample.wav")
	download("catch_me.wav",
			"http://github.com/downloads/Motiejus/SoundPatty/catch_me.wav")
	from Utils import pprint
	pprint ('CYAN', "Creating sample file...")
	os.system("./main config.cfg sample.wav > samplefile.txt 2>/dev/null")
	pprint ('CYAN', "Launching checker...")
	output = Popen("./main config.cfg samplefile.txt catch_me.wav".split(' '),
			stdout=PIPE, stderr=open('/dev/null', 'w'))
	if output.communicate()[0].find('FOUND') != -1:
		print("Tests passed")
	else:
		from Logs import error
		error("Tests failed")

def download(filename, url):
	if not os.path.exists(filename):
		os.system(' '.join(['wget', "-nv", url, '-O', filename]))
