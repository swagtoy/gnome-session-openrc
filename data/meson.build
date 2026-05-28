openrc_target = '''
  openrc/gnome-session-wayland
  openrc/gnome-shell-wayland
  openrc/gnome-shell-wayland.gnome-login
  openrc/gnome-session-init-wayland
  openrc/gnome-session-init-x11
  openrc/gnome-session-shutdown
  openrc/gnome-session-monitor
  openrc/gnome-settings-daemon-wayland
  openrc/gnome-session-dbus
  openrc/gsd
  openrc/gsd-xsettings
'''.split()

if have_x11
  openrc_target += '''
    openrc/gnome-session-x11
    openrc/gnome-shell-x11
    openrc/gnome-shell-x11.gnome-login
    openrc/gnome-settings-daemon-x11
  '''.split()
endif

openrc_gsd = '''
  gsd-a11y-settings
  gsd-color
  gsd-datetime
  gsd-housekeeping
  gsd-keyboard
  gsd-media-keys
  gsd-power
  gsd-print-notifications
  gsd-rfkill
  gsd-screensaver-proxy
  gsd-sharing
  gsd-smartcard
  gsd-sound
  gsd-usb-protection
  gsd-wacom
  gsd-wwan
'''.split()

# Install the primary services
install_data(
  openrc_target,
  install_dir: '/etc/user/init.d'
)

# Create symlinks of gsd daemons
foreach gsd : openrc_gsd
  install_symlink(
    gsd,
    install_dir: '/etc/user/init.d',
    pointing_to: 'gsd'
  )
endforeach

# Install the wrapper script
install_data(
  'gnome-session-service-wrapper',
  install_dir: session_libexecdir
)

# Create services like gnome-session.{gnome,gnome-login,...}
gnome_sessions = [ 'gnome', 'gnome-login', 'gnome-initial-setup' ]
targets = [
  'gnome-session-wayland',
  'gnome-session-dbus',
]

if have_x11
  targets += [ 'gnome-session-x11' ]
endif

foreach session : gnome_sessions
  foreach target : targets
    install_symlink(
      target + '.' + session,
      install_dir: '/etc/user/init.d',
      pointing_to: target
    )
  endforeach
endforeach
