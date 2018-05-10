// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

// Heavily rewritten from https://github.com/jtolds/malloc_instrumentation.git
// Compile with -shared -fPIC -ldl

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/syscall.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>

#define EXITCODE_OVERFLOW 51
#define EXITCODE_DLERROR  52

__thread unsigned int entered = 0;

static inline int start_call(void) {
    return __sync_fetch_and_add(&entered, 1);
}

static inline void end_call(void) {
    __sync_fetch_and_sub(&entered, 1);
}

static inline pid_t ourgettid(void) {
    return syscall(SYS_gettid);
}

//
// Failure callback
//
static int s_assigncb;
static void* (*s_cb)(int alloc_type, size_t size);
#define TRIGGERPTR(p) ((intptr_t)(p) == (intptr_t)(-1))

//
// Mini-allocator used for bootstrapping
//
static char dm_buf[1 << 22];
static unsigned long dm_pos;
static void *dm_lastptr;
static size_t dm_lastsize;

#define DUMMYPTR(p) ((char*)p >= dm_buf && (char*)p < dm_buf + sizeof(dm_buf))

static void* dummy_malloc(size_t size) {
    if (dm_pos + size >= sizeof(dm_buf))
        _exit(EXITCODE_OVERFLOW);

    void *retptr = dm_buf + dm_pos;
    dm_pos += size;

    dm_lastptr = retptr;
    dm_lastsize = size;
    return retptr;
}

static void* dummy_calloc(size_t nmemb, size_t size) {
    void *retptr = dummy_malloc(nmemb * size);
    memset(retptr, 0, nmemb * size);
    return retptr;
}

static void* dummy_realloc(void *ptr, size_t size) {
    if (!ptr || dm_lastptr != ptr)
        return dummy_malloc(size);

    if (size > dm_lastsize) {
        size_t extra = size - dm_lastsize;

        if (dm_pos + extra >= sizeof(dm_buf))
            _exit(EXITCODE_OVERFLOW);

        dm_pos += extra;
        dm_lastsize = size;
    }
    return ptr;
}

static void* dummy_memalign(size_t blocksize, size_t size) {
    size_t padding = 0;

    if (dm_pos % blocksize)
        dm_pos += blocksize - (dm_pos % blocksize);

    dm_pos += padding;
    void *retptr = dummy_malloc(size);
    dm_lastsize += padding;
    return retptr;
}

static void dummy_free(void *ptr) {
    if (ptr && dm_lastptr == ptr) {
        dm_pos -= dm_lastsize;
        dm_lastptr = NULL;
    }
}

//
// Library constructor that sets up hooks
//
static void* (*real_malloc)(size_t size) = dummy_malloc;
static void* (*real_calloc)(size_t nmemb, size_t size) = dummy_calloc;
static void* (*real_realloc)(void *ptr, size_t size) = dummy_realloc;
static void* (*real_memalign)(size_t blocksize, size_t size) = dummy_memalign;
static void  (*real_free)(void *ptr) = dummy_free;
static void* (*real_valloc)(size_t size);
static int   (*real_posix_memalign)(void** memptr, size_t alignment, size_t size);

void __attribute__((constructor)) hookfns(void)
{
    start_call();

    typeof(real_malloc)   temp_malloc   = dlsym(RTLD_NEXT, "malloc");
    typeof(real_calloc)   temp_calloc   = dlsym(RTLD_NEXT, "calloc");
    typeof(real_realloc)  temp_realloc  = dlsym(RTLD_NEXT, "realloc");
    typeof(real_free)     temp_free     = dlsym(RTLD_NEXT, "free");
    typeof(real_memalign) temp_memalign = dlsym(RTLD_NEXT, "memalign");
    typeof(real_valloc)   temp_valloc   = dlsym(RTLD_NEXT, "valloc");
    typeof(real_posix_memalign) temp_posix_memalign = dlsym(RTLD_NEXT, "posix_memalign");

    if (!temp_malloc || !temp_calloc || !temp_realloc || !temp_memalign ||
        !temp_valloc || !temp_posix_memalign || !temp_free)
    {
        // fprintf(stderr, "Error in `dlsym`: %s\n", dlerror());
        _exit(EXITCODE_DLERROR);
    }

    real_malloc         = temp_malloc;
    real_calloc         = temp_calloc;
    real_realloc        = temp_realloc;
    real_free           = temp_free;
    real_memalign       = temp_memalign;
    real_valloc         = temp_valloc;
    real_posix_memalign = temp_posix_memalign;

    end_call();
}

//
// Call processing
//
typedef struct record_tag {
    enum alloc_type {
        NEW_CALL,
        MALLOC_CALL,
        CALLOC_CALL,
        REALLOC_CALL,
        MEMALIGN_CALL,
        VALLOC_CALL,
        POSIX_MEMALIGN_CALL,
        FREE_CALL
    } type;

    size_t size;
    void *in_ptr;
    void *out_ptr;

    union {
        struct {
            size_t nmemb;
        } calloc_call;

        struct {
            size_t blocksize;
        } memalign_call;

        struct {
            void** memptr;
            size_t alignment;
            int rv;
        } posix_memalign_call;
    };
} call_record;

