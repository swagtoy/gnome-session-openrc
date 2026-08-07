#define _GNU_SOURCE
#include <dlfcn.h>
#include <glib.h>
#include <glib-unix.h>
#include <syslog.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <stdbool.h>
#include <security/pam_appl.h>
#include <security/pam_modules.h>

void (*dyn_rc_set_user)(void);
bool (*dyn_rc_service_add)(char const*, char const*);

/* We need a way to start dbus early, before our
   gnome-session-init-worker starts up (so after PAM cycles through),
   otherwise, systems will struggle. The proper solution is to add
   dbus to the boot user runlevel, that way, pam_openrc.so can chew on
   it and get everything started up.

   Now, pardon my struggles, but I would've _preferred_ to start dbus
   on a whim instead. However, I have had very little success in doing
   that, so... we still add dbus to the boot user runlevel, but what's
   nice, is that now we don't have to force users to do this by hand,
   and avoid any broken setups entirely. :)

   Now, we may also do something like this in leader-openrc.c,
   however, this would be after the PAM phase, and this would mean
   that the users first experience would be inconvenient, and it's
   likely they'd be confused that 'restarting' just fixes it, since we
   performed this step after pam_openrc.so, where the dbus service
   would block and start.

   So, the overwhelming bulk of this code is just to do that, but
   without the user having to do it themselves. :S */

static void pls_dlclose(void **obj) { dlclose(*obj); }

PAM_EXTERN int
pam_sm_open_session(pam_handle_t *pamh, int flags, int argc, char const **argv)
{
	(void)argc; (void)argv;
	/* we must dlmopen librc.so and give it it's own address space,
	   since pam_openrc.so is already loaded at this point by PAM (and
	   thus, librc.so is referenced already).  if we don't, then
	   functions like rc_set_user(), which internally flip some static
	   variables within librc, will end up persisting to
	   pam_openrc.so, as librc.so would still be loaded... :/ thanks... */
	__attribute__((cleanup(pls_dlclose))) void *librc =
		dlmopen(LM_ID_NEWLM, "/usr/lib64/librc.so", RTLD_NOW | RTLD_LOCAL);
	if (!librc)
		return PAM_SESSION_ERR;

	dyn_rc_set_user = dlsym(librc, "rc_set_user");
	if (!dyn_rc_set_user) return PAM_SESSION_ERR;
	dyn_rc_service_add = dlsym(librc, "rc_service_add");
	if (!dyn_rc_service_add) return PAM_SESSION_ERR;
	char const *username = NULL;
	char *conf_dir = NULL;

#define GETENVDUP(var, env) char *var = getenv(env); if (var) var = strdup(var)
	GETENVDUP(old_home, "HOME");
	GETENVDUP(old_xdg_config_home, "XDG_CONFIG_HOME");
	GETENVDUP(old_xdg_runtime_dir, "XDG_RUNTIME_DIR");

	pam_get_item(pamh, PAM_USER, (const void**)&username);

	struct passwd *user = getpwnam(username);
	if (!user)
		return PAM_SESSION_ERR;

	conf_dir = g_strdup_printf("%s/.config", user->pw_dir);
	setenv("HOME", user->pw_dir, 1);
	setenv("XDG_CONFIG_HOME", conf_dir, 1);
	g_autofree char *xdg_runtime_dir = g_strdup_printf("/run/user/%d", user->pw_uid);
	setenv("XDG_RUNTIME_DIR", xdg_runtime_dir, 1);
	dyn_rc_set_user();

	// we don't bother checking error codes for most of these, since
	// they're usually a sign that everything exists anyway.
	g_autofree char *dirs_rc = g_strdup_printf("%s/rc", conf_dir);
	g_autofree char *dirs_rc_runlevels = g_strdup_printf("%s/rc/runlevels", conf_dir);
	g_autofree char *dirs_rc_runlevels_boot = g_strdup_printf("%s/rc/runlevels/boot", conf_dir);
	g_mkdir_with_parents(dirs_rc_runlevels_boot, 0755);
	// this is probably overprecautious...
	lchown(dirs_rc, user->pw_uid, -1);
	lchown(dirs_rc_runlevels, user->pw_uid, -1);
	lchown(dirs_rc_runlevels_boot, user->pw_uid, -1);
	// everything we've worked for, just to do this...
	dyn_rc_service_add("boot", "dbus");
	g_autofree char *dbus_file = g_strdup_printf("%s/dbus", dirs_rc_runlevels_boot);
	lchown(dbus_file, user->pw_uid, -1);

#define SETUNSET(var, env) if (var) { setenv(env, var, 1); } else unsetenv(env)
	SETUNSET(old_home, "HOME");
	SETUNSET(old_xdg_config_home, "XDG_CONFIG_HOME");
	SETUNSET(old_xdg_runtime_dir, "XDG_RUNTIME_DIR");
#undef SETUNSET
	// set by rc_set_user();
	unsetenv("RC_USER_SERVICES");

	dlclose(librc);

	return PAM_SUCCESS;
}

PAM_EXTERN int
pam_sm_close_session(pam_handle_t *pamh, int flags, int argc, char const **argv)
{
	return PAM_SUCCESS;
}
