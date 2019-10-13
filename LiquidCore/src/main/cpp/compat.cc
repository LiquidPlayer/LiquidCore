/*
 * Copyright (c) 2019 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include <cstdlib>

#ifdef __arm__
int dl_iterate_phdr(int(*callback) (struct dl_phdr_info *, size_t, void *), void *data)
{
  return 0;
}
#endif
