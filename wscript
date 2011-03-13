# vim:ts=4:sw=4:noet:ft=python:
APPNAME='SoundPatty'
VERSION='1.0beta1'
import os, Scripting
from subprocess import Popen, PIPE

def set_options(opt):
	opt.tool_options('compiler_cxx')

def configure(conf):
	conf.check_tool('compiler_cxx')
	conf.check_cfg(atleast_pkgconfig_version='0.0.0', mandatory=True)
	conf.check_cfg(package='sox', args='--libs', uselib_store='SOX',
			mandatory=True)
	#conf.check_cxx(header_name='sox.h', uselib_store='SOX',
			#compiler_mode='cxx', mandatory=True)
	conf.check_cxx(function_name='inotify_init', header_name='sys/inotify.h',
			compiler_mode='cxx')

	conf.check_cfg(package='jack', args='--libs', uselib_store="JACK")
	conf.write_config_header('config.h')

def build(bld):
	cxxflags = ['-Wall', '-g', '-pedantic', '-Wno-variadic-macros',
				'-I', 'default/']
	sp_source = ['src/main.cpp', 'src/soundpatty.cpp', 'src/fileinput.cpp',\
				 'src/logger.cpp', 'src/input.cpp']
	sp_uselib = ['SOX']
	if bld.env.HAVE_JACK:
		sp_source += ['src/jackinput.cpp']
		sp_uselib += ['JACK']

	# And the runnable executables
	bld(features		= ['cxx', 'cprogram'],
		source			= sp_source,
		target			= 'soundpatty',
		cxxflags		= cxxflags,
		uselib			= sp_uselib,
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
	pprint ('CYAN', "Creating sample file... ", sep="")
	cmd = "./soundpatty -qa dump -c config.cfg sample.wav > samplefile.txt"
	print (cmd)
	os.system(cmd)
	pprint ('CYAN', "Launching checker... ", sep="")
	cmd = ['./soundpatty', '-acapture', '-cconfig.cfg', '-qx',
					'-ssamplefile.txt', 'catch_me.wav']
	print (" ".join(cmd))
	output = Popen(cmd, stdout=PIPE).communicate()[0]
	print (output)
	if output.find('FOUND') != -1:
		print("Tests passed")
	else:
		from Logs import error
		error("Tests failed")

def download(filename, url):
	if not os.path.exists(filename):
		os.system(' '.join(['wget', "-nv", "--no-check-certificate", url, '-O', filename]))
