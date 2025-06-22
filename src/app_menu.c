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

    printf(COLOR_BLUE "\n********************************************\n");
    printf("*         GNSS Positioning Engine     *\n");
    printf("********************************************\n\n" COLOR_RESET);

    printf(COLOR_YELLOW "Note: Only GPS L1 and RTCM input are supported at this time.\n\n" COLOR_RESET);

    printf(COLOR_GREEN "********** RTCM Input Source Menu **********\n");
    printf("* 1. Serial Port                           *\n");
    printf("* 2. File                                  *\n");
    printf("* 3. Exit                                  *\n");
    printf("********************************************\n" COLOR_RESET);

    while (true)
    {
        printf(COLOR_BLUE "\nEnter your choice (1-3): " COLOR_RESET);
        scanf("%d", &rtcm_input_source);

        switch (rtcm_input_source)
        {
        case 1:
            printf(COLOR_GREEN "You selected Serial Port.\n" COLOR_RESET);
            // Call the serial connection function
            break;
        case 2:
            printf(COLOR_GREEN "You selected File.\n" COLOR_RESET);
            // Add code to handle file input
            break;
        case 3:
            printf(COLOR_RED "Exiting the application...\n" COLOR_RESET);
            return;
        default:
            printf(COLOR_RED "Invalid choice. Please try again.\n" COLOR_RESET);
        }
    }
}
