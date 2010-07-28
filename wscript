# vim:ts=4:sw=4:noet:ft=python:
APPNAME='SoundPatty'
VERSION='0.2'

def set_options(opt):
	opt.tool_options('compiler_cxx')
	opt.add_option('--disable', action='store')

def configure(conf):
	conf.recurse('src')

def build(bld):
	bld.recurse('src')
