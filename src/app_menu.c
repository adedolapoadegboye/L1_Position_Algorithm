/**
 * @file app_menu.c
 * @brief Menu interface for selecting RTCM input source in the GNSS application.
 *
 * Provides a terminal menu for choosing how RTCM data is fed into the GNSS L1
 * positioning resolver.
 */

#include "../include/algo.h" // assumes color macros, serial_connect(), file_input_mode()

#define INPUT_BUF_SZ 64

static void print_banner(void)
{
    printf(COLOR_BLUE
           "\n********************************************\n"
           "*         GNSS Positioning Engine          *\n"
           "********************************************\n\n" COLOR_RESET);

    printf(COLOR_YELLOW
           "Note: Only GPS L1 signals from RTCM MSM1 and MSM4 inputs are supported at this time.\n\n" COLOR_RESET);
}

static void print_menu(void)
{
    printf(COLOR_GREEN
           "********** RTCM Input Source Menu **********\n"
           "* 1. Serial Port  (Raw Binary)             *\n"
           "* 2. Pre-recorded File (Raw Binary)        *\n"
           "* 3. Pre-recorded File (Parsed with PyRTCM)*\n"
           "* 4. Exit                                  *\n"
           "********************************************\n" COLOR_RESET);
}

/**
 * @brief Read a line from stdin and parse a menu choice.
 * @return Parsed choice in [1..4], or 4 on EOF to exit gracefully.
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
            // TODO: replace NULL/0 with actual port configuration when available
            serial_connect(NULL, 0);
            break;

        case 2:
            printf(COLOR_GREEN "You selected Pre-recorded File Input (Raw Binary).\n" COLOR_RESET);
            file_input_mode(false);
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
