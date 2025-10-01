# OpenRC changes

*Some features used may depend on experimental upstream OpenRC features.*

This is a patch-over of GNOME-session for OpenRC users. It creates a new leader, ctl tool, as well as some init scripts to make GNOME session work.

This was created for Gentoo, as we plan to use this but only apply a patch over the core gnome-session meson files and just drop the new files in. This is because `USE=systemd` implementation should work just fine without any adjustments (and it makes bumping a little easier).

For other distributions that use OpenRC, as far as I'm aware, things should also work just fine. YMMV :-)

# Applying the changes

```sh
$ git clone https://gitlab.gnome.org/GNOME/gnome-session.git
$ rsync -av gnome-session-openrc/ /path/to/gnome-session
```
