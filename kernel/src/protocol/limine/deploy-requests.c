#include <limine/limine.h>
#include <limine/deploy.h>

#define LIMINE_DEFINE(VARIABLE_NAME, TYPE, ID, EXTRA)    \
__attribute__((used, section(".limine_requests")))       \
volatile struct TYPE LIMINE_REQ(VARIABLE_NAME) = {       \
    .id = ID,                                            \
    .revision = 4 EXTRA                                  \
}

__attribute__((used, section(".limine_requests")))
volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(4);

LIMINE_DEFINE(memmap, limine_memmap_request, LIMINE_MEMMAP_REQUEST_ID,);
LIMINE_DEFINE(framebuffer, limine_framebuffer_request, LIMINE_FRAMEBUFFER_REQUEST_ID,);
LIMINE_DEFINE(rsdp, limine_rsdp_request, LIMINE_RSDP_REQUEST_ID,);
LIMINE_DEFINE(executableAddress, limine_executable_address_request, LIMINE_EXECUTABLE_ADDRESS_REQUEST_ID,);
LIMINE_DEFINE(hhdm, limine_hhdm_request, LIMINE_HHDM_REQUEST_ID,);
LIMINE_DEFINE(mp, limine_mp_request, LIMINE_MP_REQUEST_ID,);

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;