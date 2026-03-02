sources = files(
	'leader-openrc.c',
)

executable(
	meson.project_name() + '-init-worker',
	sources,
	include_directories: top_inc,
	dependencies: session_bin_deps,
	install: true,
	install_dir: session_libexecdir
)

sources = files(
	'gnome-session-ctl.c',
)

executable(
	meson.project_name() + '-ctl',
	sources,
	include_directories: top_inc,
	dependencies: session_bin_deps,
	install: true,
	install_dir: session_libexecdir
)

