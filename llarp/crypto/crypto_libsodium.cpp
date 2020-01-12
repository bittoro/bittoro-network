#include <crypto/crypto_libsodium.hpp>
#include <sodium/crypto_generichash.h>
#include <sodium/crypto_sign.h>
#include <sodium/crypto_scalarmult.h>
#include <sodium/crypto_stream_xchacha20.h>
#include <util/mem.hpp>

#include <cassert>

extern "C"
{
  extern int
  sodium_init(void);
}

namespace llarp
{
  namespace sodium
  {
    static bool
    dh(llarp::SharedSecret &out, const PubKey &client_pk,
       const PubKey &server_pk, const uint8_t *themPub, const SecretKey &usSec)
    {
      llarp::SharedSecret shared;
      crypto_generichash_state h;

      if(crypto_scalarmult_curve25519(shared.data(), usSec.data(), themPub))
      {
        return false;
      }
      crypto_generichash_blake2b_init(&h, nullptr, 0U, shared.size());
      crypto_generichash_blake2b_update(&h, client_pk.data(), 32);
      crypto_generichash_blake2b_update(&h, server_pk.data(), 32);
      crypto_generichash_blake2b_update(&h, shared.data(), 32);
      crypto_generichash_blake2b_final(&h, out.data(), shared.size());
      return true;
    }

    static bool
    dh_client_priv(llarp::SharedSecret &shared, const PubKey &pk,
                   const SecretKey &sk, const TunnelNonce &n)
    {
      llarp::SharedSecret dh_result;

      if(dh(dh_result, sk.toPublic(), pk, pk.data(), sk))
      {
        return crypto_generichash_blake2b(shared.data(), 32, n.data(), 32,
                                          dh_result.data(), 32)
            != -1;
      }
      llarp::LogWarn("crypto::dh_client - dh failed");
      return false;
    }

    static bool
    dh_server_priv(llarp::SharedSecret &shared, const PubKey &pk,
                   const SecretKey &sk, const TunnelNonce &n)
    {
      llarp::SharedSecret dh_result;
      if(dh(dh_result, pk, sk.toPublic(), pk.data(), sk))
      {
        return crypto_generichash_blake2b(shared.data(), 32, n.data(), 32,
                                          dh_result.data(), 32)
            != -1;
      }
      llarp::LogWarn("crypto::dh_server - dh failed");
      return false;
    }

    CryptoLibSodium::CryptoLibSodium()
    {
      if(sodium_init() == -1)
      {
        throw std::runtime_error("sodium_init() returned -1");
      }
      char *avx2 = std::getenv("AVX2_FORCE_DISABLE");
      if(avx2 && std::string(avx2) == "1")
      {
        ntru_init(1);
      }
      else
      {
        ntru_init(0);
      }
      int seed = 0;
      randombytes(reinterpret_cast< unsigned char * >(&seed), sizeof(seed));
      srand(seed);
    }

    bool
    CryptoLibSodium::xchacha20(const llarp_buffer_t &buff,
                               const SharedSecret &k, const TunnelNonce &n)
    {
      return crypto_stream_xchacha20_xor(buff.base, buff.base, buff.sz,
                                         n.data(), k.data())
          == 0;
    }

    bool
    CryptoLibSodium::xchacha20_alt(const llarp_buffer_t &out,
                                   const llarp_buffer_t &in,
                                   const SharedSecret &k, const byte_t *n)
    {
      if(in.sz > out.sz)
        return false;
      return crypto_stream_xchacha20_xor(out.base, in.base, in.sz, n, k.data())
          == 0;
    }

    bool
    CryptoLibSodium::dh_client(llarp::SharedSecret &shared, const PubKey &pk,
                               const SecretKey &sk, const TunnelNonce &n)
    {
      return dh_client_priv(shared, pk, sk, n);
    }
    /// path dh relay side
    bool
    CryptoLibSodium::dh_server(llarp::SharedSecret &shared, const PubKey &pk,
                               const SecretKey &sk, const TunnelNonce &n)
    {
      return dh_server_priv(shared, pk, sk, n);
    }
    /// transport dh client side
    bool
    CryptoLibSodium::transport_dh_client(llarp::SharedSecret &shared,
                                         const PubKey &pk, const SecretKey &sk,
                                         const TunnelNonce &n)
    {
      return dh_client_priv(shared, pk, sk, n);
    }
    /// transport dh server side
    bool
    CryptoLibSodium::transport_dh_server(llarp::SharedSecret &shared,
                                         const PubKey &pk, const SecretKey &sk,
                                         const TunnelNonce &n)
    {
      return dh_server_priv(shared, pk, sk, n);
    }

