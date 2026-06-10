/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2006 Novell, Inc.
 * Copyright (C) 2008 Red Hat, Inc.
 * Copyright (C) 2026 Hyland B. <me@ow.swag.toys>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>

#include <unistd.h>
#include <glib.h>
#include <glib-unix.h>
#include <gio/gio.h>
#include <sys/syslog.h>
#include <rc.h>

typedef struct {
        GDBusConnection *session_bus;
        GMainLoop *loop;
        int fifo_fd;
} Leader;

static void
leader_clear (Leader *ctx)
{
        g_clear_object (&ctx->session_bus);
        g_clear_pointer (&ctx->loop, g_main_loop_unref);
        g_close (ctx->fifo_fd, NULL);
}

G_DEFINE_AUTO_CLEANUP_CLEAR_FUNC (Leader, leader_clear);

static gboolean
async_run_cmd (gchar **argv, GError **error)
{
        return g_spawn_async(NULL,
                             argv,
                             NULL,
                             G_SPAWN_DEFAULT,
                             NULL,
                             NULL,
                             NULL,
                             error);
}

static gboolean
leader_term_or_int_signal_cb (gpointer data)
{
        Leader *ctx = data;

        g_debug ("Session termination requested");

        if (write (ctx->fifo_fd, "S", 1) < 0) {
                g_warning ("Failed to signal shutdown to monitor: %m");
                g_main_loop_quit (ctx->loop);
        }

        return G_SOURCE_REMOVE;
}

static gboolean
monitor_hangup_cb (int          fd,
                   GIOCondition condition,
                   gpointer     user_data)
{
        g_main_loop_quit (((Leader *)user_data)->loop);
        return G_SOURCE_REMOVE;
}

static void
debug_logger (gchar const *log_domain,
              GLogLevelFlags log_level,
              gchar const *message,
              gpointer user_data)
{
        printf ("%s\n", message);
        syslog (LOG_INFO, "%s", message);
}

/* There is a lengthy comment in gnome-session/leader-systemd.c that
   explains how this all works; I've removed it here for brevity */
int
main (int argc, char **argv)
{
        // Hook into syslog, as on an openrc system it's probably more convenient
        g_log_set_default_handler(debug_logger, NULL);
        g_autoptr (GError) error = NULL;
        g_auto (Leader) ctx = { .fifo_fd = -1 };
        const char *session_name = NULL;
        const char *debug_string = NULL;
        g_autofree char *target = NULL;
        g_autofree char *fifo_path = NULL;
        g_autofree char *home_dir = NULL;
        g_autofree char *config_dir = NULL;
        g_autofree char *pid_path = NULL;
        g_autofree char *pid_str = NULL;
        struct stat statbuf;

        if (argc < 2)
            g_error ("No session name was specified");
        session_name = argv[1];

        // probably not rely on this
        char const *user = g_getenv("USER");
        if (!user)
                user = "gdm-greeter"; // :/
        g_info("User is: %s", user);

        // strncmp because we also have gdm-greeter-{2,3,4,...}
        if (strncmp(user, "gdm-greeter", sizeof("gdm-greeter")) == 0)
        {
                home_dir = g_strdup_printf("/var/lib/%s", user);

                // Need to hijack the home to point to /var/lib because /var/run/... gets nuked on each gdm start
                config_dir = g_strdup_printf("%s/.config", home_dir);
                g_setenv("XDG_CONFIG_HOME", config_dir, TRUE);
                g_setenv("HOME", home_dir, TRUE);
        }

        // Finally, let's get started
        rc_set_user();

        char const *home         = g_getenv("HOME");
        g_debug("XDG_RUNTIME_DIR: %s", g_getenv("XDG_RUNTIME_DIR"));

        // TODO what about custom XDG config directory?
        g_autofree char *gnome_runlevel_dir = g_strdup_printf("%s/.config/rc/runlevels/gnome-session", home);
        if (!g_mkdir_with_parents(gnome_runlevel_dir, 0755))
                g_debug("Directory exists. OK");

        g_debug("runlevel dir: %s", gnome_runlevel_dir);

        debug_string = g_getenv ("GNOME_SESSION_DEBUG");
        if (debug_string != NULL)
                g_log_set_debug_enabled (atoi (debug_string) == 1);
        g_debug("Hello from leader-openrc!");

        ctx.loop = g_main_loop_new (NULL, TRUE);

        ctx.session_bus = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
        if (ctx.session_bus == NULL)
                g_error ("Failed to obtain session bus: %s", error->message);

        target = g_strdup_printf ("gnome-session.%s", session_name);

        RC_SERVICE state = rc_service_state(target);
        switch (state)
        {
        case RC_SERVICE_STARTED:
        case RC_SERVICE_FAILED:
                g_error("Service manager is already running!");
                break;
        case RC_SERVICE_STOPPED:
                break;
        default:
                g_debug("Service in state: %d", state);
        }

        if (!rc_runlevel_stack("gnome-session", "default"))
                g_info("Couldn't set runlevel stack");
        if (!rc_runlevel_exists("gnome-session"))
                g_info("No runlevel \"gnome-session\" seen!"); // next function will fail now, but librc error reporting sucks so we check this specifically
        if (!rc_service_add("gnome-session", target))
        {
                g_info("Couldn't add service to gnome-session runlevel: %s", strerror(errno));
        }

        g_message ("Starting GNOME session target: %s", target);

        // This is a hack. We want to store the pid of this process in
        // a predictable path for the openrc scripts. We do this
        // because we later get at /proc/{whatami}/environ. Later this
        // shouldn't be the case, but it is for now.
        pid_t whatami = getpid();
        pid_path = g_build_filename (g_get_user_runtime_dir (),
                                     "gnome-session-leader.pid",
                                     NULL);
        g_remove(pid_path);
        pid_str = g_strdup_printf("%i", whatami);
        if (!g_file_set_contents(pid_path, pid_str, -1, &error))
        {
                g_error("Couldn't write PID to file %s: %s", pid_path, error->message);
        }

        // No way that i'm aware of to enter a user runlevel from librc :/
        gchar *rl_argv[] = { "/sbin/openrc", "-U", "gnome-session", NULL };
        if (!async_run_cmd(rl_argv, &error))
                g_error("Failed to start unit %s: %s", target, error ? error->message : "(no message)");

        // fifo bits for ctl
        fifo_path = g_build_filename (g_get_user_runtime_dir (),
                                      "gnome-session-leader-fifo",
                                      NULL);
        if (mkfifo (fifo_path, 0666) < 0 && errno != EEXIST)
                g_warning ("Failed to create leader FIFO: %m");

        ctx.fifo_fd = g_open (fifo_path, O_WRONLY | O_CLOEXEC, 0666);
        if (ctx.fifo_fd < 0)
                g_error ("Failed to watch openrc session: open failed: %m");
        if (fstat (ctx.fifo_fd, &statbuf) < 0)
                g_error ("Failed to watch openrc session: fstat failed: %m");
        else if (!(statbuf.st_mode & S_IFIFO))
                g_error ("Failed to watch openrc session: FD is not a FIFO");

        g_unix_fd_add (ctx.fifo_fd, G_IO_HUP, (GUnixFDSourceFunc) monitor_hangup_cb, &ctx);
        g_unix_signal_add (SIGHUP, leader_term_or_int_signal_cb, &ctx);
        g_unix_signal_add (SIGTERM, leader_term_or_int_signal_cb, &ctx);
        g_unix_signal_add (SIGINT, leader_term_or_int_signal_cb, &ctx);

        g_main_loop_run (ctx.loop);
        return 0;
}
