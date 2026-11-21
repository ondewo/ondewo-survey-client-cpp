# Release History

*****************

## Release ONDEWO SURVEY C++ Client 2.1.1

### New Features

* [[OND236-120]](https://ondewo.atlassian.net/browse/OND236-120) Keycloak authentication in the new header-only `<ondewo/client/keycloak.h>`: `ClientConfig` gains `keycloak_url`, `realm`, `client_id`, `username`, `password`, `token_expiration_in_s`, `verify_ssl` and `refresh_token` (appended last, so positional initialisation keeps its meaning); `ValidateConfig` enforces them all-or-nothing and `ToString` redacts `password` and `refresh_token`.
* [[OND236-120]](https://ondewo.atlassian.net/browse/OND236-120) Offline-token start-up: with `refresh_token` set, `KeycloakTokenProvider` starts with one `grant_type=refresh_token` exchange and never sends a password grant, not even as a fallback. A background thread refreshes 30 s before expiry; `Stop()` is bounded and idempotent.
* [[OND236-120]](https://ondewo.atlassian.net/browse/OND236-120) A weak, credential-keyed (SHA-256) provider registry, a gRPC `MetadataCredentialsPlugin`, `CreateAuthenticatedChannel()` and `AddKeycloakAuthorization()`. The libcurl transport (10 s timeout, TLS verification per request) lives in `<ondewo/client/keycloak_curl.h>`, so only its users link libcurl (`libcurl4-openssl-dev`).
* [[OND236-120]](https://ondewo.atlassian.net/browse/OND236-120) Tests: the cross-SDK Keycloak test catalogue (T01-T62) in `tests/test_keycloak.cc`, plus the bearer header on the wire and the curl transport on loopback; hand-written line coverage stays at 100 %.

### Bug Fixes

* [[OND236-120]](https://ondewo.atlassian.net/browse/OND236-120) Keycloak token refresh: a failed refresh no longer ends the refresh. Every failure kind (a 5xx from a proxy, a DNS or connection failure, a restarting Keycloak, a dead offline session) is retried with full-jitter exponential backoff, 5 s doubling up to 300 s and measured from the moment the failure was recorded; the background refresh thread and the per-call path share that backoff. A success resets the backoff.
* [[OND236-120]](https://ondewo.atlassian.net/browse/OND236-120) A token that is still valid keeps being served when its refresh fails inside the 30 s expiry leeway; only an EXPIRED token raises, and it raises the typed `KeycloakAuthenticationError` whatever failed underneath (transport error, non-2xx status, a body that is not JSON or not a JSON object, a missing `access_token`, a non-numeric `expires_in`), with the original error kept as its cause. A malformed response changes no provider state. Concurrent callers share one in-flight refresh, so an outage costs one token request, not one per call.
* [[OND236-120]](https://ondewo.atlassian.net/browse/OND236-120) A token response without an `access_token` is reported by its field names only, never by its values (a token response is credential material), and a quoted Keycloak error body is cut at 500 characters.
* [[OND236-120]](https://ondewo.atlassian.net/browse/OND236-120) A quoted error body is cut at 500 characters (UTF-8 code points) without splitting a character, and a provider whose `token_expiration_in_s` window ended its refresh is still shared by the registry; only a provider closed with `Stop()` / `Close()` is replaced (new `closed()` accessor).

### Improvements

* [[OND236-120]](https://ondewo.atlassian.net/browse/OND236-120) The same Keycloak behaviour as every ONDEWO SDK (the python SDKs are the reference, ondewo-survey-client-python 2.1.1 ships the same fixes): the token request is bounded by a 10 s timeout, durations use a monotonic clock, the `token_expiration_in_s` window is armed on both start-up paths, each failed refresh logs one WARNING with the attempt number, the retry delay and how long the current token stays valid, and the first success afterwards one INFO. No credential appears in any log line, error or rendering of the configuration.
* Tracking API Version [2.1.0](https://github.com/ondewo/ondewo-survey-api/releases/tag/2.1.0) ( [Documentation](https://ondewo.github.io/ondewo-survey-api/) ), generated with ondewo-proto-compiler 5.17.0: the generated code is unchanged from 2.1.0

*****************

## Release ONDEWO SURVEY C++ Client 2.1.0

### Improvements

* Tracking API Version [2.1.0](https://github.com/ondewo/ondewo-survey-api/releases/tag/2.1.0) ( [Documentation](https://ondewo.github.io/ondewo-survey-api/) )
* Generated with ondewo-proto-compiler 5.17.0 (protoc 3.21.12, Debian trixie). Runtime floors unchanged: protobuf 3.21.12 / grpc 1.51.1.
* The build and the release run in docker, as in ondewo-nlu-client-cpp: CMake, protobuf/gRPC, GoogleTest and `gh` come from the utils image built from `Dockerfile.utils`, the compiler container runs as the invoking user (no `sudo`), the GitHub token is validated before anything is pushed, and the GitHub release is created last, in one `gh release create` with the package archive and its `.sha256` attached. The tag-triggered `release.yml` workflow is removed: CI only tests.

*****************

## Release ONDEWO SURVEY C++ Client 2.0.1

### New Features

* TLS / mutual TLS: new header-only channel helper `ondewo/client/channel.h`, installed beside the generated
  headers and on the exported `ondewo::ondewo_survey_client` target's include path. `ondewo::client::ClientConfig`
  (`host`, `port`, `grpc_cert`, `grpc_client_cert`, `grpc_client_key`, `use_secure_channel`) takes PEM **content**,
  not file paths. `CreateChannel`, `CreateChannelCredentials`, `ValidateConfig` and `DefaultChannelArguments()`
  report errors as `grpc::Status` `INVALID_ARGUMENT`, never as exceptions.
* An empty `grpc_cert` uses the platform trust store. Half a client identity (certificate without key or key
  without certificate) and a plaintext channel with a client identity are refused before gRPC is called. A
  plaintext channel logs a warning on `std::clog` naming `host:port`. No error message renders a PEM, a key or the
  config; `ToString()` / `operator<<` print the client key as `***REDACTED***`. Bare IPv6 hosts are bracketed;
  CRLF PEMs work.
* Default channel arguments match the ONDEWO Python SDKs: keepalive every 30 s during calls with a 20 s timeout,
  `grpc.http2.max_pings_without_data=2`, reconnect backoff capped at 5 s, 2^31-1 byte send/receive message limit.
  Documented gaps: gRPC 1.51 ignores `grpc.http2.ping_timeout_ms`, and no per-method retry policy is configured.
* README: new section "TLS, mutual TLS and certificates" (modes, loading a PEM from a file, test PKI, security
  notes, troubleshooting).

### Improvements

* `tests/test_tls.cc`: real handshakes against in-process servers with a PKI generated by the openssl CLI at test
  time - TLS, mutual TLS, server rejecting a missing or foreign client certificate, wrong CA, system trust store,
  non-PEM CA (error, not crash), CRLF PEMs, server-name override, `[::1]` - plus the refusal, warning, redaction
  and default-argument cases. `make coverage` now also holds `client/` to 100 % line coverage.
* `tests/test_release_notes.sh` (CTest) pins the release-notes slice, the heading spelling, the `*****` separators
  and non-empty notes for the current version.
* The pinned `ondewo-proto-compiler` submodule moves from 5.15.1 to 5.15.2 (its fixes concern the PHP, Rust and
  JavaScript images and the release automation; the C++ generation is unchanged).
* Tracking API Version [2.0.0](https://github.com/ondewo/ondewo-survey-api/releases/tag/2.0.0) ( [Documentation](https://ondewo.github.io/ondewo-survey-api/) )

*****************

## Release ONDEWO SURVEY C++ Client 2.0.0

### New Features

* Initial release of `ondewo-survey-client-cpp`, the C++ gRPC client library for the
  [ONDEWO SURVEY API](https://github.com/ondewo/ondewo-survey-api) (Survey). The stubs are generated
  from the `ondewo-survey-api` submodule by the `ondewo-cpp-proto-compiler` image of
  [ondewo-proto-compiler](https://github.com/ondewo/ondewo-proto-compiler) 5.15.1, using protoc's
  built-in `--cpp_out` for the messages and `grpc_cpp_plugin` for the service stubs.
* Ships as a consumable CMake package: `find_package(ondewo_survey_client CONFIG REQUIRED)` plus
  `target_link_libraries(my_app PRIVATE ondewo::ondewo_survey_client)`. The package installs the generated
  headers under `include/ondewo_survey_client/`, the static archive
  `lib/libondewo_survey_client.a` and `lib/cmake/ondewo_survey_client/`, and re-finds `Protobuf` and
  `gRPC` on the consumer's machine through `find_dependency`.
* `public-api.h` is an umbrella header that includes every generated `*.pb.h` and `*.grpc.pb.h`, so a single
  `#include "public-api.h"` gives access to the whole API surface.
* The generated stub sources are committed to the repository. C++ has no package registry, so a git tag is the
  distribution channel: a plain `git clone` or a CMake `FetchContent_Declare` yields a buildable project with
  no Docker and no code generation step.
* `make build` reproduces the whole pipeline - submodule checkout at the pinned tags, compiler image build,
  code generation and a host-native CMake build - and `make test` verifies that every `.proto` produced a
  header and that the installed CMake package is consumable by a downstream project.

*****************
