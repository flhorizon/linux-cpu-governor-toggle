#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define PERF_STR "performance"
#define POWERSAVE_STR "powersave"
#define SMLEN(str) (sizeof(str) - 1) /* compile-time strlen for literals ONLY */
#define INFO_CMD_DESKTOP "zenity --info --text=\"Perf mode while this dialogue is active...\""

static void success(int s)
{
	exit(s ^ s);
}

static void set_sigs(void)
{
	signal(SIGINT, success);
	signal(SIGHUP, success);
	signal(SIGTERM, success);
	signal(SIGQUIT, success);
	signal(SIGUSR1, success);
	signal(SIGUSR2, success);
}

struct s_xuid
{
	int		ruid;
	int		euid;
	int		ssuid;
};

static void set_gov(const char *s, const int len, const struct s_xuid *as)
{
	FILE *pipe;
	char msg[2048];

	if (setresuid(as->ruid, as->euid, as->ssuid))
	{
		snprintf(msg, sizeof(msg), "%s(%s): setresuid(%d, %d, %d):", __func__, s, as->ruid, as->euid, as->ssuid);
		perror(msg);
		_exit(1);
	}
	pipe = popen("tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor >/dev/null", "w");
	fwrite(s, len, 1, pipe);
	pclose(pipe);
}

static int g_ruid, g_ssuid;

static void exit_handler(void)
{
	struct s_xuid	id = {
		.ruid = g_ssuid,
		.euid = g_ssuid,
		.ssuid = -1
	};

	set_gov(POWERSAVE_STR, SMLEN(POWERSAVE_STR), &id);
}

static void hold_on(void)
{
	int outfd;

	if (isatty(STDIN_FILENO) == 1)
	{
		outfd = dup(STDIN_FILENO);
		dprintf(outfd, "%s\n%s\n"
				, "Settings Perf mode... Press any key to exit & restore Powersave."
				, "(terminating signals will also restore Powersave)");
		(void)getchar();
		close(outfd);
	}
	else
	{
		system(INFO_CMD_DESKTOP);
	}
}

int		main(int argc, char **argv)	
{
	struct	s_xuid	id;

	(void)argc;
	(void)argv;
	/* popen seems to drop euid & saved-setuid */
	getresuid(&g_ruid, NULL, &g_ssuid);
	set_sigs();				/* Regular killer sigs => clean exit */
	atexit(&exit_handler);	/* Restore powersave profile at exit */
	/* Enter perf mode */
	id.ruid = g_ssuid;
	id.euid = g_ssuid;
	id.ssuid = -1;
	set_gov(PERF_STR, SMLEN(PERF_STR), &id);
	/* Drop privileges while waiting ; keep saved-setuid */
	if (setresuid(g_ruid, g_ruid, -1))
	{
		perror("setresuid ruid");
		return 1;
	}
	hold_on();
	return 0;
}