    bool
    CryptoLibSodium::shorthash(ShortHash &result, const llarp_buffer_t &buff)
    {
      return crypto_generichash_blake2b(result.data(), ShortHash::SIZE,
                                        buff.base, buff.sz, nullptr, 0)
          != -1;
    }

    bool
    CryptoLibSodium::hmac(byte_t *result, const llarp_buffer_t &buff,
                          const SharedSecret &secret)
    {
      return crypto_generichash_blake2b(result, HMACSIZE, buff.base, buff.sz,
                                        secret.data(), HMACSECSIZE)
          != -1;
    }

    static bool
    hash(uint8_t *result, const llarp_buffer_t &buff)
    {
      return crypto_generichash_blake2b(result, HASHSIZE, buff.base, buff.sz,
                                        nullptr, 0)
          != -1;
    }

    bool
    CryptoLibSodium::sign(Signature &sig, const SecretKey &secret,
                          const llarp_buffer_t &buf)
    {
      return crypto_sign_detached(sig.data(), nullptr, buf.base, buf.sz,
                                  secret.data())
          != -1;
    }

    bool
    CryptoLibSodium::verify(const PubKey &pub, const llarp_buffer_t &buf,
                            const Signature &sig)
    {
      return crypto_sign_verify_detached(sig.data(), buf.base, buf.sz,
                                         pub.data())
          != -1;
    }

    bool
    CryptoLibSodium::seed_to_secretkey(llarp::SecretKey &secret,
                                       const llarp::IdentitySecret &seed)
    {
      PubKey pk;
      return crypto_sign_ed25519_seed_keypair(pk.data(), secret.data(),
                                              seed.data())
          != -1;
    }

    void
    CryptoLibSodium::randomize(const llarp_buffer_t &buff)
    {
      randombytes((unsigned char *)buff.base, buff.sz);
    }

    void
    CryptoLibSodium::randbytes(byte_t *ptr, size_t sz)
    {
      randombytes((unsigned char *)ptr, sz);
    }

    void
    CryptoLibSodium::identity_keygen(llarp::SecretKey &keys)
    {
      PubKey pk;
      int result = crypto_sign_keypair(pk.data(), keys.data());
      assert(result != -1);
      const PubKey sk_pk = keys.toPublic();
      assert(pk == sk_pk);
      (void)result;
      (void)sk_pk;
    }

    bool
    CryptoLibSodium::check_identity_privkey(const llarp::SecretKey &keys)
    {
      AlignedBuffer< crypto_sign_SEEDBYTES > seed;
      llarp::PubKey pk;
      llarp::SecretKey sk;
      if(crypto_sign_ed25519_sk_to_seed(seed.data(), keys.data()) == -1)
        return false;
      if(crypto_sign_seed_keypair(pk.data(), sk.data(), seed.data()) == -1)
        return false;
      return keys.toPublic() == pk && sk == keys;
    }

    void
    CryptoLibSodium::encryption_keygen(llarp::SecretKey &keys)
    {
      auto d = keys.data();
      randbytes(d, 32);
      crypto_scalarmult_curve25519_base(d + 32, d);
    }

    bool
    CryptoLibSodium::pqe_encrypt(PQCipherBlock &ciphertext,
                                 SharedSecret &sharedkey,
                                 const PQPubKey &pubkey)
    {
      return crypto_kem_enc(ciphertext.data(), sharedkey.data(), pubkey.data())
          != -1;
    }
    bool
    CryptoLibSodium::pqe_decrypt(const PQCipherBlock &ciphertext,
                                 SharedSecret &sharedkey,
                                 const byte_t *secretkey)
    {
      return crypto_kem_dec(sharedkey.data(), ciphertext.data(), secretkey)
          != -1;
    }

    void
    CryptoLibSodium::pqe_keygen(PQKeyPair &keypair)
    {
      auto d = keypair.data();
      crypto_kem_keypair(d + PQ_SECRETKEYSIZE, d);
    }
  }  // namespace sodium

  const byte_t *
  seckey_topublic(const SecretKey &sec)
  {
    return sec.data() + 32;
  }

  const byte_t *
  pq_keypair_to_public(const PQKeyPair &k)
  {
    return k.data() + PQ_SECRETKEYSIZE;
  }

  const byte_t *
  pq_keypair_to_secret(const PQKeyPair &k)
  {
    return k.data();
  }

  uint64_t
  randint()
  {
    uint64_t i;
    randombytes((byte_t *)&i, sizeof(i));
    return i;
  }

}  // namespace llarp
