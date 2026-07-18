#include <stdlib.h>
#include <string.h>
#include <stdio.h>

const char githubLink[] = "https://github.com/leyo-lang/leyo";

void openGithub(void) {
    size_t maxStrSize = (strlen(githubLink) * sizeof(char)) + (10 * sizeof(char));
    char *command = malloc(maxStrSize);

#ifdef _WIN32
    snprintf(command, maxStrSize, "start %s", githubLink);
#elif __APPLE__
    snprintf(command, maxStrSize, "open %s", githubLink);
#else
    snprintf(command, maxStrSize, "xdg-open %s", githubLink);
#endif

    system(command);
}