subdir('pam')

sources = files(
	'leader-openrc.c',
)

executable(
	'gnome-session-init-worker',
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
	'gnome-session-ctl',
	sources,
	include_directories: top_inc,
	dependencies: session_bin_deps,
	install: true,
	install_dir: session_libexecdir
)
