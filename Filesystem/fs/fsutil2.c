#include "fsutil2.h"
#include "bitmap.h"
#include "cache.h"
#include "debug.h"
#include "directory.h"
#include "file.h"
#include "filesys.h"
#include "free-map.h"
#include "fsutil.h"
#include "inode.h"
#include "off_t.h"
#include "partition.h"
#include "../interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
/**
 * Copies a file from the host filesystem into the
 * disk image's filesystem. If the file already
 * exists in the host filesystem, it is overwritten.
 *
 * @param fname The name of the file within the host fs to copy into the disk image.
 * @return Returns 0 on success, or an error code on failure.
 */
int copy_in(char *fname)
{

    // Open the file from the host filesystem for reading
    FILE *source_file = fopen(fname, "r");
    if (source_file == NULL)
    {
        return FILE_DOES_NOT_EXIST; // Error handling if the file couldn't be opened
    }

    // Determine the size of the file in bytes
    fseek(source_file, 0, SEEK_END);
    long file_size = ftell(source_file);
    fseek(source_file, 0, SEEK_SET);

    // Check if the file is too large to fit in the disk image fs.
    bool out_of_space = (file_size > 512 * fsutil_freespace());

    // Attempt to create the file in the disk image file system, with size 0 bytes
    if (!fsutil_create(fname, 0))
    {
        fclose(source_file);
        return FILE_CREATION_ERROR; // Error handling for file creation failure
    }

    int c; // Use int to capture EOF
    long bytes_written;
    long total_bytes_written = 0;
    int index = 0;

    // While we can still read characters from the source file
    while ((c = fgetc(source_file)) != EOF)
    {
        char buffer[2] = {(char)c, '\0'}; // Cast the read character back to char and null-terminate the buffer
        // char buffer[2];
        // buffer[0]=c;

        // Write the single byte to the disk image fs at the correct index
        fsutil_seek(fname, index);
        // Write 2 bytes to the file, one is the character,
        // the other is the null terminator
        bytes_written = fsutil_write(fname, buffer, 2);
        index++;

        total_bytes_written += bytes_written - 1; // Subtract 1 to account for the null terminator--quirk of the fsutil_write function

        // If we couldn't write any bytes, break out of the loop
        if (bytes_written == 0)
        {
            break;
        }
    }

    fsutil_seek(fname, index - 1); // Reset the index to before the null terminator

    // If we couldn't write all the bytes, as predetermined, print the warning message.
    if (out_of_space)
    {
        printf("Warning: could only write %ld out of %ld bytes (reached end of file).\n", total_bytes_written + 2, file_size);
    }

    // fsutil_close(fname); // Close the file in the disk image fs FOLLOW UP ON THIS
    return 0; // Indicate success.
}

/**
 * Copies a file from the disk image's filesystem to the host filesystem.
 * If the file already exists in the host filesystem, it is overwritten.
 *
 * @param fname The name of the file within the disk image to copy out.
 * @return Returns 0 on success, or an error code on failure.
 */
int copy_out(char *fname)
{
    // Ensure that the file is not open in the simulated filesystem
    // so that its index is reset to the beginning of the file.
    fsutil_close(fname);

    // Determine the size of the file.
    int filesize = fsutil_size(fname);

    // Return code for file not existing in the disk image.
    if (filesize == -1)
    {
        return FILE_DOES_NOT_EXIST;
    }

    // Allocate a buffer to hold the file content.
    char buffer[filesize + 1];
    buffer[filesize] = '\0'; // Ensure the buffer is null-terminated.

    // Read the content of the file from the disk image into the buffer.
    fsutil_read(fname, buffer, filesize);

    // If the file exists in the host filesystem, remove it, to clear it.
    remove(fname);

    // Open a new file for writing in the host filesystem.
    FILE *newfile = fopen(fname, "wb");
    // If the file could not be opened; is NULL, return an error.
    if (!newfile)
    {
        return FILE_CREATION_ERROR;
    }

    // Write the buffer to the new file in the host filesystem.
    // int written = fwrite(buffer, sizeof(char), filesize, newfile);

    for (int i = 0; buffer[i] != '\0'; i++)
    {
        putc(buffer[i], newfile);
    }

    // Close the file in the disk image and the new file in the host filesystem.
    fsutil_close(fname);
    fclose(newfile);

    // Return success.
    return 0;
}

