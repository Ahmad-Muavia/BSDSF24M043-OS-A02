#include <sys/ioctl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

#define BLUE    "\033[1;34m"
#define GREEN   "\033[1;32m"
#define RED     "\033[1;31m"
#define PINK    "\033[1;35m"
#define REVERSE "\033[7m"
#define RESET   "\033[0m"

void do_ls(const char *dir, int recursive_flag,
           int long_format, int horizontal);
void do_long_ls(const char *dir);
void do_column_ls(const char *dir);
void do_horizontal_ls(const char *dir);
void do_recursive_ls(const char *dir, int long_format,
                     int horizontal);
void print_permissions(mode_t mode);

int compare_names(const void *a, const void *b);
int read_names(const char *dir, char ***names, size_t *count);
void free_names(char **names, size_t count);

const char *get_file_color(const char *name, mode_t mode);
void print_colored_name(const char *name, mode_t mode);

int compare_names(const void *a, const void *b)
{
    const char *name_a = *(const char * const *)a;
    const char *name_b = *(const char * const *)b;

    return strcmp(name_a, name_b);
}

int main(int argc, char *argv[])
{
    int opt;
    int long_format = 0;
    int horizontal = 0;
    int recursive_flag = 0;

    while ((opt = getopt(argc, argv, "lxR")) != -1)
    {
        if (opt == 'l')
            long_format = 1;
        else if (opt == 'x')
            horizontal = 1;
        else if (opt == 'R')
            recursive_flag = 1;
        else
        {
            fprintf(stderr,
                    "Usage: %s [-l] [-x] [-R] [directory]\n",
                    argv[0]);
            return 1;
        }
    }

    if (optind == argc)
    {
        do_ls(".", recursive_flag, long_format, horizontal);
    }
    else
    {
        for (int i = optind; i < argc; i++)
        {
            if (!recursive_flag && argc - optind > 1)
                printf("Directory listing of %s:\n", argv[i]);

            do_ls(argv[i], recursive_flag,
                  long_format, horizontal);

            if (i < argc - 1)
                puts("");
        }
    }

    return 0;
}

void do_ls(const char *dir, int recursive_flag,
           int long_format, int horizontal)
{
    if (recursive_flag)
    {
        do_recursive_ls(dir, long_format, horizontal);
    }
    else if (long_format)
    {
        do_long_ls(dir);
    }
    else if (horizontal)
    {
        do_horizontal_ls(dir);
    }
    else
    {
        do_column_ls(dir);
    }
}

int read_names(const char *dir, char ***names, size_t *count)
{
    DIR *dp = opendir(dir);

    *names = NULL;
    *count = 0;

    if (dp == NULL)
    {
        perror(dir);
        return -1;
    }

    size_t capacity = 0;
    struct dirent *entry;

    while (1)
    {
        errno = 0;
        entry = readdir(dp);

        if (entry == NULL)
        {
            if (errno != 0)
            {
                perror("readdir");
                goto failure;
            }

            break;
        }

        if (entry->d_name[0] == '.')
            continue;

        if (*count == capacity)
        {
            size_t new_capacity =
                capacity == 0 ? 16 : capacity * 2;

            char **temp = realloc(
                *names, new_capacity * sizeof(char *));

            if (temp == NULL)
            {
                perror("realloc");
                goto failure;
            }

            *names = temp;
            capacity = new_capacity;
        }

        (*names)[*count] = strdup(entry->d_name);

        if ((*names)[*count] == NULL)
        {
            perror("strdup");
            goto failure;
        }

        (*count)++;
    }

    closedir(dp);

    if (*count > 1)
        qsort(*names, *count, sizeof(char *), compare_names);

    return 0;

failure:
    closedir(dp);
    free_names(*names, *count);
    *names = NULL;
    *count = 0;

    return -1;
}

void free_names(char **names, size_t count)
{
    for (size_t i = 0; i < count; i++)
        free(names[i]);

    free(names);
}

