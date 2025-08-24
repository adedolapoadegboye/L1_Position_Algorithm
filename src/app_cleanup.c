/**
 * @file app_cleanup.c
 * @brief Cleanup routine for the GNSS L1 Position Resolver application.
 *
 * This module provides the cleanup routine that runs before the application exits.
 * It is intended for releasing resources, closing files, freeing memory, or
 * performing any other final shutdown operations.
 */

#include "../include/algo.h"

/**
 * @brief Perform cleanup operations before application exit.
 *
 * This function should be called at the end of the application's lifecycle.
 * Currently, it only prints a confirmation message, but in a full deployment
 * it can be expanded to include:
 *  - Memory deallocation
 *  - File handle closure
 *  - Hardware resource shutdown
 *  - Logging or telemetry flushing
 *
 * @todo Add actual resource cleanup when dynamic allocations or handles are introduced.
 */
void app_cleanup(void)
{
    /* Placeholder: no dynamic resources to release yet */
    printf(COLOR_GREEN "Cleanup completed. Exiting the application.\n" COLOR_RESET);
}