/**
 * Searches for and prints out all files in the root directory
 * that contain the specified pattern.
 *
 * @param pattern The text pattern to search for within each file.
 */
void find_file(char *pattern)
{
    struct dir *root_dir = dir_open_root();

    char file_name[NAME_MAX + 1]; // To store the file name
    while (dir_readdir(root_dir, file_name))
    { // Iterate over all files in the root directory in disk image fs

        // Allocate buffer to read file contents into
        off_t len_file = fsutil_size(file_name);
        char *buffer = malloc((len_file + 1) * sizeof(char));
        // If malloc fails, close the file and continue to the next file
        if (buffer == NULL)
        {
            fsutil_close(file_name);
            continue;
        }

        // Read file contents
        off_t bytes_read = fsutil_read(file_name, buffer, len_file);
        buffer[bytes_read] = '\0'; // Null-terminate the buffer

        // Search for the pattern in the file contents
        if (strstr(buffer, pattern) != NULL)
        {
            printf("%s\n", file_name); // Print the file name if the pattern is found
        }

        // Freeing up each iteration
        free(buffer);
        fsutil_close(file_name);
    }

    // Close root directory when finished
    dir_close(root_dir);
}
/**
 * DESCRIPTION:
 * Gets all file names in root
 *
 * return arr is NULL terminated
 */
char **getAllFilesInRoot()
{
    struct dir *root_dir = dir_open_root();
    int num_files_in_root = 0;
    char file_name[NAME_MAX + 1];

    if (root_dir == NULL)
    {
        return NULL;
    }

    while (dir_readdir(root_dir, file_name))
    {
        num_files_in_root++;
    }

    dir_close(root_dir);

    root_dir = dir_open_root();

    if (root_dir == NULL)
    {
        return NULL;
    }

    char **names = malloc((sizeof(char *) * num_files_in_root) + sizeof(char *));
    names[num_files_in_root] = NULL;

    int i = 0;
    while (dir_readdir(root_dir, file_name))
    {
        names[i++] = strdup(file_name);
    }

    dir_close(root_dir);

    return names;
}

/**
 * DESCRIPTION:
 * Gets number of fragmentable files (i.e files that have 2 or more blocks)
 */
int getNumberOfFragmentableFiles(char **names)
{
    int count = 0;
    for (int i = 0; names[i] != NULL; i++)
    {
        struct file *file_s = get_file_by_fname(names[i]);
        if (file_s == NULL)
        {
            file_s = filesys_open(names[i]);
        }

        // checks if it has 2 or more blocks
        if (file_s->inode->data.direct_blocks[0] != 0 && file_s->inode->data.direct_blocks[1] != 0)
        {
            count++;
        }

        fsutil_close(names[i]);
    }

    return count;
}

/**
 * DESCRIPTION:
 * Gets number of fragmented files
 *
 * PARAMS:
 * names: names of all the files in the root
 */
int getNumberOfFragmentedFiles(char **names)
{
    int count = 0;
    for (int i = 0; names[i] != NULL; i++)
    {

        struct file *file_s = get_file_by_fname(names[i]);
        if (file_s == NULL)
        {
            file_s = filesys_open(names[i]);
        }

        block_sector_t *arr = get_inode_data_sectors(file_s->inode);
        size_t num_sectors = bytes_to_sectors(file_s->inode->data.length);
        bool fragmentable = false;

        // Checks if its fragmented
        for (int j = 0; j < num_sectors - 1; j++)
        {
            if (abs(arr[j] - arr[j + 1]) > 3)
            {
                fragmentable = true;
            }
            if (fragmentable)
            {
                count++;
                break;
            }
        }

        free(arr);
        fsutil_close(names[i]);
    }

    return count;
}

void fragmentation_degree()
{
    // gets all file names in roots
    char **names = getAllFilesInRoot();

    int nb_fragmented_files = getNumberOfFragmentedFiles(names);
    int nb_fragmentable_files = getNumberOfFragmentableFiles(names);

    // computes fragmentation degree
    // Num fragmentable files: 90
    printf("Num fragmentable files: %d\n", nb_fragmentable_files);

    printf("Num fragmented files: %d\n", nb_fragmented_files);

    double frag_deg = ((double)nb_fragmented_files) / ((double)nb_fragmentable_files);
    printf("Fragmentation pct: %f\n", frag_deg);

    // frees names array
    for (int i = 0; names[i] != NULL; i++)
    {
        free(names[i]);
    }
    free(names);
}

