#ifndef LIMINE_REQ_DEPLOYER_H
#define LIMINE_REQ_DEPLOYER_H

#define LIMINE_REQ(name) __limine_##name
#define LIMINE_DECLARE(variableName, typeName) extern volatile struct typeName LIMINE_REQ(variableName)

#define LIMINE_BASE_REF() extern volatile uint64_t limine_base_revision[3]
#define LIMINE_GET_MEMMAP() LIMINE_DECLARE(memmap, limine_memmap_request)
#define LIMINE_GET_FRAMEBUFFER() LIMINE_DECLARE(framebuffer, limine_framebuffer_request)
#define LIMINE_GET_RSDP() LIMINE_DECLARE(rsdp, limine_rsdp_request)
#define LIMINE_GET_EXECUTABLE_ADDR() LIMINE_DECLARE(executableAddress, limine_executable_address_request)
#define LIMINE_GET_HHDM() LIMINE_DECLARE(hhdm, limine_hhdm_request)
#define LIMINE_GET_MP() LIMINE_DECLARE(mp, limine_mp_request)

#endif