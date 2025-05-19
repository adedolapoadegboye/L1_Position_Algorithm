/**
 * @file app_menu.c
 * @brief Menu interface for selecting RTCM input source in the GNSS application.
 *
 * This module provides a simple terminal-based menu to allow users to choose how
 * RTCM data will be fed into the GNSS L1 positioning resolver. Currently supports:
 *  - Serial port input
 *  - File-based input
 *  - Exit option
 */

#include "../include/algo.h"

/**
 * @brief Display the main menu for RTCM input source selection.
 *
 * The user is prompted to select an RTCM data input method:
 *  - Option 1: Serial Port (to be implemented)
 *  - Option 2: File (to be implemented)
 *  - Option 3: Exit the program
 *
 * The function runs in a loop until the user selects to exit.
 * Input is accepted via standard input and actions are printed to the terminal.
 */
void app_menu(void)
{
    int rtcm_input_source = 0;
    // Initialize the application
    printf("GNSS Positioning Application Starting...\n");

    // Set up any necessary configurations or initializations here

    // Display the application menu
    printf("Initializing application menu...\n");

    printf("Welcome to the GNSS Positioning Application!\n");
    printf("Note: Only GPS L1 is supported for now\n");
    printf("Please select RTCM input source:\n");
    printf("1. Serial Port\n");
    printf("2. File\n");
    printf("3. Exit\n");

    while (true)
    {
        printf("Enter your choice (1-3): ");
        scanf("%d", &rtcm_input_source);

        switch (rtcm_input_source)
        {
        case 1:
            printf("You selected Serial Port.\n");
            // Add code to handle serial port input
            break;
        case 2:
            printf("You selected File.\n");
            // Add code to handle file input
            break;
        case 3:
            // Clean up and exit
            printf("Application exiting...\n");
            return;
        default:
            printf("Invalid choice. Please try again.\n");
        }
    }
}
