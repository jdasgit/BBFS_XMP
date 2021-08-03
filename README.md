# BBFS_XMP

Two main references were used for this codebase.
The fusexmp.c was taken from libFuse examples and a passthrough was taken.
BBFS was used to mount a user specified directory instead of /

To compile the program ,do: make
To clean , do: make clean


To execute,
1. For Foreground operation, do: ./fusexmp -f <ROOT_DIR> <MOUNT_DIR>
2. For background operation, do: ./fusexmp <ROOT_DIR> <MOUNT_DIR>

To check the mount, do: mount
To unmount, do: fusermount -u <MOUNT_DIR>
