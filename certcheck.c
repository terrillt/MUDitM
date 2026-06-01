#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>

#include "debug.h"
#include "muditm.h"
#include "certcheck.h"

static int cert_days_remaining(char *cert_file) {
	FILE *fp;
	X509 *cert;
	int days, seconds;

	fp = fopen(cert_file, "r");
	if (!fp) {
		muditm_log("Cannot open certificate file: %s", cert_file);
		return -1;
	}

	cert = PEM_read_X509(fp, NULL, NULL, NULL);
	fclose(fp);
	if (!cert) {
		muditm_log("Cannot parse certificate: %s", cert_file);
		return -1;
	}

	if (!ASN1_TIME_diff(&days, &seconds, NULL, X509_get0_notAfter(cert))) {
		X509_free(cert);
		muditm_log("Cannot read certificate expiry date: %s", cert_file);
		return -1;
	}
	X509_free(cert);

	if (days < 0 || (days == 0 && seconds < 0))
		return -2;
	return days;
}

static void cert_throttle_path(char *cert_file, char *out, size_t size) {
	snprintf(out, size, "%s", cert_file);
	char *slash = strrchr(out, '/');
	if (slash) {
		size_t prefix = (size_t)(slash - out + 1);
		if (prefix < size)
			snprintf(slash + 1, size - prefix, ".cert-expiry-warned");
	} else {
		snprintf(out, size, ".cert-expiry-warned");
	}
}

static void cert_throttle_touch(char *cert_file) {
	char path[PATH_MAX];
	cert_throttle_path(cert_file, path, sizeof(path));
	int fd = open(path, O_CREAT|O_WRONLY|O_TRUNC, 0644);
	if (fd >= 0) {
		if (write(fd, "x", 1) != 1)
			muditm_log("Cannot write throttle file: %s", path);
		close(fd);
	}
}

void check_cert_expiry(char *cert_file) {
	int days = cert_days_remaining(cert_file);
	if (days == -1) return;

	if (days == -2) {
		muditm_notify("ERROR: TLS certificate has expired!");
		exit(EXIT_FAILURE);
	} else if (days == 0) {
		muditm_notify("WARNING: TLS certificate expires today!");
		cert_throttle_touch(cert_file);
	} else if (days < 30) {
		char notify_buf[128];
		snprintf(notify_buf, sizeof(notify_buf),
			"WARNING: TLS certificate expires in %d day%s!",
			days, days == 1 ? "" : "s");
		muditm_notify(notify_buf);
		cert_throttle_touch(cert_file);
	}
}

void check_cert_expiry_throttled(char *cert_file) {
	int days = cert_days_remaining(cert_file);
	if (days == -1 || days >= 30) return;

	char path[PATH_MAX];
	cert_throttle_path(cert_file, path, sizeof(path));

	int fd = open(path, O_CREAT|O_RDWR, 0644);
	if (fd < 0) return;

	if (flock(fd, LOCK_EX|LOCK_NB) < 0) {
		close(fd);
		return;
	}

	struct stat st;
	time_t now = time(NULL);
	int interval = (days < 7) ? 3600 : 86400;

	if (fstat(fd, &st) == 0 && st.st_size > 0) {
		time_t diff = now - st.st_mtime;
		if (diff >= 0 && diff < interval) {
			flock(fd, LOCK_UN);
			close(fd);
			return;
		}
	}

	if (days == -2) {
		muditm_notify("ALERT: TLS certificate has expired!");
	} else if (days == 0) {
		muditm_notify("WARNING: TLS certificate expires today!");
	} else {
		char notify_buf[128];
		snprintf(notify_buf, sizeof(notify_buf),
			"WARNING: TLS certificate expires in %d day%s!",
			days, days == 1 ? "" : "s");
		muditm_notify(notify_buf);
	}

	ftruncate(fd, 0);
	lseek(fd, 0, SEEK_SET);
	if (write(fd, "x", 1) != 1)
		muditm_log("Cannot write throttle file");
	flock(fd, LOCK_UN);
	close(fd);
}
