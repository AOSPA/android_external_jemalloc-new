/*
 * Copyright (C) 2016 The Android Open Source Project
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

static pthread_mutex_t malloc_disabled_lock = PTHREAD_MUTEX_INITIALIZER;
static bool malloc_disabled_tcache;

int je_iterate(uintptr_t base, size_t size,
    void (*callback)(uintptr_t ptr, size_t size, void* arg), void* arg) {
  // TODO: Figure out how to implement this functionality for jemalloc5.
  return -1;
}

static void je_malloc_disable_prefork() {
  pthread_mutex_lock(&malloc_disabled_lock);
}

static void je_malloc_disable_postfork_parent() {
  pthread_mutex_unlock(&malloc_disabled_lock);
}

static void je_malloc_disable_postfork_child() {
  pthread_mutex_init(&malloc_disabled_lock, NULL);
}

void je_malloc_disable_init() {
  if (pthread_atfork(je_malloc_disable_prefork,
      je_malloc_disable_postfork_parent, je_malloc_disable_postfork_child) != 0) {
    malloc_write("<jemalloc>: Error in pthread_atfork()\n");
    if (opt_abort)
      abort();
  }
}

void je_malloc_disable() {
  static pthread_once_t once_control = PTHREAD_ONCE_INIT;
  pthread_once(&once_control, je_malloc_disable_init);

  pthread_mutex_lock(&malloc_disabled_lock);
  bool new_tcache = false;
  size_t old_len = sizeof(malloc_disabled_tcache);
  je_mallctl("thread.tcache.enabled",
      &malloc_disabled_tcache, &old_len,
      &new_tcache, sizeof(new_tcache));
  jemalloc_prefork();
}

void je_malloc_enable() {
  jemalloc_postfork_parent();
  if (malloc_disabled_tcache) {
    je_mallctl("thread.tcache.enabled", NULL, NULL,
        &malloc_disabled_tcache, sizeof(malloc_disabled_tcache));
  }
  pthread_mutex_unlock(&malloc_disabled_lock);
}
