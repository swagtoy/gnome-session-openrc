# OpenRC changes

> [!WARNING]
> Some features used may depend on experimental upstream OpenRC features. As I write this, this isn't the case but it may change.

This is a patch-over of GNOME-session for OpenRC users. It creates a new leader, ctl tool, as well as some init scripts to make GNOME session work.

This was created for Gentoo, as we plan to use this but only apply a patch over the core gnome-session meson files and just drop the new files in. This is because `USE=systemd` implementation should work just fine without any adjustments (and it makes bumping a little easier).

For other distributions that use OpenRC, as far as I'm aware, things should also work just fine. YMMV :-)

# Applying the changes

First, apply the changes:

```sh
$ git clone https://gitlab.gnome.org/GNOME/gnome-session.git
$ rsync -av gnome-session-openrc/ /path/to/gnome-session
```

Now create the `gdm-greeter` user (unless you build with userdb support, which you probably haven't yet as I write this):

```
$ # This has to be /var/lib/gdm-greeter until 
$ useradd -rm -d /var/lib/gdm-greeter -G gdm gdm-greeter
```

You may add `gdm-greeter{-2,-3,-4}` as well but this is an edge case for multiseat greeters. Best to wait until userdb support (properly) lands into elogind.

> [!NOTE]
> The /var/lib/gdm-greeter user must be done until [GNOME/gdm#325](https://gitlab.gnome.org/GNOME/gdm/-/merge_requests/325) gets merged

For each user you want to login as, add the `dbus` *user* service to the boot runlevel (this blocks pam_openrc.so from finishing before `dbus` is ready, and on some devices may cause gnome-session to not start):

```
$ rc-update -U add dbus boot
```

Assuming you already installed GNOME 49 and GDM 49 (you can snag the ones from [my repo](https://github.com/swagtoy/local-ebuilds) for now), then you can build the new gnome-session:

```
$ cd /path/to/gnome-session # Not this repo! The one we applied into!
$ meson setup build --prefix /usr
$ cd build && sudo ninja install
```

And of course, update your PAMs if you haven't:

0. Append `-session optional pam_openrc.so` to `/etc/pam.d/gdm-launch-environment`.
0. Add `-session optional pam_openrc.so` before `session include system-local-login` (this is a weird workaround, haven't figured it out yet).


Happy GNOMEing!
