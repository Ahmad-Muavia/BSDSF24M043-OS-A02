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
