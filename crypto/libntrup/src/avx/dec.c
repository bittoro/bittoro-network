#ifdef KAT
#include <stdio.h>
#endif
#include <sodium/crypto_hash_sha512.h>
#include <sodium/crypto_verify_32.h>
#include <sodium/crypto_kem.h>

#include "params.h"
#include "small.h"
#include "mod3.h"
#include "rq.h"
#include "r3.h"

int
crypto_kem_dec_avx2(unsigned char *k, const unsigned char *cstr,
                    const unsigned char *sk)
{
#if __AVX2__
  small f[768];
  modq h[768];
  small grecip[768];
  modq c[768];
  modq t[768];
  small t3[768];
  small r[768];
  modq hr[768];
  unsigned char rstr[small_encode_len];
  unsigned char hash[64];
  int i;
  int result = 0;

  small_decode(f, sk);
  small_decode(grecip, sk + small_encode_len);
  rq_decode(h, sk + 2 * small_encode_len);

  rq_decoderounded(c, cstr + 32);

  rq_mult(t, c, f);
  rq_mod3(t3, t);

  r3_mult(r, t3, grecip);

#ifdef KAT
  {
    int j;
    printf("decrypt r:");
    for(j = 0; j < p; ++j)
      if(r[j] == 1)
        printf(" +%d", j);
      else if(r[j] == -1)
        printf(" -%d", j);
    printf("\n");
  }
#endif

  result |= r3_weightw_mask(r);

  rq_mult(hr, h, r);
  rq_round3(hr, hr);
  for(i = 0; i < p; ++i)
    result |= modq_nonzero_mask(hr[i] - c[i]);

  small_encode(rstr, r);
  crypto_hash_sha512(hash, rstr, sizeof rstr);
  result |= crypto_verify_32(hash, cstr);

  for(i = 0; i < 32; ++i)
    k[i] = (hash[32 + i] & ~result);
  return result;
#else
  (void)(k);
  (void)(sk);
  (void)(cstr);
  return -1;
#endif
}
