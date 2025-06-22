/**
 * @file app_cleanup.c
 * @brief Cleanup routine for the GNSS Positioning Application.
 *
 * This module handles cleanup operations before the application exits.
 * Intended for releasing resources or final output messages.
 */

#include "../include/algo.h"

/**
 * @brief Perform cleanup operations before application exit.
 *
 * This function should be called at the end of the application's lifecycle.
 * Place any memory deallocation, file closing, or shutdown logic here.
 */
void app_cleanup(void)
{
    /* Perform any necessary cleanup operations here */

    printf(COLOR_GREEN "Cleanup completed. Exiting the application.\n" COLOR_RESET);
}
