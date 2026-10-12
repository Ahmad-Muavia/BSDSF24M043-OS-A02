# Feature 2: Long Listing Format

## 1. Difference between `stat()` and `lstat()`

Both functions retrieve file information, such as permissions, size, owner, and modification time.

- `stat()` follows symbolic links and returns information about the file they point to.
- `lstat()` returns information about the symbolic link itself.

In an `ls` implementation, `lstat()` is useful because it allows symbolic links to be identified and displayed with the correct file type and permissions.

## 2. Understanding `st_mode` and bitwise operators

`st_mode` is a field in the `struct stat` structure. It stores the file type and permission bits.

Examples of file-type macros:
- `S_IFDIR`: identifies a directory.
- `S_IFREG`: identifies a regular file.

Examples of permission macros:
- `S_IRUSR`: owner has read permission.
- `S_IWUSR`: owner has write permission.
- `S_IXUSR`: owner has execute permission.

The bitwise AND operator (`&`) checks whether a particular permission bit is set. For example, `mode & S_IRUSR` checks whether the owner has read permission.

These values are used to construct the permission string, such as `-rw-r--r--` or `drwxr-xr-x`.

## Feature 3: Column Display (Down Then Across)

### 1. Explain the logic for down-then-across printing. Why is a single loop insufficient?

The program first reads all filenames into a dynamically allocated array and records the longest filename. It then detects the terminal width using `ioctl()` and calculates the number of columns and rows.

To print down then across, the program calculates each filename's array index using the column number multiplied by the number of rows, plus the row number.

A simple loop through the array would print filenames sequentially, producing rows in across-first order. Calculating the index allows the program to print entries vertically within each column before moving to the next column.

### 2. What is the purpose of `ioctl()`? What are the limitations of a fixed 80-column width?

The `ioctl()` system call obtains terminal information. With `TIOCGWINSZ`, the program can retrieve the terminal's width and adjust its column layout accordingly.

If the program always uses a fixed width of 80 characters, the output may not fit on wider or narrower terminals. It may leave unused space on wide terminals or produce poorly aligned columns on narrow terminals. Detecting the actual terminal width makes the output more adaptable.

## Feature 4: Horizontal Column Display (-x)

### 1. Compare down-then-across and horizontal printing

The down-then-across layout requires more pre-calculation because it calculates the number of columns and rows before printing filenames. It uses the formula `index = col * rows + row` to display filenames vertically within each column.

The horizontal layout is simpler. It processes filenames in sequence from left to right, tracks the current screen position, and starts a new line when the terminal width would be exceeded.

### 2. Managing display modes

I used two integer variables: `long_format` and `horizontal`. The `getopt()` function recognizes the `-l` and `-x` options. If `-l` is selected, the program calls `do_long_ls()`. If `-x` is selected, it calls `do_horizontal_ls()`. If neither option is selected, it calls `do_column_ls()` for the default down-then-across layout.
