/**
 * @file main.c
 * @brief Entry point for the GNSS L1 Position Resolver application.
 *
 * This file contains the main function, which serves as the starting point
 * for the GNSS L1 Position Resolver. It initializes the application menu
 * for selecting the RTCM input source and ensures proper cleanup before exiting.
 */

#include "../include/algo.h"

/**
 * @brief Main entry point of the GNSS L1 Position Resolver application.
 *
 * Executes the application menu to allow the user to select and process
 * an RTCM input source. After processing is complete, resources are cleaned up
 * before the program exits.
 *
 * @return int Returns 0 on successful execution, non-zero otherwise.
 */
int main(void)
{
    // Start the application menu for RTCM input selection
    app_menu();

    // Perform cleanup of allocated resources
    app_cleanup();

    return 0;
}
