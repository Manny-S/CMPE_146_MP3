// @file: song_list.h

#pragma once

#include <stddef.h> // size_t

typedef char song_memory_t[128];

/* Do not declare variables in a header file */
#if 0
static song_memory_t list_of_songs[32];
static size_t number_of_songs;
#endif

typedef struct {
  char tagName[4];
  char title[31];
  char artist[31];
  char album[31];
  char year[5];
  // char comment[30];
  // int genre;
} Metadata;

void song_list__populate(void);
size_t song_list__get_item_count(void);
const char *song_list__get_name_for_item(size_t item_number);

Metadata song_list__get_metadata(char *name);