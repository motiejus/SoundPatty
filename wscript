# vim:ts=4:sw=4:noet:ft=python:
APPNAME='SoundPatty'
VERSION='0.2'
import os
from subprocess import Popen, PIPE

def set_options(opt):
	opt.tool_options('compiler_cxx')
	opt.add_option('--disable', action='store')

def configure(conf):
	conf.recurse('src')

def build(bld):
	bld.recurse('src')

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
