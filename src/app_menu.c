/**
 * @file app_menu.c
 * @brief Menu interface for selecting RTCM input source in the GNSS application.
 *
 * Provides a terminal-driven menu for choosing how RTCM data is fed into the
 * GNSS L1 positioning resolver. At present, only the "Pre-recorded File" modes
 * are functional. Serial input (Option 1) and live parsed input (Option 2) are
 * defined in the menu but not yet implemented.
 *
 * Usage flow:
 *  - Displays a banner and usage notice
 *  - Displays an input menu
 *  - Prompts the user for a choice
 *  - Dispatches to the appropriate handler function
 *
 * @note
 *   - Option 1 (Serial Port Input) is not implemented yet.
 *   - Option 2 (Pre-recorded File Input, text format) is placeholder-only.
 *   - Option 3 (Pre-recorded File Input, parsed with PyRTCM) is supported.
 */

#include "../include/algo.h" // assumes color macros, serial_connect(), file_input_mode()

#define INPUT_BUF_SZ 64

/** @brief Print program banner with title and supported features. */
static void print_banner(void)
{
    printf(COLOR_BLUE
           "\n********************************************\n"
           "*         GNSS Positioning Engine          *\n"
           "********************************************\n\n" COLOR_RESET);

    printf(COLOR_YELLOW
           "Note: Only GPS L1 signals from RTCM MSM1 and MSM4 inputs are supported at this time.\n\n" COLOR_RESET);
}

/** @brief Print the main RTCM input selection menu. */
static void print_menu(void)
{
    printf(COLOR_GREEN
           "********** RTCM Input Source Menu **********\n"
           "* 1. Serial Port  (Raw Binary)             *\n"
           "*    [Not yet implemented]                 *\n"
           "* 2. Pre-recorded File (Text format)        *\n"
           "*    [Placeholder only, not functional]    *\n"
           "* 3. Pre-recorded File (Parsed with PyRTCM)*\n"
           "* 4. Exit                                  *\n"
           "********************************************\n" COLOR_RESET);
}

/**
 * @brief Prompt the user for a menu choice and parse input.
 *
 * Reads from stdin and validates the entered choice. Handles whitespace,
 * newline, and EOF gracefully.
 *
 * @return Parsed menu option in [1..4], or 4 if input is closed (EOF).
 */
static int prompt_choice(void)
{
    char buf[INPUT_BUF_SZ];

    printf(COLOR_BLUE "\nEnter your choice (1-4): " COLOR_RESET);
    fflush(stdout);

    if (!fgets(buf, sizeof(buf), stdin))
    {
        // EOF or error -> treat as "Exit"
        puts(COLOR_RED "Input closed. Exiting..." COLOR_RESET);
        return 4;
    }

    // Trim leading/trailing whitespace
    char *p = buf;
    while (*p == ' ' || *p == '\t')
        p++;
    char *end = p + strlen(p);
    while (end > p && (end[-1] == '\n' || end[-1] == '\r' || end[-1] == ' ' || end[-1] == '\t'))
    {
        *--end = '\0';
    }

    // Convert to integer safely
    char *conv_end = NULL;
    long v = strtol(p, &conv_end, 10);
    if (conv_end == p || *conv_end != '\0')
    {
        printf(COLOR_RED "Invalid input. Please enter a number 1-4.\n" COLOR_RESET);
        return -1;
    }
    if (v < 1 || v > 4)
    {
        printf(COLOR_RED "Choice out of range. Please enter 1-4.\n" COLOR_RESET);
        return -1;
    }
    return (int)v;
}

/**
 * @brief Application menu loop for selecting RTCM input source.
 *
 * Prints a banner and menu, then repeatedly prompts the user until
 * a valid choice is entered. Dispatches to the respective handler for
 * each menu option.
 *
 * @todo Implement Option 1 (Serial Port Input).
 * @todo Implement Option 2 (Pre-recorded File Input, Raw Binary).
 */
void app_menu(void)
{
    print_banner();
    print_menu();

    for (;;)
    {
        int choice = prompt_choice();
        if (choice == -1)
        {
            continue; // re-prompt on parse errors
        }

        switch (choice)
        {
        case 1:
            printf(COLOR_GREEN "You selected Serial Port Input (Raw binary message).\n" COLOR_RESET);
            printf(COLOR_YELLOW "Note: This mode is not implemented yet.\n" COLOR_RESET);
            // @todo Implement serial input mode
            // serial_connect(NULL, 0);
            break;

        case 2:
            printf(COLOR_GREEN "You selected Pre-recorded File Input (Parsed text format).\n" COLOR_RESET);
            printf(COLOR_YELLOW "Note: This mode is placeholder-only and not functional yet.\n" COLOR_RESET);
            // @todo Implement raw binary file input mode
            // file_input_mode(false);
            break;

        case 3:
            printf(COLOR_GREEN "You selected Pre-recorded File Input (Parsed with PyRTCM).\n" COLOR_RESET);
            file_input_mode(true);
            break;

        case 4:
        default:
            printf(COLOR_RED "Exiting the application...\n" COLOR_RESET);
            return;
        }
    }
}