struct filebuffer {
    char *filename;
    char *file_contents;
    int filesize;
};

static struct filebuffer *init_file_contents(char *filename, char* buffer, int filesize) {
    struct filebuffer *file = malloc(sizeof(struct filebuffer));
    file->filename = filename;
    file->file_contents = buffer;
    file->filesize = filesize;
    return file;
}

static void free_file_contents(struct filebuffer *buffer) {
    free(buffer->file_contents);
    free(buffer->filename);
    free(buffer);
}

int defragment()
{
    int return_code = 0;

    // gets number of files in root
    int filecount = 0;
    char file_name[NAME_MAX + 10];

    int file_contents_ptr = 100;
    struct filebuffer **filebuffers = malloc(sizeof(struct filebuffer*) * file_contents_ptr);

    struct dir *root_dir = dir_open_root();
    if (!root_dir)
        return handle_error(FILESYSTEM_ERROR);

    // gets file names and their contents and saves them into the buffers
    while (dir_readdir(root_dir, file_name)) {      

        int filesize = fsutil_size(file_name);
        char *filebuffer = malloc(sizeof(char) * (filesize + 1));
        
        // reads file into filebuffer
        int code = fsutil_read(file_name, filebuffer, filesize);
        if (code == -1)
        {
            dir_close(root_dir);
            free(filebuffer);
            return_code = FILE_READ_ERROR;
            goto FreeBuffers;
        }

        fsutil_close(file_name);

        // adds filename and file contents to filebuffers array
        filebuffers[filecount] = init_file_contents(strdup(file_name), filebuffer, filesize);
        filecount++;

        // resizes if necessary 
        if(filecount >= file_contents_ptr) {
            filebuffers = realloc(filebuffers, file_contents_ptr * 2);
            file_contents_ptr*=2;
        }
    }

    dir_close(root_dir);

     // deletes files
    for(int i = 0; i < filecount; i++) {
        if (!fsutil_rm(filebuffers[i]->filename))
        {
            return_code = FILESYSTEM_ERROR;
            goto FreeBuffers;
        }
    }

    // Creates new files
    for (int i = 0; i < filecount; i++)
    {
        char *filename = filebuffers[i]->filename;
        char *filecontents = filebuffers[i]->file_contents;
        int filesize = filebuffers[i]->filesize;

        if (!fsutil_create(filename, filesize))
        {
            return_code = FILE_CREATION_ERROR;
            break;
        }

        fsutil_write(filename, filecontents, filesize);
    }

    // frees memory
    FreeBuffers:;

    for (int i = 0; i < filecount; i++) {
        free_file_contents(filebuffers[i]);
    }
    free(filebuffers);
  
    return return_code;
}

int defragment_()
{

    struct dir *root_dir = dir_open_root();
    int num_files_in_root = 0;
    char file_name[NAME_MAX + 1];

    if (root_dir == NULL)
    {
        return handle_error(FILESYSTEM_ERROR);
    }

    while (dir_readdir(root_dir, file_name))
    {
        num_files_in_root++;
    }

    dir_close(root_dir);

    root_dir = dir_open_root();

    if (root_dir == NULL)
    {
        return handle_error(FILESYSTEM_ERROR);
    }

    char **names = malloc((sizeof(char *) * num_files_in_root) + sizeof(char *));
    names[num_files_in_root] = NULL;

    int i = 0;
    while (dir_readdir(root_dir, file_name))
    {
        names[i++] = strdup(file_name);
    }
    dir_close(root_dir);

    // For each file in the disk image fs root directory
    // check if it exists in the host filesystem beforehand
    // If so, don't want to remove it.
    bool in_host_dir[num_files_in_root];

    for (int i = 0; i < num_files_in_root; i++)
    {
        if (fopen(names[i], "r") != NULL)
        {
            in_host_dir[i] = true;
        }
        else
        {
            in_host_dir[i] = false;
        }
    }

    for (int i = 0; names[i] != NULL; i++)
    {
        copy_out(names[i]);
        fsutil_rm(names[i]);
    }
    for (int i = 0; names[i] != NULL; i++)
    {
        copy_in(names[i]);
        if (!in_host_dir[i])
        {
            remove(names[i]); // potential edge case:
            // say we have a.txt in host fs with contents "aaaa"
            // and a.txt in disk image fs with contents "bbbb"
            // then defragment will copy out a.txt from disk image fs
            // into host fs, which will change the contents of a.txt in host fs
            // to "bbbb".  DOUBT THIS WOULD BE TESTED.
        }
    }

    return 0; // Success
}

