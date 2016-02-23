/*
 * Copyright (c) 2011 Apple Inc. All rights reserved.
 *
 * @APPLE_APACHE_LICENSE_HEADER_START@
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 * @APPLE_APACHE_LICENSE_HEADER_END@
 */
/*
    auto_tester.h
    Copyright (c) 2008-2011 Apple Inc. All rights reserved.
 */

#ifndef AUTO_TESTER_H
#define AUTO_TESTER_H

__BEGIN_DECLS

#ifdef AUTO_TESTER

#include "auto_zone.h"

typedef struct {
    void (*auto_probe_auto_zone_collect)(auto_zone_options_t options);
    void (*auto_probe_admin_deallocate)(void *address);
    void (*auto_probe_begin_heap_scan)(boolean_t generational);
    void (*auto_probe_begin_local_scan)();
    void (*auto_probe_collection_complete)();
    void (*auto_probe_end_heap_scan)(size_t garbage_count, void **garbage_blocks);
    void (*auto_probe_end_local_scan)(size_t garbage_count, void **garbage_blocks);
    void (*auto_scan_barrier)();
    void (*auto_probe_end_thread_scan)();
    void (*auto_probe_heap_collection_complete)();
    void (*auto_probe_local_collection_complete)();
    void (*auto_probe_mature)(void *address, unsigned char age);
    void (*auto_probe_make_global)(void *address, unsigned char age);
    void (*auto_probe_scan_range)(void *address, void *end);
    void (*auto_probe_scan_with_layout)(void *address, void *end, const unsigned char *map);
    void (*auto_probe_did_scan_with_layout)(void *address, void *end, const unsigned char *map);
    void (*auto_probe_set_pending)(void *block);
    void (*auto_probe_unregistered_thread_error)();
} AutoProbeFunctions;

extern AutoProbeFunctions *auto_probe_functions;

#define AUTO_PROBE(probe_func) do { if (auto_probe_functions) auto_probe_functions->probe_func; } while (0)

#else /* AUTO_TESTER */

typedef struct AutoProbeFunctions AutoProbeFunctions;
#define AUTO_PROBE(probe_func)

#endif /* AUTO_TESTER */

AUTO_EXPORT bool auto_set_probe_functions(AutoProbeFunctions *func)
    __OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_NA);

__END_DECLS

#endif /* AUTO_TESTER_H */
