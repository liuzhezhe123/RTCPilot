#ifndef DTLS_SESSION_HPP
#define DTLS_SESSION_HPP
#include "utils/logger.hpp"
#include "utils/timer.hpp"
#include "udp_transport.hpp"
#include "srtp_session.hpp"
#include "net/udp/udp_pub.hpp"
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <map>
#include <vector>

namespace cpp_streamer {

typedef enum
{
	ROLE_NONE = 0,
	ROLE_AUTO = 1,
	ROLE_CLIENT,
	ROLE_SERVER
} Role;

typedef enum
{
	ALGORITHM_NONE = 0,
	ALGORITHM_SHA1 = 1,
	ALGORITHM_SHA224,
	ALGORITHM_SHA256,
	ALGORITHM_SHA384,
	ALGORITHM_SHA512
} FingerprintAlgorithm;

long OnSslBioOut(BIO* bio, int operationType, const char* argp, size_t len, int /*argi*/, long /*argl*/, int ret, size_t* /*processed*/);

class DtlsSession;
class DtlsWriteCallbackI
{
public:
    virtual void OnDtlsTransportSendData(
        const uint8_t* data, size_t sent_size, UdpTuple address) = 0;
    virtual void OnDtlsTransportConnected(
      const DtlsSession* /*dtlsTransport*/,
      SRtpSessionCryptoSuite srtp_crypto_suite,
      uint8_t* srtp_local_key,
      size_t srtp_local_key_len,
      uint8_t* srtp_remote_key,
      size_t srtp_remote_key_len,
      std::string& remote_cert) = 0;
};

class Fingerprint
{
public:
    Fingerprint() = default;
    ~Fingerprint() = default;

    std::string ToString() const;
public:
	FingerprintAlgorithm algorithm{ FingerprintAlgorithm::ALGORITHM_NONE };
	std::string value;
};

struct SRtpCryptoSuiteMapEntry
{
	SRtpSessionCryptoSuite cryptoSuite;
	const char* name;
};

class DtlsSession : public TimerInterface
{
friend long OnSslBioOut(BIO* bio, int operationType, const char* argp, size_t len, int /*argi*/, long /*argl*/, int ret, size_t* /*processed*/);
friend void OnSslMsgCallback(int write_p, int version, int content_type,
							 const void* buf, size_t len, SSL* ssl, void* arg);

public:
    DtlsSession(DtlsWriteCallbackI* transport, Logger* logger);
    ~DtlsSession();

public:
    int SetRemoteFingerprint(const std::string& fingerprint);
    Fingerprint GetRemoteFingerprint() { return remote_fp_; }
    void SetRole(Role role);
    Role GetRole() { return role_; }
	int InitSession();
	int Run();
	int OnHandleDtlsData(const uint8_t* data, size_t len, UdpTuple addr);
	bool CheckStatus(int ret);
    bool GetIceConnected() const { return ice_connected_; }
    void SetIceConnected(bool connected) { ice_connected_ = connected; }
    void SetRemoteAddress(const UdpTuple& addr) { dtls_remote_addr_ = addr; }
    UdpTuple GetRemoteAddress() { return dtls_remote_addr_; }
    bool GetDtlsConnected() const { return dtls_connected_; }
    void SetDtlsConnected(bool connected) { dtls_connected_ = connected; }

public:
    void OnSslInfo(int where, int ret);

public:
    static int Init(const std::string& cert_file, const std::string& key_file);
    static void CleanupGlobal();
	static bool IsDtlsData(const uint8_t*, size_t len);
	static FingerprintAlgorithm GetFingerprintAlgorithm(const std::string& fingerprint);
	static std::string& GetFingerprintAlgorithmString(FingerprintAlgorithm fingerprint);
    static std::vector<Fingerprint> GetLocalFingerprints() { return DtlsSession::local_fingerprints_; }
    static Fingerprint GetLocalFingerprint(FingerprintAlgorithm algorithm);
    
protected:
    virtual bool OnTimer() override;

private:
    static X509* certificate_;
    static EVP_PKEY* private_key_;
    static SSL_CTX* ssl_ctx_;
    static uint8_t ssl_read_buffer_[];
    static std::map<std::string, Role> string2role_;
    static std::map<std::string, FingerprintAlgorithm> string2fingerprint_algorithm_;
    static std::map<FingerprintAlgorithm, std::string> fingerprint_algorithm2string_;
    static std::vector<Fingerprint> local_fingerprints_;
    static std::vector<SRtpCryptoSuiteMapEntry> srtp_crypto_suites_;

private:
	void SendDtlsMemData();
	bool ProcessHandshake();
	bool GenRemoteCertByRemoteFingerprint();
	SRtpSessionCryptoSuite GenSslSrtpCryptoSuite();
	void GenSrtpKeys(SRtpSessionCryptoSuite crypto_suite);

private:
	DtlsWriteCallbackI* transport_ = nullptr;
    Logger* logger_;
    Fingerprint local_fp_;
    Fingerprint remote_fp_;
    Role role_ = Role::ROLE_SERVER;
	UdpTuple dtls_remote_addr_;

private:
	SSL* ssl_ = nullptr;
	BIO* ssl_bio_from_network_ = nullptr;
	BIO* ssl_bio_to_network_ = nullptr;
	bool handshake_done_ = false;
	std::string remote_cert_;

private:
    bool ice_connected_ = false;//stun packet received
    bool dtls_connected_ = false;//dtls handshake done
    int64_t last_dtls_send_ms_ = -1;
};

} // namespace cpp_streamer

#endif