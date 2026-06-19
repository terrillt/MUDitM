/* test_max_children.c — verify max-children connection limiting.
 *
 * Starts a TCP echo server, launches MUDitM with max-children=3,
 * and verifies connections are capped and queued correctly.
 *
 * Build:  cc -o test_max_children tests/test_max_children.c
 * Run:    ./test_max_children [path-to-muditm]
 */

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_CHILDREN  3
#define BACKLOG       2
#define MAX_CONNS     8

static pid_t echo_pid = 0;
static pid_t muditm_pid = 0;
static char conf_path[256];
static char log_path[256];
static int  held_conns[MAX_CONNS];
static int  held_count = 0;

static void cleanup(void) {
	for (int i = 0; i < held_count; i++)
		if (held_conns[i] >= 0) close(held_conns[i]);
	if (muditm_pid > 0) { kill(muditm_pid, SIGTERM); waitpid(muditm_pid, NULL, 0); }
	if (echo_pid > 0) { kill(echo_pid, SIGTERM); waitpid(echo_pid, NULL, 0); }
	if (conf_path[0]) unlink(conf_path);
	if (log_path[0]) unlink(log_path);
}

static int bind_any_port(void) {
	int s = socket(AF_INET, SOCK_STREAM, 0);
	int on = 1;
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	struct sockaddr_in addr = {0};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = 0;
	bind(s, (struct sockaddr *)&addr, sizeof(addr));
	socklen_t len = sizeof(addr);
	getsockname(s, (struct sockaddr *)&addr, &len);
	int port = ntohs(addr.sin_port);
	close(s);
	return port;
}

static void run_echo_server(int port) {
	int s = socket(AF_INET, SOCK_STREAM, 0);
	int on = 1;
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	struct sockaddr_in addr = {0};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = htons(port);
	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) _exit(1);
	listen(s, 16);
	signal(SIGCHLD, SIG_IGN);
	while (1) {
		int c = accept(s, NULL, NULL);
		if (c < 0) continue;
		if (fork() == 0) {
			close(s);
			char buf[4096];
			int n;
			while ((n = read(c, buf, sizeof(buf))) > 0)
				write(c, buf, n);
			close(c);
			_exit(0);
		}
		close(c);
	}
}

static int connect_to(int port, int timeout_ms) {
	int s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) return -1;

	/* set non-blocking for connect timeout */
	int flags = fcntl(s, F_GETFL, 0);
	fcntl(s, F_SETFL, flags | O_NONBLOCK);

	struct sockaddr_in addr = {0};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = htons(port);

	int ret = connect(s, (struct sockaddr *)&addr, sizeof(addr));
	if (ret < 0 && errno == EINPROGRESS) {
		fd_set wset;
		FD_ZERO(&wset);
		FD_SET(s, &wset);
		struct timeval tv = { timeout_ms / 1000, (timeout_ms % 1000) * 1000 };
		if (select(s + 1, NULL, &wset, NULL, &tv) <= 0) {
			close(s);
			return -1;
		}
		int err = 0;
		socklen_t elen = sizeof(err);
		getsockopt(s, SOL_SOCKET, SO_ERROR, &err, &elen);
		if (err != 0) { close(s); return -1; }
	} else if (ret < 0) {
		close(s);
		return -1;
	}

	/* restore blocking */
	fcntl(s, F_SETFL, flags);
	return s;
}

static int test_echo(int sock) {
	const char *msg = "ping\n";
	if (write(sock, msg, strlen(msg)) < 0) return 0;
	usleep(200000);

	char buf[256] = {0};
	struct timeval tv = {2, 0};
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	int n = read(sock, buf, sizeof(buf) - 1);
	return (n > 0 && strstr(buf, "ping") != NULL);
}

static int file_contains(const char *path, const char *needle) {
	FILE *f = fopen(path, "r");
	if (!f) return 0;
	char line[1024];
	while (fgets(line, sizeof(line), f)) {
		if (strstr(line, needle)) { fclose(f); return 1; }
	}
	fclose(f);
	return 0;
}

