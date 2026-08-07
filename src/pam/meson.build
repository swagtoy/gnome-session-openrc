pam_dep = dependency('pam', required: true)
if not pam_dep.found()
    pam_dep = cc.find_library('pam')
endif

pam_dir = session_libdir / 'security'

shared_module('pam_gnome_session_openrc', 'pam_gnome_session_openrc.c',
    name_prefix: '',
    dependencies: [ pam_dep, glib_dep, meson.get_compiler('c').find_library('dl') ],
    install: true,
    install_dir: pam_dir
)
