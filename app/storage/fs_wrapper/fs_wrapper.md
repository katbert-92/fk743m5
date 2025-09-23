# File System Wrapper

## General

File System Wrapper (FSW) implements a high-level abstraction of most operations that can be done with files and directories.

## File System Type Registration

The registration routine as well as some preparation that FSW needs to work properly are part of `FsWrap_Init` function.  
`FsWrap_Init` is the only function needs to be called to use FSW.

Since the only thing which FSW needed is at least one type of file system registered, `FsWrap_Init` simply calls `FsWrap_Register` for the only one FS implementation exists in the project by far - FAT FS.
There is a possibility to unregister the FS by calling `FsWrap_Unregister` (but this possibility is questionable *at best*).

## Mounting File System

To create a file system, `FsWrap_Mount` needs to be called. No addition function calls have to be done in order to use mounted FS.

In terms of FS operations, `FsWrap_Mount` **only runs** a `mount` function which is reponsible for creation of a drive; `mount` needs to be provided in specific FS implementation.  
Though, the FAT FS implementation **not only mounts** the drive (`f_mount`), **it also checks whether the actual FS is present** on this drive. If not, it also creates one (`f_mkfs`).  
It is recommended to do the same for all implementations in future.

To (possibly) destroy existing FS and create a fresh one, `FsWrap_Mkfs` function can be used.

## Working with entities

There are two types of entites which FSW currently supports: *files* and *directories*.

To work with *files*, the following functions are implemented:

- `FsWrap_Open`
- `FsWrap_Close`
- `FsWrap_Read`
- `FsWrap_Write`
- `FsWrap_Seek`
- `FsWrap_Tell`
- `FsWrap_Truncate`
- `FsWrap_Sync`

Working with *directories* made possible by following functions:

- `FsWrap_Mkdir`
- `FsWrap_OpenDir`
- `FsWrap_ReadDir`
- `FsWrap_CloseDir`

Also, there are some auxilarry functions which can be called either for *file* or *directory* enitites:

- `FsWrap_Unlink`
- `FsWrap_Rename`
- `FsWrap_Stat`

The information of the certain file system can be collected via `FsWrap_StatVFS` function.

The interface of all of these function is self-explanatory.  
The only thing to mention there is to make sure that `FsWrap_File_t` and `FsWrap_Dir_t` instances have been **set to zeroes** before passing them to `FsWrap_Open` and `FsWrap_OpenDir` function respectively. If this is not done, mentioned function will reject the operation since the objects passed to them may well already contain some data.  

Speaking of implementations, working with FAT FS through the `FsWrap_Open` and `FsWrap_OpenDir` functions will use dynamic memory allocation for file and directory instances.
