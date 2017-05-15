//
// Created by kajvi on 2017-05-15.
//
#include "Generic.h"

static void flushRestOfLine(void)
{
    // Call ONLY when EOL expected from the keyboardstream.

    char ch;
    do
    {
        scanf("%c", &ch);
        //printf_s("\n%d\n", ch);
    } while (ch != '\n');
}// flushRestOfLine

// ============================================================================

// Print prompt: wait for "y" or "n" to be entered and return 1 = yes , 0 = no
uint yesNoRepeater(char ir_prompt[])
{
    char repeatChar;
    do
    {
        // Loopar tills man väljer y eller n.

        printf("%s (y/n): ", ir_prompt);
        scanf("%c", &repeatChar);

        // Tömmer resten av In-strömmen från tangentbordet fram till radslut.
        flushRestOfLine();

        if (repeatChar == 'y' || repeatChar == 'Y')
        {
            return 1;
        }
        else if (repeatChar == 'n' || repeatChar == 'N')
        {
            return 0;
        }
        else
        {
            printf("Please enter either y (yes) or n (no)\n");
        }
    } while (1);
} // yesNoRepeater