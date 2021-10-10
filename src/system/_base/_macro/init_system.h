//
// Created by olivier on 09/10/2021.
//

#ifndef OPENLIFE_INIT_SYSTEM_H
#define OPENLIFE_INIT_SYSTEM_H

#if defined(__linux__)
#define SYSTEM linux

#elif defined(_WIN32)||defined(WIN32)
#define SYSTEM win32

#elif defined(__mac__)
#define SYSTEM mac

#else
#define SYSTEM system

#endif

#endif //OPENLIFE_INIT_SYSTEM_H
