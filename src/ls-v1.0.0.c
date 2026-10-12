
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

void do_ls(const char *dir);
void do_long_ls(const char *dir);
void do_column_ls(const char *dir);
void do_horizontal_ls(const char *dir);
void print_permissions(mode_t mode);

int main(int argc, char *argv[])
{
    int opt;
    int long_format = 0;
    int horizontal = 0;

    while ((opt = getopt(argc, argv, "lx")) != -1)
    {
        if (opt == 'l')
            long_format = 1;
        else if (opt == 'x')
            horizontal = 1;
        else
        {
            fprintf(stderr, "Usage: %s [-l] [-x] [directory]\n",
                    argv[0]);
            return 1;
        }
    }

    if (optind == argc)
    {
        if (long_format)
            do_long_ls(".");
        else if (horizontal)
            do_horizontal_ls(".");
        else
            do_column_ls(".");
    }
    else
    {
        for (int i = optind; i < argc; i++)
        {
            if (argc - optind > 1)
                printf("Directory listing of %s:\n", argv[i]);

            if (long_format)
                do_long_ls(argv[i]);
            else if (horizontal)
                do_horizontal_ls(argv[i]);
            else
                do_column_ls(argv[i]);

            if (i < argc - 1)
                puts("");
        }
    }

    return 0;
}

void do_ls(const char *dir)
{
    DIR *dp = opendir(dir);

    if (dp == NULL)
    {
        perror(dir);
        return;
    }

    struct dirent *entry;

    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] != '.')
            printf("%s\n", entry->d_name);
    }

    closedir(dp);
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
    DIR *dp = opendir(dir);

    if (dp == NULL)
    {
        perror(dir);
        return;
    }

    struct dirent *entry;
    struct stat file_stat;
    char path[4096];
    char date[32];

    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

        int n = snprintf(path, sizeof(path), "%s/%s",
                         dir, entry->d_name);

        if (n < 0 || (size_t)n >= sizeof(path))
        {
            fprintf(stderr, "Path too long: %s/%s\n",
                    dir, entry->d_name);
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
        {
            snprintf(date, sizeof(date), "%.12s", time_text + 4);
        }
        else
        {
            strcpy(date, "unknown");
        }

        print_permissions(file_stat.st_mode);

        printf(" %2lu %-8s %-8s %8ld %s %s\n",
               (unsigned long)file_stat.st_nlink,
               user ? user->pw_name : "unknown",
               group ? group->gr_name : "unknown",
               (long)file_stat.st_size,
               date,
               entry->d_name);
    }

    closedir(dp);
}

void do_column_ls(const char *dir)
{
    DIR *dp = opendir(dir);

    if (dp == NULL)
    {
        perror(dir);
        return;
    }

    struct dirent *entry;
    char **names = NULL;
    size_t count = 0;
    size_t capacity = 0;
    size_t max_len = 0;

    while (1)
    {
        errno = 0;
        entry = readdir(dp);

        if (entry == NULL)
        {
            if (errno != 0)
                perror("readdir");
            break;
        }

        if (entry->d_name[0] == '.')
            continue;

        if (count == capacity)
        {
            size_t new_capacity = capacity == 0 ? 16 : capacity * 2;
            char **temp = realloc(names,
                                  new_capacity * sizeof(char *));

            if (temp == NULL)
            {
                perror("realloc");
                goto cleanup_column;
            }

            names = temp;
            capacity = new_capacity;
        }

        names[count] = strdup(entry->d_name);

        if (names[count] == NULL)
        {
            perror("strdup");
            goto cleanup_column;
        }

        size_t len = strlen(names[count]);

        if (len > max_len)
            max_len = len;

        count++;
    }

    if (count == 0)
        goto cleanup_column;

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

            printf("%s", names[index]);

            if (col < columns - 1)
            {
                size_t padding =
                    column_width - strlen(names[index]);

                printf("%*s", (int)padding, "");
            }
        }

        putchar('\n');
    }

cleanup_column:
    for (size_t i = 0; i < count; i++)
        free(names[i]);

    free(names);
    closedir(dp);
}

void do_horizontal_ls(const char *dir)
{
    DIR *dp = opendir(dir);

    if (dp == NULL)
    {
        perror(dir);
        return;
    }

    struct dirent *entry;
    char **names = NULL;
    size_t count = 0;
    size_t capacity = 0;
    size_t max_len = 0;

    while (1)
    {
        errno = 0;
        entry = readdir(dp);

        if (entry == NULL)
        {
            if (errno != 0)
                perror("readdir");
            break;
        }

        if (entry->d_name[0] == '.')
            continue;

        if (count == capacity)
        {
            size_t new_capacity = capacity == 0 ? 16 : capacity * 2;
            char **temp = realloc(names,
                                  new_capacity * sizeof(char *));

            if (temp == NULL)
            {
                perror("realloc");
                goto cleanup_horizontal;
            }

            names = temp;
            capacity = new_capacity;
        }

        names[count] = strdup(entry->d_name);

        if (names[count] == NULL)
        {
            perror("strdup");
            goto cleanup_horizontal;
        }

        size_t len = strlen(names[count]);

        if (len > max_len)
            max_len = len;

        count++;
    }

    if (count == 0)
        goto cleanup_horizontal;

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

        printf("%s", names[i]);
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

cleanup_horizontal:
    for (size_t i = 0; i < count; i++)
        free(names[i]);

    free(names);
    closedir(dp);
}
