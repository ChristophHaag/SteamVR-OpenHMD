project(
	'steamvr-openhmd', 'cpp',
	default_options : 'cpp_std=c++11',
	version : '0.0.1'
)

sources = [
	'driverlog.cpp',
	'driverlog.h',
	'driver_openhmd.cpp'
]

includes = include_directories('./subprojects/openhmd/include')

openhmd_subproject = subproject('openhmd')
openhmd_lib = openhmd_subproject.get_variable('openhmd_lib')

deps = [
	dependency('threads')
]

steamvr_openhmd_lib = library(
	'driver_openhmd', sources,
	include_directories : includes,
	dependencies : deps,
	link_with: openhmd_lib,
	install : true,
	version : meson.project_version(),
	name_prefix : ''
)

#copyfiles = [
#	'driver.vrdrivermanifest',
#]
#foreach copyfile: copyfiles
#	configure_file(
#		input : copyfile,
#		output : copyfile,
#		configuration : configuration_data()
#	)
#endforeach
