#pragma once
#ifndef CPU_TOPOLOGY_H
#define CPU_TOPOLOGY_H

#include <stdint.h>

extern uint32_t cpuCount;
extern uint32_t *cpuIDs;

static inline uint32_t cpuGetID() {
    return cpuIDs[0];
}

#endif