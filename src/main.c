/**
 * @file main.c
 * @brief Entry point for the GNSS L1 Position Resolver application.
 *
 * The program launches a simple interactive flow that lets the user select an
 * RTCM input source, processes it, and then performs a clean shutdown.
 *
 * High-level flow:
 * 1) Invoke the application menu to select/process the RTCM source.
 * 2) Clean up any allocated resources before exit.
 *
 * @author
 *   Ade (Adedolapo Adegboye)
 *
 * @version 1.0
 * @date 2025-08-24
 */

#include "../include/algo.h"

/**
 * @brief Program entry point.
 *
 * Runs the application menu for RTCM input selection and processing, then
 * performs final cleanup before exiting.
 *
 * @return int 0 on success, non-zero on failure.
 */
int main(void)
{
    /* Launch the application menu (RTCM source selection & processing) */
    app_menu();

    /* Release resources and perform final cleanup */
    app_cleanup();

    return 0;
}