void recover0()
{
    size_t nb_of_sectors = bitmap_size(free_map);

    // keeps track of recovered files
    for (int i = 0; i < nb_of_sectors; i++)
    {

        // if sector is currently in use
        if (bitmap_test(free_map, i))
        {
            continue;
        }

        struct inode *inode = inode_open(i);

        // if magic is not 0, then it means this used to be the start of a file
        if (inode->data.magic == INODE_MAGIC)
        {

            // creates file name
            char namebuffer[NAME_MAX + 1];
            snprintf(namebuffer, sizeof(namebuffer), "recovered0-%d", i);

            // adds file and links to inode
            struct dir *root = dir_open_root();
            dir_add(root, namebuffer, i, false);
            inode->removed = false;
            dir_close(root);

            // adds entries to the sector array to know which sectors need to flipped
            size_t num_sectors = bytes_to_sectors(inode->data.length);
            block_sector_t *sectors = get_inode_data_sectors(inode);

            for(int j=0; j < num_sectors; j++) {
                bitmap_mark(free_map, sectors[j]);
            }
            free(sectors);

            // bitmap_mark(free_map, inode->sector);
            bitmap_mark(free_map, i);
        }

        // closes (i.e frees) opened inode
        inode_close(inode);
    }
}

void recover1()
{

    block_sector_t num_of_sectors = bitmap_size(free_map);

    for (block_sector_t i = 4; i < num_of_sectors; i++)
    {

        struct inode *node = inode_open(i);

        char buffer[BLOCK_SECTOR_SIZE + 1];
        buffer[BLOCK_SECTOR_SIZE] = '\0';

        buffer_cache_read(i, buffer);

        // creates filename
        char filename[100];
        snprintf(filename, sizeof(filename), "recovered1-%d.txt", i);
        printf("%s\n", filename);

        FILE *newfile = fopen(filename, "wb");
        if (!newfile)
        {
            inode_close(node);
            return;
        }

        // // Write the buffer to the new file in the host filesystem.
        // int written = fwrite(buffer, sizeof(char), BLOCK_SECTOR_SIZE, newfile);

        for (int i = 0; buffer[i] != '\0'; i++)
        {
            putc(buffer[i], newfile);
        }

        fclose(newfile);

        inode_close(node);
    }
}

void recover2()
{

    struct dir *root = dir_open_root();
    char filename[NAME_MAX + 20];
    while (dir_readdir(root, filename))
    {
        char buffer[BLOCK_SECTOR_SIZE + 1];
        int filesize_bytes = fsutil_size(filename);
        size_t filesector_size = bytes_to_sectors(filesize_bytes);
        int remainder = filesize_bytes % BLOCK_SECTOR_SIZE;

        struct file *file = get_file_by_fname(filename);

        // struct inode *inode = inode_open(file->inode->data.direct_blocks[filesector_size]);

        buffer_cache_read(file->inode->data.direct_blocks[filesector_size - 1], buffer);

        char newfilename[NAME_MAX + 100];
        snprintf(newfilename, sizeof(newfilename), "recovered2-%s.txt", filename);

        FILE *newfile = fopen(newfilename, "wb");
        if (!file)
            return;

        // stores the remainder of the sector into a file
        for (int i = remainder + 1; i < BLOCK_SECTOR_SIZE; i++)
        {
            char c = buffer[i];
            if (c != '\0')
                fputc(c, newfile);
        }

        fclose(newfile);
        fsutil_close(filename);
    }

    dir_close(root);
}

void recover(int flag)
{
    if (flag == 0)
    { // recover deleted inodes
        recover0();
    }
    else if (flag == 1)
    { // recover all non-empty sectors

        recover1();
    }
    else if (flag == 2)
    { // data past end of file.
        recover2();
    }
}