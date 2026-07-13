#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <glib.h>

#include "debug.h"
#include "muditm.h"
#include "skmud/skmud.h"

static char skmud_control_socket[256];

static void skmud_notify(const char *msg) {
	int fd;
	struct sockaddr_un addr;
	char buf[1024];
	int len;

	muditm_log("%s", msg);

	if (!skmud_control_socket[0]) return;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0) return;

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", skmud_control_socket);

	if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		muditm_log("SKMUD control socket not available: %s", skmud_control_socket);
		close(fd);
		return;
	}

	len = snprintf(buf, sizeof(buf), "notify log %s\n", msg);
	if (len >= (int)sizeof(buf)) len = sizeof(buf) - 1;
	write(fd, buf, len);
	close(fd);
}

void skmud_init(GKeyFile *gkf) {
	char *path;

	path = g_key_file_get_string(gkf, "skmud", "control_socket", NULL);
	if (!path) return;

	snprintf(skmud_control_socket, sizeof(skmud_control_socket), "%s", path);
	g_free(path);

	muditm_notify = skmud_notify;
	muditm_log("SKMUD notifications enabled via %s", skmud_control_socket);
}
