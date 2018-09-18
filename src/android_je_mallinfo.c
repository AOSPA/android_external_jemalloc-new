/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* Only use bin locks since the stats are now all atomic and can be read
 * without taking the stats lock.
 */
struct mallinfo je_mallinfo() {
  struct mallinfo mi;
  memset(&mi, 0, sizeof(mi));

  malloc_mutex_lock(TSDN_NULL, &arenas_lock);
  for (unsigned i = 0; i < narenas_auto; i++) {
    arena_t* arena = atomic_load_p(&arenas[i], ATOMIC_ACQUIRE);
    if (arena != NULL) {
      mi.hblkhd += atomic_load_zu(&arena->stats.mapped, ATOMIC_ACQUIRE);
      mi.uordblks += atomic_load_zu(&arena->stats.allocated_large, ATOMIC_ACQUIRE);

      for (unsigned j = 0; j < NBINS; j++) {
        bin_t* bin = &arena->bins[j];

        malloc_mutex_lock(TSDN_NULL, &bin->lock);
        mi.uordblks += bin_infos[j].reg_size * bin->stats.curregs;
        malloc_mutex_unlock(TSDN_NULL, &bin->lock);
      }
    }
  }
  malloc_mutex_unlock(TSDN_NULL, &arenas_lock);
  mi.fordblks = mi.hblkhd - mi.uordblks;
  mi.usmblks = mi.hblkhd;
  return mi;
}

size_t __mallinfo_narenas() {
  return narenas_auto;
}

size_t __mallinfo_nbins() {
  return NBINS;
}

struct mallinfo __mallinfo_arena_info(size_t aidx) {
  struct mallinfo mi;
  memset(&mi, 0, sizeof(mi));

  malloc_mutex_lock(TSDN_NULL, &arenas_lock);
  if (aidx < narenas_auto) {
    arena_t* arena = atomic_load_p(&arenas[aidx], ATOMIC_ACQUIRE);
    if (arena != NULL) {
      mi.hblkhd = atomic_load_zu(&arena->stats.mapped, ATOMIC_ACQUIRE);
      mi.ordblks = atomic_load_zu(&arena->stats.allocated_large, ATOMIC_ACQUIRE);

      for (unsigned j = 0; j < NBINS; j++) {
        bin_t* bin = &arena->bins[j];

        malloc_mutex_lock(TSDN_NULL, &bin->lock);
        mi.fsmblks += bin_infos[j].reg_size * bin->stats.curregs;
        malloc_mutex_unlock(TSDN_NULL, &bin->lock);
      }
    }
  }
  malloc_mutex_unlock(TSDN_NULL, &arenas_lock);
  return mi;
}

struct mallinfo __mallinfo_bin_info(size_t aidx, size_t bidx) {
  struct mallinfo mi;
  memset(&mi, 0, sizeof(mi));

  malloc_mutex_lock(TSDN_NULL, &arenas_lock);
  if (aidx < narenas_auto && bidx < NBINS) {
    arena_t* arena = atomic_load_p(&arenas[aidx], ATOMIC_ACQUIRE);
    if (arena != NULL) {
      bin_t* bin = &arena->bins[bidx];

      malloc_mutex_lock(TSDN_NULL, &bin->lock);
      mi.ordblks = bin_infos[bidx].reg_size * bin->stats.curregs;
      mi.uordblks = (size_t) bin->stats.nmalloc;
      mi.fordblks = (size_t) bin->stats.ndalloc;
      malloc_mutex_unlock(TSDN_NULL, &bin->lock);
    }
  }
  malloc_mutex_unlock(TSDN_NULL, &arenas_lock);
  return mi;
}