const char *get_file_color(const char *name, mode_t mode)
{
    if (S_ISLNK(mode))
        return PINK;

    if (S_ISDIR(mode))
        return BLUE;

    if (S_ISFIFO(mode) || S_ISSOCK(mode) ||
        S_ISCHR(mode) || S_ISBLK(mode))
        return REVERSE;

    if (strstr(name, ".tar") != NULL ||
        strstr(name, ".gz") != NULL ||
        strstr(name, ".zip") != NULL)
        return RED;

    if (mode & (S_IXUSR | S_IXGRP | S_IXOTH))
        return GREEN;

    return RESET;
}

void print_colored_name(const char *name, mode_t mode)
{
    printf("%s%s%s", get_file_color(name, mode),
           name, RESET);
}

void print_permissions(mode_t mode)
{
    char p[11];

    if (S_ISDIR(mode))
        p[0] = 'd';
    else if (S_ISLNK(mode))
        p[0] = 'l';
    else if (S_ISCHR(mode))
        p[0] = 'c';
    else if (S_ISBLK(mode))
        p[0] = 'b';
    else if (S_ISFIFO(mode))
        p[0] = 'p';
    else if (S_ISSOCK(mode))
        p[0] = 's';
    else
        p[0] = '-';

    p[1] = (mode & S_IRUSR) ? 'r' : '-';
    p[2] = (mode & S_IWUSR) ? 'w' : '-';
    p[3] = (mode & S_IXUSR) ? 'x' : '-';

    p[4] = (mode & S_IRGRP) ? 'r' : '-';
    p[5] = (mode & S_IWGRP) ? 'w' : '-';
    p[6] = (mode & S_IXGRP) ? 'x' : '-';

    p[7] = (mode & S_IROTH) ? 'r' : '-';
    p[8] = (mode & S_IWOTH) ? 'w' : '-';
    p[9] = (mode & S_IXOTH) ? 'x' : '-';

    p[10] = '\0';

    if (mode & S_ISUID)
        p[3] = (mode & S_IXUSR) ? 's' : 'S';

    if (mode & S_ISGID)
        p[6] = (mode & S_IXGRP) ? 's' : 'S';

    if (mode & S_ISVTX)
        p[9] = (mode & S_IXOTH) ? 't' : 'T';

    printf("%s", p);
}

void do_long_ls(const char *dir)
{
    char **names;
    size_t count;

    if (read_names(dir, &names, &count) == -1)
        return;

    struct stat file_stat;
    char path[4096];
    char date[32];

    for (size_t i = 0; i < count; i++)
    {
        int n = snprintf(path, sizeof(path), "%s/%s",
                         dir, names[i]);

        if (n < 0 || (size_t)n >= sizeof(path))
        {
            fprintf(stderr, "Path too long: %s/%s\n",
                    dir, names[i]);
            continue;
        }

        if (lstat(path, &file_stat) == -1)
        {
            perror(path);
            continue;
        }

        struct passwd *user = getpwuid(file_stat.st_uid);
        struct group *group = getgrgid(file_stat.st_gid);

        char *time_text = ctime(&file_stat.st_mtime);

        if (time_text != NULL)
            snprintf(date, sizeof(date), "%.12s", time_text + 4);
        else
            strcpy(date, "unknown");

        print_permissions(file_stat.st_mode);

        printf(" %2lu %-8s %-8s %8ld %s ",
               (unsigned long)file_stat.st_nlink,
               user ? user->pw_name : "unknown",
               group ? group->gr_name : "unknown",
               (long)file_stat.st_size,
               date);

        print_colored_name(names[i], file_stat.st_mode);
        putchar('\n');
    }

    free_names(names, count);
}

