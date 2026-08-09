project(
  'gnome-session-openrc', 'c',
  version: '49.2',
  license: 'GPL2+',
  meson_version: '>= 0.61.0'
)

session_version = meson.project_version()
session_api_name = '@0@-@1@'.format(meson.project_name(), session_version.split('.')[0])

session_prefix = get_option('prefix')
session_datadir = join_paths(session_prefix, get_option('datadir'))
session_libdir = join_paths(session_prefix, get_option('libdir'))
session_securedir = get_option('securedir') != '' ? get_option('securedir') : join_paths(session_libdir, 'security')
session_libexecdir = join_paths(session_prefix, get_option('libexecdir'))

have_x11 = get_option('x11')

set_defines = [
  # package
  ['PACKAGE', meson.project_name()],
  ['PACKAGE_VERSION', session_version],
  ['VERSION', session_version],
]

glib_req_version = '>= 2.82.0'
librc_dep = dependency('openrc', required: true)
glib_dep = dependency('glib-2.0', version: glib_req_version)
gio_deps = [
  dependency('gio-2.0', version: glib_req_version),
  dependency('gio-unix-2.0', version: glib_req_version),
]

session_bin_deps = [
  glib_dep,
  gio_deps,
  dependency('libelogind', version: '>=209', required: true),
  librc_dep
]

pkg = import('pkgconfig')

top_inc = include_directories('.')

subdir('src')
subdir('data')

summary_dirs = {
  'prefix': get_option('prefix'),
  'datadir': session_datadir,
  'libexecdir': session_libexecdir,
  'securedir': session_securedir,
}

summary(summary_dirs, section: 'Directories')
summary({ 'X11 services': have_x11 }, section: 'Miscellaneous')
