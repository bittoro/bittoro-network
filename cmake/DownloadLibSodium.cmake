set(LIBSODIUM_PREFIX ${CMAKE_BINARY_DIR}/libsodium)
set(LIBSODIUM_SRC ${LIBSODIUM_PREFIX}/libsodium-1.0.17)
set(LIBSODIUM_TARBALL ${LIBSODIUM_PREFIX}/libsodium-1.0.17.tar.gz)
set(LIBSODIUM_URL https://github.com/jedisct1/libsodium/releases/download/1.0.17/libsodium-1.0.17.tar.gz)
if(SODIUM_TARBALL_URL)
    # make a build time override of the tarball url so we can fetch it if the original link goes away
    set(LIBSODIUM_URL ${SODIUM_TARBALL_URL})
endif()
set(SODIUM_PRETEND_TO_BE_CONFIGURED ON)
file(DOWNLOAD
    ${LIBSODIUM_URL}
    ${LIBSODIUM_TARBALL}
    EXPECTED_HASH SHA512=7cc9e4f11e656008ce9dff735acea95acbcb91ae4936de4d26f7798093766a77c373e9bd4a7b45b60ef8a11de6c55bc8dcac13bebf8c23c671d0536430501da1
    SHOW_PROGRESS)

execute_process(COMMAND tar -xzf ${LIBSODIUM_TARBALL} -C ${LIBSODIUM_PREFIX})
if(WIN32)
  message("patch -p0 -d ${LIBSODIUM_SRC} < ${CMAKE_SOURCE_DIR}/llarp/win32/libsodium-1.0.17-win32.patch")
  execute_process(COMMAND "patch -p0 -d ${LIBSODIUM_SRC} < ${CMAKE_SOURCE_DIR}/llarp/win32/libsodium-1.0.17-win32.patch")
endif()
add_library(sodium_vendor
    ${LIBSODIUM_SRC}/src/libsodium/crypto_aead/aes256gcm/aesni/aead_aes256gcm_aesni.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_aead/chacha20poly1305/sodium/aead_chacha20poly1305.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_aead/xchacha20poly1305/sodium/aead_xchacha20poly1305.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_auth/crypto_auth.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_auth/hmacsha256/auth_hmacsha256.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_auth/hmacsha512/auth_hmacsha512.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_auth/hmacsha512256/auth_hmacsha512256.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_box/crypto_box.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_box/crypto_box_easy.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_box/crypto_box_seal.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_box/curve25519xsalsa20poly1305/box_curve25519xsalsa20poly1305.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_core/ed25519/ref10/ed25519_ref10.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_core/ed25519/ref10/fe_25_5/base.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_core/ed25519/ref10/fe_25_5/base2.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_core/ed25519/ref10/fe_25_5/constants.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_core/ed25519/ref10/fe_25_5/fe.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_core/ed25519/ref10/fe_51/base.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_core/ed25519/ref10/fe_51/base2.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_core/ed25519/ref10/fe_51/constants.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_core/ed25519/ref10/fe_51/fe.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_core/hchacha20/core_hchacha20.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_core/hsalsa20/core_hsalsa20.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_core/hsalsa20/ref2/core_hsalsa20_ref2.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_core/salsa/ref/core_salsa_ref.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_generichash/blake2b/generichash_blake2.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_generichash/blake2b/ref/blake2.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_generichash/blake2b/ref/blake2b-compress-avx2.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_generichash/blake2b/ref/blake2b-compress-avx2.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_generichash/blake2b/ref/blake2b-compress-ref.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_generichash/blake2b/ref/blake2b-compress-sse41.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_generichash/blake2b/ref/blake2b-compress-sse41.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_generichash/blake2b/ref/blake2b-compress-ssse3.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_generichash/blake2b/ref/blake2b-compress-ssse3.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_generichash/blake2b/ref/blake2b-load-avx2.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_generichash/blake2b/ref/blake2b-load-sse2.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_generichash/blake2b/ref/blake2b-load-sse41.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_generichash/blake2b/ref/blake2b-ref.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_generichash/blake2b/ref/generichash_blake2b.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_generichash/crypto_generichash.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_hash/crypto_hash.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_hash/sha256/cp/hash_sha256_cp.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_hash/sha256/hash_sha256.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_hash/sha512/cp/hash_sha512_cp.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_hash/sha512/hash_sha512.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_kdf/blake2b/kdf_blake2b.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_kdf/crypto_kdf.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_kx/crypto_kx.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_onetimeauth/crypto_onetimeauth.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_onetimeauth/poly1305/donna/poly1305_donna.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_onetimeauth/poly1305/donna/poly1305_donna.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_onetimeauth/poly1305/donna/poly1305_donna32.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_onetimeauth/poly1305/donna/poly1305_donna64.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_onetimeauth/poly1305/onetimeauth_poly1305.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_onetimeauth/poly1305/onetimeauth_poly1305.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_onetimeauth/poly1305/sse2/poly1305_sse2.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_onetimeauth/poly1305/sse2/poly1305_sse2.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_pwhash/argon2/argon2-core.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_pwhash/argon2/argon2-core.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_pwhash/argon2/argon2-encoding.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_pwhash/argon2/argon2-encoding.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_pwhash/argon2/argon2-fill-block-avx2.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_pwhash/argon2/argon2-fill-block-avx512f.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_pwhash/argon2/argon2-fill-block-ref.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_pwhash/argon2/argon2-fill-block-ssse3.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_pwhash/argon2/argon2.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_pwhash/argon2/argon2.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_pwhash/argon2/blake2b-long.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_pwhash/argon2/blake2b-long.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_pwhash/argon2/blamka-round-avx2.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_pwhash/argon2/blamka-round-avx512f.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_pwhash/argon2/blamka-round-ref.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_pwhash/argon2/blamka-round-ssse3.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_pwhash/argon2/pwhash_argon2i.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_pwhash/argon2/pwhash_argon2id.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_pwhash/crypto_pwhash.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_scalarmult/crypto_scalarmult.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_scalarmult/curve25519/ref10/x25519_ref10.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_scalarmult/curve25519/ref10/x25519_ref10.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_scalarmult/curve25519/sandy2x/consts_namespace.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_scalarmult/curve25519/sandy2x/curve25519_sandy2x.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_scalarmult/curve25519/sandy2x/curve25519_sandy2x.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_scalarmult/curve25519/sandy2x/fe.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_scalarmult/curve25519/sandy2x/fe51.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_scalarmult/curve25519/sandy2x/fe51_invert.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_scalarmult/curve25519/sandy2x/fe51_namespace.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_scalarmult/curve25519/sandy2x/fe_frombytes_sandy2x.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_scalarmult/curve25519/sandy2x/ladder.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_scalarmult/curve25519/sandy2x/ladder_base.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_scalarmult/curve25519/sandy2x/ladder_base_namespace.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_scalarmult/curve25519/sandy2x/ladder_namespace.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_scalarmult/curve25519/scalarmult_curve25519.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_scalarmult/curve25519/scalarmult_curve25519.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_secretbox/crypto_secretbox.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_secretbox/crypto_secretbox_easy.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_secretbox/xsalsa20poly1305/secretbox_xsalsa20poly1305.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_secretstream/xchacha20poly1305/secretstream_xchacha20poly1305.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_shorthash/crypto_shorthash.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_shorthash/siphash24/ref/shorthash_siphash24_ref.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_shorthash/siphash24/ref/shorthash_siphash_ref.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_shorthash/siphash24/shorthash_siphash24.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_sign/crypto_sign.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_sign/ed25519/ref10/keypair.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_sign/ed25519/ref10/open.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_sign/ed25519/ref10/sign.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_sign/ed25519/ref10/sign_ed25519_ref10.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_sign/ed25519/sign_ed25519.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_stream/chacha20/dolbeau/chacha20_dolbeau-avx2.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_stream/chacha20/dolbeau/chacha20_dolbeau-avx2.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_stream/chacha20/dolbeau/chacha20_dolbeau-ssse3.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_stream/chacha20/dolbeau/chacha20_dolbeau-ssse3.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_stream/chacha20/dolbeau/u0.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_stream/chacha20/dolbeau/u1.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_stream/chacha20/dolbeau/u4.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_stream/chacha20/dolbeau/u8.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_stream/chacha20/ref/chacha20_ref.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_stream/chacha20/ref/chacha20_ref.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_stream/chacha20/stream_chacha20.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_stream/chacha20/stream_chacha20.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_stream/crypto_stream.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_stream/salsa20/ref/salsa20_ref.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_stream/salsa20/ref/salsa20_ref.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_stream/salsa20/stream_salsa20.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_stream/salsa20/stream_salsa20.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_stream/salsa20/xmm6/salsa20_xmm6.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_stream/salsa20/xmm6/salsa20_xmm6.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_stream/salsa20/xmm6int/salsa20_xmm6int-avx2.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_stream/salsa20/xmm6int/salsa20_xmm6int-avx2.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_stream/salsa20/xmm6int/salsa20_xmm6int-sse2.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_stream/salsa20/xmm6int/salsa20_xmm6int-sse2.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_stream/salsa20/xmm6int/u0.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_stream/salsa20/xmm6int/u1.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_stream/salsa20/xmm6int/u4.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_stream/salsa20/xmm6int/u8.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_stream/xsalsa20/stream_xsalsa20.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_verify/sodium/verify.c
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/core.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_aead_aes256gcm.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_aead_chacha20poly1305.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_aead_xchacha20poly1305.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_auth.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_auth_hmacsha256.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_auth_hmacsha512.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_auth_hmacsha512256.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_box.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_box_curve25519xchacha20poly1305.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_box_curve25519xsalsa20poly1305.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_core_ed25519.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_core_hchacha20.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_core_hsalsa20.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_core_salsa20.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_core_salsa2012.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_core_salsa208.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_generichash.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_generichash_blake2b.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_hash.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_hash_sha256.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_hash_sha512.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_kdf.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_kdf_blake2b.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_kx.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_onetimeauth.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_onetimeauth_poly1305.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_pwhash.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_pwhash_argon2i.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_pwhash_argon2id.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_pwhash_scryptsalsa208sha256.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_scalarmult.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_scalarmult_curve25519.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_scalarmult_ed25519.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_secretbox.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_secretbox_xchacha20poly1305.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_secretbox_xsalsa20poly1305.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_secretstream_xchacha20poly1305.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_shorthash.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_shorthash_siphash24.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_sign.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_sign_ed25519.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_sign_edwards25519sha512batch.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_stream.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_stream_chacha20.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_stream_salsa20.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_stream_salsa2012.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_stream_salsa208.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_stream_xchacha20.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_stream_xsalsa20.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_verify_16.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_verify_32.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/crypto_verify_64.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/export.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/private/chacha20_ietf_ext.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/private/common.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/private/ed25519_ref10.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/private/ed25519_ref10_fe_25_5.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/private/ed25519_ref10_fe_51.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/private/implementations.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/private/mutex.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/private/sse2_64_32.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/randombytes.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/randombytes_nativeclient.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/randombytes_salsa20_random.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/randombytes_sysrandom.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/runtime.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/utils.h
    ${LIBSODIUM_SRC}/src/libsodium/include/sodium/version.h
    ${LIBSODIUM_SRC}/src/libsodium/randombytes/nativeclient/randombytes_nativeclient.c
    ${LIBSODIUM_SRC}/src/libsodium/randombytes/randombytes.c
    ${LIBSODIUM_SRC}/src/libsodium/randombytes/salsa20/randombytes_salsa20_random.c
    ${LIBSODIUM_SRC}/src/libsodium/randombytes/sysrandom/randombytes_sysrandom.c
    ${LIBSODIUM_SRC}/src/libsodium/sodium/codecs.c
    ${LIBSODIUM_SRC}/src/libsodium/sodium/core.c
    ${LIBSODIUM_SRC}/src/libsodium/sodium/runtime.c
    ${LIBSODIUM_SRC}/src/libsodium/sodium/utils.c
    ${LIBSODIUM_SRC}/src/libsodium/sodium/version.c
)
target_sources(sodium_vendor
    PRIVATE
    ${LIBSODIUM_SRC}/src/libsodium/crypto_box/curve25519xchacha20poly1305/box_curve25519xchacha20poly1305.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_box/curve25519xchacha20poly1305/box_seal_curve25519xchacha20poly1305.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_core/ed25519/core_ed25519.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_pwhash/scryptsalsa208sha256/crypto_scrypt-common.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_pwhash/scryptsalsa208sha256/crypto_scrypt.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_pwhash/scryptsalsa208sha256/nosse/pwhash_scryptsalsa208sha256_nosse.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_pwhash/scryptsalsa208sha256/pbkdf2-sha256.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_pwhash/scryptsalsa208sha256/pbkdf2-sha256.h
    ${LIBSODIUM_SRC}/src/libsodium/crypto_pwhash/scryptsalsa208sha256/pwhash_scryptsalsa208sha256.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_pwhash/scryptsalsa208sha256/scrypt_platform.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_pwhash/scryptsalsa208sha256/sse/pwhash_scryptsalsa208sha256_sse.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_scalarmult/ed25519/ref10/scalarmult_ed25519_ref10.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_secretbox/xchacha20poly1305/secretbox_xchacha20poly1305.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_shorthash/siphash24/ref/shorthash_siphashx24_ref.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_shorthash/siphash24/shorthash_siphashx24.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_sign/ed25519/ref10/obsolete.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_stream/salsa2012/ref/stream_salsa2012_ref.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_stream/salsa2012/stream_salsa2012.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_stream/salsa208/ref/stream_salsa208_ref.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_stream/salsa208/stream_salsa208.c
    ${LIBSODIUM_SRC}/src/libsodium/crypto_stream/xchacha20/stream_xchacha20.c
)

set_target_properties(sodium_vendor
    PROPERTIES
        C_STANDARD 99
)

target_include_directories(sodium_vendor
    PUBLIC
        ${LIBSODIUM_SRC}/src/libsodium/include
    PRIVATE
        ${LIBSODIUM_SRC}/src/libsodium/include/sodium
)

target_compile_definitions(sodium_vendor
    PUBLIC
        $<$<NOT:$<BOOL:${BUILD_SHARED_LIBS}>>:SODIUM_STATIC>
        $<$<BOOL:${SODIUM_MINIMAL}>:SODIUM_LIBRARY_MINIMAL>
    PRIVATE
        $<$<BOOL:${BUILD_SHARED_LIBS}>:SODIUM_DLL_EXPORT>
        $<$<BOOL:${SODIUM_ENABLE_BLOCKING_RANDOM}>:USE_BLOCKING_RANDOM>
        $<$<BOOL:${SODIUM_MINIMAL}>:MINIMAL>
        $<$<BOOL:${SODIUM_PRETEND_TO_BE_CONFIGURED}>:CONFIGURED>
)

# Variables that need to be exported to version.h.in
set(VERSION_ORIG "${VERSION}") # an included module sets things in the calling scope :(
set(VERSION 1.0.17)
set(SODIUM_LIBRARY_VERSION_MAJOR 10)
set(SODIUM_LIBRARY_VERSION_MINOR 2)

configure_file(
  ${LIBSODIUM_SRC}/src/libsodium/include/sodium/version.h.in
  ${LIBSODIUM_SRC}/src/libsodium/include/sodium/version.h
)

set(VERSION "${VERSION_ORIG}")