int main(int argc, char **argv) {
	const char *muditm_bin = (argc > 1) ? argv[1] : "./muditm";
	int pass = 0, fail = 0, total = 4;

	if (access(muditm_bin, X_OK) != 0) {
		printf("SKIP: muditm binary not found at %s\n", muditm_bin);
		return 0;
	}

	atexit(cleanup);

	int proxy_port = bind_any_port();
	int game_port = bind_any_port();

	/* start echo server */
	echo_pid = fork();
	if (echo_pid == 0) {
		run_echo_server(game_port);
		_exit(0);
	}
	usleep(500000);

	/* write config */
	snprintf(conf_path, sizeof(conf_path), "/tmp/muditm_test_%d.conf", getpid());
	snprintf(log_path, sizeof(log_path), "/tmp/muditm_test_%d.log", getpid());
	FILE *cf = fopen(conf_path, "w");
	fprintf(cf,
		"[muditm]\n"
		"demon = true\n"
		"listen = %d\n"
		"listen-backlog = %d\n"
		"max-children = %d\n"
		"log-file = %s\n"
		"\n"
		"[game]\n"
		"host = 127.0.0.1\n"
		"service = %d\n"
		"security = none\n"
		"compression = disable\n"
		"\n"
		"[client]\n"
		"security = none\n"
		"compression = disable\n",
		proxy_port, BACKLOG, MAX_CHILDREN, log_path, game_port);
	fclose(cf);

	/* start muditm */
	muditm_pid = fork();
	if (muditm_pid == 0) {
		execlp(muditm_bin, muditm_bin, "-c", conf_path, NULL);
		_exit(1);
	}
	sleep(1);

	/* Test 1: connections up to the limit succeed */
	printf("Test 1: Open %d connections (should all echo)... ", MAX_CHILDREN);
	fflush(stdout);
	int ok = 1;
	for (int i = 0; i < MAX_CHILDREN; i++) {
		int s = connect_to(proxy_port, 3000);
		if (s < 0 || !test_echo(s)) {
			ok = 0;
			if (s >= 0) close(s);
			break;
		}
		held_conns[held_count++] = s;
	}
	if (ok) { printf("PASS\n"); pass++; }
	else    { printf("FAIL\n"); fail++; }

	/* Test 2: connection beyond limit doesn't get echo (queued or refused) */
	printf("Test 2: Connection %d (should queue, no echo)... ", MAX_CHILDREN + 1);
	fflush(stdout);
	int extra = connect_to(proxy_port, 2000);
	if (extra < 0) {
		printf("PASS (connection refused)\n");
		pass++;
	} else {
		/* connected (in backlog), but send should not get echoed */
		write(extra, "test\n", 5);
		usleep(500000);
		char buf[256] = {0};
		struct timeval tv = {1, 0};
		setsockopt(extra, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		int n = read(extra, buf, sizeof(buf) - 1);
		if (n <= 0) {
			printf("PASS (queued, no echo while at capacity)\n");
			pass++;
		} else {
			printf("FAIL (connection served while at capacity)\n");
			fail++;
		}
	}

	/* Test 3: close one held connection, the queued one should get served */
	printf("Test 3: Close one, queued connection should proceed... ");
	fflush(stdout);
	if (extra >= 0) {
		close(held_conns[0]);
		held_conns[0] = -1;
		sleep(2);
		/* drain any buffered data from the test 2 send */
		struct timeval tv2 = {1, 0};
		setsockopt(extra, SOL_SOCKET, SO_RCVTIMEO, &tv2, sizeof(tv2));
		char drain[256];
		read(extra, drain, sizeof(drain));
		/* send fresh data */
		write(extra, "freed\n", 6);
		usleep(500000);
		char buf[256] = {0};
		int n = read(extra, buf, sizeof(buf) - 1);
		if (n > 0 && strstr(buf, "freed")) {
			printf("PASS\n");
			pass++;
		} else {
			printf("FAIL: expected echo, got '%.*s'\n", n > 0 ? n : 0, buf);
			fail++;
		}
		close(extra);
	} else {
		/* extra never connected — can't test queuing */
		printf("SKIP (connection was refused, can't test queue drain)\n");
		total--;
	}

	/* Test 4: log message */
	printf("Test 4: Capacity log message present... ");
	fflush(stdout);
	usleep(500000);
	if (file_contains(log_path, "At max children")) {
		printf("PASS\n");
		pass++;
	} else {
		printf("FAIL: 'At max children' not in log\n");
		fail++;
	}

	printf("\n%d/%d passed, %d/%d failed\n", pass, total, fail, total);
	return (fail > 0) ? 1 : 0;
}
