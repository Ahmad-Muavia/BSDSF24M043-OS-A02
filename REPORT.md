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

### Feature 5: Alphabetical Sorting

**Q1. Why must we read all directory entries into memory before sorting? What are the drawbacks for directories containing millions of files?**

We store all filenames in a dynamic array because `qsort()` needs an array to rearrange the entries. Reading filenames into memory allows us to sort them before displaying them. For millions of files, the array consumes a large amount of memory, requires more processing time, and memory allocation may fail.

**Q2. Explain the purpose and signature of the `qsort()` comparison function. Why does it use `const void *`?**

The comparison function compares two filenames and tells `qsort()` their alphabetical order. Its signature is `int compare_names(const void *a, const void *b)`. It returns a negative value if the first filename comes before the second, zero if they are equal, and a positive value if the first comes after the second. The `const void *` parameters allow `qsort()` to work with different data types. We cast them to string pointers and use `strcmp()` to compare filenames.

### Feature-7: Recursive Listing (-R)

**1. What is a base case in recursion? What stops recursion in our `ls` program?**

A base case is the condition that stops a recursive function from calling itself repeatedly. In our `ls` program, recursion stops when the current directory has no more subdirectories to visit, or when a directory cannot be opened. The program also skips hidden entries, including `.` and `..`, preventing infinite recursion.

**2. Why must we construct the full path before making a recursive call?**

We construct the full path, such as `parent_dir/subdir`, so the program can locate the correct subdirectory. If we call `do_recursive_ls("subdir")` directly, the program searches for that directory relative to the current working directory rather than the parent directory being listed. This can cause it to open the wrong directory or fail to find it.
