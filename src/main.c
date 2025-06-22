/**
 * @file main.c
 * @brief Entry point for the GNSS L1 Position Resolver application.
 *
 * Initializes and starts the user interface for selecting RTCM input,
 * and ensures proper cleanup before exit.
 */

#include "../include/algo.h"

/**
 * @brief Main entry point of the GNSS application.
 *
 * Calls the application menu to prompt the user for RTCM input source selection.
 * After the session ends, it performs cleanup before exiting.
 *
 * @return int Returns 0 on successful exit.
 */
int main(void)
{
    // Call the application menu function
    app_menu();

    // Clean up and exit
    app_cleanup();

    return 0;
}
