# GNOME Session for OpenRC!

This is an OpenRC leader for GNOME Session. It creates a new leader,
ctl tool, as well as some openrc init scripts to make GNOME session
Just Work.

This was intended for use in Gentoo, but for other distributions that
use OpenRC, things should also work just fine and I've taken some time
to ensure it'll work smoothly: as long as you apply the GNOME Session
patch. YMMV, but please don't be shy to reach out over GH issues for
assistance and I may be able to help you! :-)

# Setting up

Before anything, for GNOME 49, you will need to patch your GNOME
Session with commit 419191d3 from my gnome-session fork over at the
GNOME Gitlab. Later this may not be the case...

https://gitlab.gnome.org/swagtoy/gnome-session/-/commit/419191d3897957bd8cd325f2167f3c8663969a13

Now, when you build GNOME Session, you'll need to pass `-Dsystemd=false` like so:

```
$ cd /path/to/gnome-session
### now apply the patch...
$ meson setup build --prefix /usr -Dsystemd=false
$ cd build && ninja && sudo ninja install
```

Now you have a 'partial' build of GNOME Session, but without the
systemd leader and the ctl tool. Pretty useless, right?

Now comes a gdm bit: create the `gdm-greeter` fallback user:

```
$ useradd -rm -d /var/lib/gdm-greeter -G gdm gdm-greeter
```

*(unless you build with userdb support, which you probably haven't yet
as I write this, as stable elogind hasn't rolled over
https://github.com/elogind/elogind/issues/323, though, the fixes may
be backportable now if it works!)*

You may also add `gdm-greeter{-2,-3,-4}` as well, but this is an edge
case for multiseat greeters. Best to wait until userdb support
(properly) lands into elogind (and soon, it appears that other
alternatives for userdb are cropping up!).

OK, now for each user you want to login as, add the `dbus` *user*
service to the boot runlevel (this blocks pam_openrc.so from finishing
before `dbus` is ready, which on some devices may cause gnome-session
to not start):

```
$ rc-update -U add dbus boot
```

Assuming you already installed GNOME 49 and GDM 49, then you can build
the gnome-session-openrc in all its glory!

```
$ cd /path/to/gnome-session-openrc
$ meson setup build --prefix /usr
$ cd build && ninja && sudo ninja install
```

And of course, update your PAMs for GDM if you haven't:

0. Append `-session optional pam_openrc.so` to `/etc/pam.d/gdm-launch-environment`.
0. Add `-session optional pam_openrc.so` before `session include system-local-login` (this is a weird workaround, haven't figured it out yet).

Happy GNOMEing!
