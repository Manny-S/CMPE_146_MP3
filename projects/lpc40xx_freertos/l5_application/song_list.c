#include <string.h>

#include "ff.h"
#include "song_list.h"
#include <stdio.h>

static song_memory_t list_of_songs[32];
static size_t number_of_songs;

static void song_list__handle_filename(const char *filename) {
  // This will not work for cases like "file.mp3.zip"
  if (NULL != strstr(filename, ".mp3")) {
    // printf("Filename: %s\n", filename);

    // Dangerous function: If filename is > 128 chars, then it will copy extra
    // bytes leading to memory corruption strcpy(list_of_songs[number_of_songs],
    // filename);

    // Better: But strncpy() does not guarantee to copy null char if max length
    // encountered So we can manually subtract 1 to reserve as NULL char
    strncpy(list_of_songs[number_of_songs], filename,
            sizeof(song_memory_t) - 1);

    // Best: Compensates for the null, so if 128 char filename, then it copies
    // 127 chars, AND the NULL char snprintf(list_of_songs[number_of_songs],
    // sizeof(song_memory_t), "%.149s", filename);

    ++number_of_songs;
    // or
    // number_of_songs++;
  }
}

void song_list__populate(void) {
  FRESULT res;
  static FILINFO file_info;
  const char *root_path = "/";

  DIR dir;
  res = f_opendir(&dir, root_path);

  if (res == FR_OK) {
    for (;;) {
      res = f_readdir(&dir, &file_info); /* Read a directory item */
      if (res != FR_OK || file_info.fname[0] == 0) {
        break; /* Break on error or end of dir */
      }

      if (file_info.fattrib & AM_DIR) {
        /* Skip nested directories, only focus on MP3 songs at the root */
      } else { /* It is a file. */
        song_list__handle_filename(file_info.fname);
      }
    }
    f_closedir(&dir);
  }
}

size_t song_list__get_item_count(void) { return number_of_songs; }

const char *song_list__get_name_for_item(size_t item_number) {
  const char *return_pointer = "";

  if (item_number >= number_of_songs) {
    return_pointer = "";
  } else {
    return_pointer = list_of_songs[item_number];
  }

  return return_pointer;
}

Metadata song_list__get_metadata(char *name) {
  // songname_t name;
  char chunk;

  FIL file;
  FRESULT result;
  UINT readCount;
  Metadata meta;
  printf("Fetching metadata for: %s\n", name);

  result = f_open(&file, name, FA_READ);
  if (FR_OK == result) {
    // fseek(file, -128,SEEK_END);
    // move to 128 bytes before end of file to read tags
    f_lseek(&file, f_size(&file) - 128);
    while (!f_eof(&file)) {
      // f_read(&file, chunk, sizeof(songbyte_t), &readCount);

      // for (int i = 0; i < 3; i++) {
      f_read(&file, (void *)meta.tagName, sizeof(chunk) * 3,
             &readCount); // tag
                          // meta.tagName[i] = (char)chunk;
      //}

      f_read(&file, (void *)meta.title, sizeof(chunk) * 30, &readCount);
      /*for (int i = 0; i < 30; i++) {
        f_read(&file, (char)chunk, sizeof(chunk), &readCount); // title
        meta.title[i] = (char)chunk;
      }*/

      f_read(&file, (void *)meta.artist, sizeof(chunk) * 30, &readCount);
      f_read(&file, (void *)meta.album, sizeof(chunk) * 30, &readCount);
      f_read(&file, (void *)meta.year, sizeof(chunk) * 4, &readCount);
      /*
      f_read(&file, chunk, sizeof(chunk) * 30, &readCount); // artist
      meta.artist = chunk;
      f_read(&file, chunk, sizeof(chunk) * 30, &readCount); // album
      meta.album = chunk;
      f_read(&file, chunk, sizeof(chunk) * 4, &readCount); // year
      meta.year = chunk;*/
    }
    f_close(&file);
  } else {
    printf("ERROR: File not found\n");
  }
  return meta;
}