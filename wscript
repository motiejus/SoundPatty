# vim:ts=4:sw=4:noet:ft=python:
APPNAME='SoundPatty'
VERSION='0.2'
import os, Scripting
from subprocess import Popen, PIPE

def set_options(opt):
	opt.tool_options('compiler_cxx')

def configure(conf):
	conf.check_cfg(atleast_pkgconfig_version='0.0.0')
	conf.check_cfg(package='jack', args='--libs', uselib_store="JACK")
	conf.check_cfg(package='sox', args='--libs', uselib_store="SOX",
			mandatory=True)
	conf.check_tool('compiler_cxx')
	conf.write_config_header('config.h')

def build(bld):
	cxxflags = ['-Wall', '-g', '-O2', '-I', 'default/']
	sp_source = ['src/soundpatty.cpp', 'src/logger.cpp', 'src/fileinput.cpp']
	sp_uselib = ['SOX']
	if bld.env.HAVE_JACK:
		sp_source += ['src/jackinput.cpp']
		sp_uselib += ['JACK']

	# Build SoundPatty static library with required modules (inputs, logger)
	bld(features		= ['cxx', 'cstaticlib'],
		source			= sp_source,
		target			= 'soundpatty',
		cxxflags		= cxxflags,
		uselib			= sp_uselib,
	)
	# And the runnable executables
	bld(features		= ['cxx', 'cprogram'],
		source			= 'src/main.cpp',
		target			= 'main',
		cxxflags		= cxxflags,
		uselib			= sp_uselib,
		uselib_local	= 'soundpatty',
		install_path	= '../',
	)
	# I hope this will not be nescesarry in the future
	if bld.env.HAVE_JACK:
		bld(features		= ['cxx', 'cprogram'],
			source			= 'src/controller.cpp',
			target			= 'controller',
			cxxflags		= cxxflags,
			uselib			= sp_uselib,
			uselib_local	= 'soundpatty',
			install_path	= '../',
		)

def test(sc):
	Scripting.commands += 'install proc_test'.split()

def proc_test(sc):
	download("sample.wav",
			"http://github.com/downloads/Motiejus/SoundPatty/sample.wav")
	download("catch_me.wav",
			"http://github.com/downloads/Motiejus/SoundPatty/catch_me.wav")
	from Utils import pprint
	pprint ('CYAN', "Creating sample file...")
	os.system("./main -qa dump -c config.cfg sample.wav > samplefile.txt")
	pprint ('CYAN', "Launching checker...")
	output = Popen(['./main', '-acapture', '-cconfig.cfg',
					'-ssamplefile.txt', 'catch_me.wav'],
			stdout=PIPE, stderr=open('/dev/null', 'w'))
	if output.communicate()[0].find('FOUND') != -1:
		print("Tests passed")
	else:
		from Logs import error
		error("Tests failed")

def download(filename, url):
	if not os.path.exists(filename):
		os.system(' '.join(['wget', "-nv", url, '-O', filename]))
