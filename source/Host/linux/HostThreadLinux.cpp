//===-- HostThreadLinux.cpp -------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "lldb/Core/DataBuffer.h"
#include "lldb/Host/linux/HostThreadLinux.h"
#include "Plugins/Process/Linux/ProcFileReader.h"

#include "llvm/ADT/SmallVector.h"

#include <pthread.h>
#include <dlfcn.h>

typedef int (*pthread_setname_np_t)(pthread_t , const char *);
static bool pthread_setname_np_initialized = false;
static pthread_setname_np_t pthread_setname_np_func = 0;

using namespace lldb_private;

HostThreadLinux::HostThreadLinux()
    : HostThreadPosix()
{
}

HostThreadLinux::HostThreadLinux(lldb::thread_t thread)
    : HostThreadPosix(thread)
{
}

void
HostThreadLinux::SetName(lldb::thread_t thread, llvm::StringRef name)
{
    // We don't fear race conditions here. Even if two threads will do the same job simultaneously,
    // it will not break logic, since both will set the same variable twice with the same value
    if (!pthread_setname_np_initialized)
    {
        void *pc = ::dlopen(NULL, RTLD_LAZY);
        pthread_setname_np_func = (pthread_setname_np_t)::dlsym(pc, "pthread_setname_np");
        ::dlclose(pc);
        pthread_setname_np_initialized = true;
    }
    if (pthread_setname_np_func)
        pthread_setname_np_func(thread, name.data());
}

void
HostThreadLinux::GetName(lldb::thread_t thread, llvm::SmallVectorImpl<char> &name)
{
    // Read /proc/$TID/comm file.
    lldb::DataBufferSP buf_sp = process_linux::ProcFileReader::ReadIntoDataBuffer(thread, "comm");
    const char *comm_str = (const char *)buf_sp->GetBytes();
    const char *cr_str = ::strchr(comm_str, '\n');
    size_t length = cr_str ? (cr_str - comm_str) : strlen(comm_str);

    name.clear();
    name.append(comm_str, comm_str + length);
}