static inline void checkptr(call_record *record) {
    if (record->out_ptr == NULL) {
        if (s_cb)
            (*s_cb)(record->type, record->size);
        else
            abort();
    }
}

static inline void checkrv(call_record *record) {
    if (record->posix_memalign_call.rv != 0) {
        if (s_cb)
            (*s_cb)(record->type, record->size);
        else
            abort();
    }
}

#define CHECKPTR(r) checkptr(r);
#define CHECKRV(r) checkrv(r);
#define DUMPLINE(fmt, ...) if (!internal) { }

static void do_call(call_record *record) {
    int internal = start_call();

    switch(record->type) {
    case MALLOC_CALL:
        record->out_ptr = real_malloc(record->size);
        CHECKPTR(record);
        DUMPLINE("malloc(%zu) = %p",
                 record->size,
                 record->out_ptr);
        break;

    case CALLOC_CALL:
        record->out_ptr = real_calloc(
            record->calloc_call.nmemb,
            record->size);
        CHECKPTR(record);
        DUMPLINE("calloc(%zu, %zu) = %p",
                 record->calloc_call.nmemb,
                 record->size,
                 record->out_ptr);
        break;

    case REALLOC_CALL:
        if (DUMMYPTR(record->in_ptr)) {
            record->out_ptr = dummy_realloc(
                record->in_ptr,
                record->size);
            DUMPLINE("reallocDUMMY(%p, %zu) = %p",
                     record->in_ptr,
                     record->size,
                     record->out_ptr);
        } else {
            record->out_ptr = real_realloc(
                record->in_ptr,
                record->size);
            CHECKPTR(record);
            DUMPLINE("realloc(%p, %zu) = %p",
                     record->in_ptr,
                     record->size,
                     record->out_ptr);
        }
        break;

    case MEMALIGN_CALL:
        record->out_ptr = real_memalign(
            record->memalign_call.blocksize,
            record->size);
        CHECKPTR(record);
        DUMPLINE("memalign(%zu, %zu) = %p",
                 record->memalign_call.blocksize,
                 record->size,
                 record->out_ptr);
        break;

    case VALLOC_CALL:
        record->out_ptr = real_valloc(
            record->size);
        CHECKPTR(record);
        DUMPLINE("valloc(%zu) = %p",
                 record->size,
                 record->out_ptr);
        break;

    case POSIX_MEMALIGN_CALL:
        record->posix_memalign_call.rv = real_posix_memalign(
            record->posix_memalign_call.memptr,
            record->posix_memalign_call.alignment,
            record->size);
        CHECKRV(record);
        if (record->posix_memalign_call.rv == 0) {
            DUMPLINE("posix_memalign(%p, %zu, %zu) = 0, %p",
                     record->posix_memalign_call.memptr,
                     record->posix_memalign_call.alignment,
                     record->size,
                     *record->posix_memalign_call.memptr);
        } else {
            DUMPLINE("posix_memalign(%p, %zu, %zu) = %d, NULL",
                     record->posix_memalign_call.memptr,
                     record->posix_memalign_call.alignment,
                     record->size,
                     record->posix_memalign_call.rv);
        }
        break;

    case FREE_CALL:
        if (record->in_ptr == NULL) {
            break;
        }
        else if (DUMMYPTR(record->in_ptr)) {
            dummy_free(record->in_ptr);
            DUMPLINE("freeDUMMY(%p)", record->in_ptr);
        }
        else if (TRIGGERPTR(record->in_ptr)) {
            s_assigncb = 1;
            break;
        }
        else if (s_assigncb != 0) {
            s_cb = record->in_ptr;
            s_assigncb = 0;
            break;
        }
        else {
            real_free(record->in_ptr);
            DUMPLINE("free(%p)", record->in_ptr);
        }
        break;

    default:
        break;
    };

    end_call();
}

void* malloc(size_t size) {
    call_record record;
    record.type = MALLOC_CALL;
    record.size = size;
    do_call(&record);

    return record.out_ptr;
}

void* calloc(size_t nmemb, size_t size) {
    call_record record;
    record.type = CALLOC_CALL;
    record.size = size;
    record.calloc_call.nmemb = nmemb;
    do_call(&record);

    return record.out_ptr;
}

void* realloc(void *ptr, size_t size) {
    call_record record;
    record.type = REALLOC_CALL;
    record.in_ptr = ptr;
    record.size = size;
    do_call(&record);

    return record.out_ptr;
}

void free(void *ptr) {
    call_record record;
    record.type = FREE_CALL;
    record.in_ptr = ptr;
    do_call(&record);
}

void* memalign(size_t blocksize, size_t size) {
    call_record record;
    record.type = MEMALIGN_CALL;
    record.memalign_call.blocksize = blocksize;
    record.size = size;
    do_call(&record);

    return record.out_ptr;
}

int posix_memalign(void** memptr, size_t alignment, size_t size) {
    call_record record;
    record.type = MEMALIGN_CALL;
    record.posix_memalign_call.memptr = memptr;
    record.posix_memalign_call.alignment = alignment;
    record.size = size;
    do_call(&record);

    return record.posix_memalign_call.rv;
}

void* valloc(size_t size) {
    call_record record;
    record.type = VALLOC_CALL;
    record.size = size;
    do_call(&record);

    return record.out_ptr;
}
