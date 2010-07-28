# vim:ts=4:sw=4:noet:ft=python:
APPNAME='SoundPatty'
VERSION='0.2'
import os
from subprocess import Popen, PIPE

def set_options(opt):
	opt.tool_options('compiler_cxx')
	opt.add_option('--disable', action='store')

def configure(conf):
	print('→ configuring the project')

	conf.check_cfg(atleast_pkgconfig_version='0.0.0')
	conf.check_cfg(package='jack', args='--libs', uselib_store="JACK")

	conf.check_tool('compiler_cxx')
	conf.write_config_header('config.h')
	#conf.env.COMP = "bac"

def build(bld):
	bld.use_the_magic()

	common_src = 'src/logger.cpp src/soundpatty.cpp src/wavinput.cpp'
	if bld.env.HAVE_JACK:
		common_src += ' src/jackinput.cpp'

	bld(features     = ['cxx', 'cprogram'],
		target       = 'main',
		source		 = 'src/main.cpp '+common_src,
		uselib		= 'JACK',
		cxxflags      = ['-Wall', '-g', '-O2', '-I', 'default/'],
		install_path = '.'
	)
	bld(features     = ['cxx', 'cprogram'],
		target       = 'controller',
		source		 = 'src/controller.cpp '+common_src,
		uselib		= 'JACK',
		cxxflags      = ['-Wall', '-g', '-O2', '-I', 'default/'],
		install_path = '.'
	)

def test(sc):
	import Scripting
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
