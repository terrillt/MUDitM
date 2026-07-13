#ifndef MUDITM_CERTCHECK_H
#define MUDITM_CERTCHECK_H

void check_cert_expiry(char *cert_file);
void check_cert_expiry_throttled(char *cert_file);

#endif /* MUDITM_CERTCHECK_H */
