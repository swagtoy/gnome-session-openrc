# General TODOs / Known issues / What nots

- Env variables are a mess currently, but this is partially due to
  OpenRC's lack of environment variable sharing to the gnome session
  process. However this has very recently been merged in.. need to
  play with it!

- GNOME Session's donation nag may appear on the login screen; this is
  because not all services need to be started on the login screen
  anyway, so need to polish that.