void do_column_ls(const char *dir)
{
    char **names;
    size_t count;

    if (read_names(dir, &names, &count) == -1)
        return;

    if (count == 0)
    {
        free_names(names, count);
        return;
    }

    size_t max_len = 0;

    for (size_t i = 0; i < count; i++)
    {
        size_t len = strlen(names[i]);

        if (len > max_len)
            max_len = len;
    }

    struct winsize ws;
    size_t terminal_width = 80;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 &&
        ws.ws_col > 0)
        terminal_width = ws.ws_col;

    size_t column_width = max_len + 2;
    size_t columns = terminal_width / column_width;

    if (columns < 1)
        columns = 1;

    if (columns > count)
        columns = count;

    size_t rows = (count + columns - 1) / columns;

    for (size_t row = 0; row < rows; row++)
    {
        for (size_t col = 0; col < columns; col++)
        {
            size_t index = col * rows + row;

            if (index >= count)
                continue;

            char path[4096];
            struct stat file_stat;

            int n = snprintf(path, sizeof(path), "%s/%s",
                             dir, names[index]);

            if (n >= 0 && (size_t)n < sizeof(path) &&
                lstat(path, &file_stat) == 0)
            {
                print_colored_name(names[index],
                                   file_stat.st_mode);
            }
            else
            {
                printf("%s", names[index]);
            }

            if (col < columns - 1)
            {
                size_t padding =
                    column_width - strlen(names[index]);

                printf("%*s", (int)padding, "");
            }
        }

        putchar('\n');
    }

    free_names(names, count);
}

void do_horizontal_ls(const char *dir)
{
    char **names;
    size_t count;

    if (read_names(dir, &names, &count) == -1)
        return;

    if (count == 0)
    {
        free_names(names, count);
        return;
    }

    size_t max_len = 0;

    for (size_t i = 0; i < count; i++)
    {
        size_t len = strlen(names[i]);

        if (len > max_len)
            max_len = len;
    }

    struct winsize ws;
    size_t terminal_width = 80;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 &&
        ws.ws_col > 0)
        terminal_width = ws.ws_col;

    size_t column_width = max_len + 2;
    size_t position = 0;

    for (size_t i = 0; i < count; i++)
    {
        size_t len = strlen(names[i]);

        if (position > 0 &&
            position + column_width > terminal_width)
        {
            putchar('\n');
            position = 0;
        }

        char path[4096];
        struct stat file_stat;

        int n = snprintf(path, sizeof(path), "%s/%s",
                         dir, names[i]);

        if (n >= 0 && (size_t)n < sizeof(path) &&
            lstat(path, &file_stat) == 0)
        {
            print_colored_name(names[i], file_stat.st_mode);
        }
        else
        {
            printf("%s", names[i]);
        }

        position += len;

        if (i < count - 1)
        {
            if (column_width < terminal_width)
            {
                size_t padding = column_width - len;

                printf("%*s", (int)padding, "");
                position += padding;
            }
            else
            {
                putchar('\n');
                position = 0;
            }
        }
    }

    putchar('\n');

    free_names(names, count);
}

void do_recursive_ls(const char *dir, int long_format,
                     int horizontal)
{
    char **names;
    size_t count;

    printf("%s:\n", dir);

    if (long_format)
        do_long_ls(dir);
    else if (horizontal)
        do_horizontal_ls(dir);
    else
        do_column_ls(dir);

    putchar('\n');

    if (read_names(dir, &names, &count) == -1)
        return;

    for (size_t i = 0; i < count; i++)
    {
        char path[4096];
        struct stat file_stat;

        int n = snprintf(path, sizeof(path), "%s/%s",
                         dir, names[i]);

        if (n < 0 || (size_t)n >= sizeof(path))
        {
            fprintf(stderr, "Path too long: %s/%s\n",
                    dir, names[i]);
            continue;
        }

        if (lstat(path, &file_stat) == -1)
        {
            perror(path);
            continue;
        }

        if (S_ISDIR(file_stat.st_mode))
            do_recursive_ls(path, long_format, horizontal);
    }

    free_names(names, count);
}
