/**
 * MIT License
 *
 * Copyright (c) 2020 Leonard Iklé
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * This is a simple alternative cross-platform implementation of getopt, which
 * is used to parse argument strings submitted to the executable (argc and argv
 * which are received in the main function).
 */

#include <assert.h>
#include <memory.h>
#include <stdio.h>
#include <string.h>

#include <CL/Utils/cargs.h>

static void cag_option_print_value(const cag_option *option,
  int *accessor_length, FILE *destination)
{
  if (option->value_name != NULL) {
    *accessor_length += fprintf(destination, "=%s", option->value_name);
  }
}

static void cag_option_print_letters(const cag_option *option, bool *first,
  int *accessor_length, FILE *destination)
{
  const char *access_letter;
  access_letter = option->access_letters;
  if (access_letter != NULL) {
    while (*access_letter) {
      if (*first) {
        *accessor_length += fprintf(destination, "-%c", *access_letter);
        *first = false;
      } else {
        *accessor_length += fprintf(destination, ", -%c", *access_letter);
      }
      ++access_letter;
    }
  }
}

static void cag_option_print_name(const cag_option *option, bool *first,
  int *accessor_length, FILE *destination)
{
  if (option->access_name != NULL) {
    if (*first) {
      *accessor_length += fprintf(destination, "--%s", option->access_name);
    } else {
      *accessor_length += fprintf(destination, ", --%s", option->access_name);
    }
  }
}

UTILS_EXPORT
void cag_option_print(const cag_option *options, size_t option_count,
  FILE *destination)
{
  size_t option_index;
  const cag_option *option;
  bool first;
  int i, accessor_length, max_accessor_length = 0;

  FILE * tmp;
  if (!tmpfile_s(&tmp)) {
    for (option_index = 0; option_index < option_count; ++option_index) {
      option = &options[option_index];
      accessor_length = 0;
      first = true;

      cag_option_print_letters(option, &first, &accessor_length, tmp);
      cag_option_print_name(option, &first, &accessor_length, tmp);
      cag_option_print_value(option, &accessor_length, tmp);

      if (accessor_length > max_accessor_length)
        max_accessor_length = accessor_length;
    }
  }

  for (option_index = 0; option_index < option_count; ++option_index) {
    option = &options[option_index];
    accessor_length = 0;
    first = true;

    fputs("  ", destination);

    cag_option_print_letters(option, &first, &accessor_length, destination);
    cag_option_print_name(option, &first, &accessor_length, destination);
    cag_option_print_value(option, &accessor_length, destination);

    for (i = accessor_length; i < max_accessor_length; ++i) {
      fputs(" ", destination);
    }

    fputs("   ", destination);
    fputs(option->description, destination);

    fprintf(destination, "\n");
  }
}

UTILS_EXPORT
void cag_option_prepare(cag_option_context *context, const cag_option *options,
  size_t option_count, int argc, char **argv)
{
  // This just initialized the values to the beginning of all the arguments.
  context->options = options;
  context->option_count = option_count;
  context->argc = argc;
  context->argv = argv;
  context->index = 1;
  context->inner_index = 0;
  context->forced_end = false;
}

static const cag_option *cag_option_find_by_name(cag_option_context *context,
  char *name, size_t name_size)
{
  const cag_option *option;
  size_t i;

  // We loop over all the available options and stop as soon as we have found
  // one. We don't use any hash map table, since there won't be that many
  // arguments anyway.
  for (i = 0; i < context->option_count; ++i) {
    option = &context->options[i];

    // The option might not have an item name, we can just skip those.
    if (option->access_name == NULL) {
      continue;
    }

    // Try to compare the name of the access name. We can use the name_size or
    // this comparison, since we are guaranteed to have null-terminated access
    // names.
    if (strncmp(option->access_name, name, name_size) == 0) {
      return option;
    }
  }

  return NULL;
}

static const cag_option *cag_option_find_by_letter(cag_option_context *context,
  char letter)
{
  const cag_option *option;
  size_t i;

  // We loop over all the available options and stop as soon as we have found
  // one. We don't use any look up table, since there won't be that many
  // arguments anyway.
  for (i = 0; i < context->option_count; ++i) {
    option = &context->options[i];

    // If this option doesn't have any access letters we will skip them.
    if (option->access_letters == NULL) {
      continue;
    }

    // Verify whether this option has the access letter in it's access letter
    // string. If it does, then this is our option.
    if (strchr(option->access_letters, letter) != NULL) {
      return option;
    }
  }

  return NULL;
}

static void cag_option_parse_value(cag_option_context *context,
  const cag_option *option, char **c)
{
  // And now let's check whether this option is supposed to have a value, which
  // is the case if there is a value name set. The value can be either submitted
  // with a '=' sign or a space, which means we would have to jump over to the
  // next argv index. This is somewhat ugly, but we do it to behave the same as
  // the other option parsers.
  if (option->value_name != NULL) {
    if (**c == '=') {
      context->value = ++(*c);
    } else {
      // If the next index is larger or equal to the argument count, then the
      // parameter for this option is missing. The user will know about this,
      // since the value pointer of the context will be NULL because we don't
      // set it here in that case.
      if (context->argc > context->index + 1) {
        // We consider this argv to be the value, no matter what the contents
        // are.
        ++context->index;
        *c = context->argv[context->index];
        context->value = *c;
      }
    }

    // Move c to the end of the value, to not confuse the caller about our
    // position.
    while (**c) {
      ++(*c);
    }
  }
}

static void cag_option_parse_access_name(cag_option_context *context, char **c)
{
  const cag_option *option;
  char *n;

  // Now we need to extract the access name, which is any symbol up to a '=' or
  // a '\0'.
  n = *c;
  while (**c && **c != '=') {
    ++*c;
  }

  // Now this will obviously always be true, but we are paranoid. Sometimes. It
  // doesn't hurt to check.
  assert(*c >= n);

  // Figure out which option this name belongs to. This might return NULL if the
  // name is not registered, which means the user supplied an unknown option. In
  // that case we return true to indicate that we finished with this option. We
  // have to skip the value parsing since we don't know whether the user thinks
  // this option has one or not. Since we don't set any identifier specifically,
  // it will remain '?' within the context.
  option = cag_option_find_by_name(context, n, (size_t)(*c - n));
  if (option == NULL) {
    // Since this option is invalid, we will move on to the next index. There is
    // nothing we can do about this.
    ++context->index;
    return;
  }

  // We found an option and now we can specify the identifier within the
  // context.
  context->identifier = option->identifier;

  // And now we try to parse the value. This function will also check whether
  // this option is actually supposed to have a value.
  cag_option_parse_value(context, option, c);

  // And finally we move on to the next index.
  ++context->index;
}

static void cag_option_parse_access_letter(cag_option_context *context,
  char **c)
{
  const cag_option *option;
  char *n = *c;
  char *v;

  // Figure out which option this letter belongs to. This might return NULL if
  // the letter is not registered, which means the user supplied an unknown
  // option. In that case we return true to indicate that we finished with this
  // option. We have to skip the value parsing since we don't know whether the
  // user thinks this option has one or not. Since we don't set any identifier
  // specifically, it will remain '?' within the context.
  option = cag_option_find_by_letter(context, n[context->inner_index]);
  if (option == NULL) {
    ++context->index;
    context->inner_index = 0;
    return;
  }

  // We found an option and now we can specify the identifier within the
  // context.
  context->identifier = option->identifier;

  // And now we try to parse the value. This function will also check whether
  // this option is actually supposed to have a value.
  v = &n[++context->inner_index];
  cag_option_parse_value(context, option, &v);

  // Check whether we reached the end of this option argument.
  if (*v == '\0') {
    ++context->index;
    context->inner_index = 0;
  }
}

static void cag_option_shift(cag_option_context *context, int start, int option,
  int end)
{
  char *tmp;
  int a_index, shift_index, shift_count, left_index, right_index;

  shift_count = option - start;

  // There is no shift is required if the start and the option have the same
  // index.
  if (shift_count == 0) {
    return;
  }

  // Lets loop through the option strings first, which we will move towards the
  // beginning.
  for (a_index = option; a_index < end; ++a_index) {
    // First remember the current option value, because we will have to save
    // that later at the beginning.
    tmp = context->argv[a_index];

    // Let's loop over all option values and shift them one towards the end.
    // This will override the option value we just stored temporarily.
    for (shift_index = 0; shift_index < shift_count; ++shift_index) {
      left_index = a_index - shift_index;
      right_index = a_index - shift_index - 1;
      context->argv[left_index] = context->argv[right_index];
    }

    // Now restore the saved option value at the beginning.
    context->argv[a_index - shift_count] = tmp;
  }

  // The new index will be before all non-option values, in such a way that they
  // all will be moved again in the next fetch call.
  context->index = end - shift_count;
}

static bool cag_option_is_argument_string(const char *c)
{
  return *c == '-' && *(c + 1) != '\0';
}

static int cag_option_find_next(cag_option_context *context)
{
  int next_index, next_option_index;
  char *c;

  // Prepare to search the next option at the next index.
  next_index = context->index;
  next_option_index = next_index;

  // Grab a pointer to the string and verify that it is not the end. If it is
  // the end, we have to return false to indicate that we finished.
  c = context->argv[next_option_index];
  if (context->forced_end || c == NULL) {
    return -1;
  }

  // Check whether it is a '-'. We need to find the next option - and an option
  // always starts with a '-'. If there is a string "-\0", we don't consider it
  // as an option neither.
  while (!cag_option_is_argument_string(c)) {
    c = context->argv[++next_option_index];
    if (c == NULL) {
      // We reached the end and did not find any argument anymore. Let's tell
      // our caller that we reached the end.
      return -1;
    }
  }

  // Indicate that we found an option which can be processed. The index of the
  // next option will be returned.
  return next_option_index;
}

UTILS_EXPORT
bool cag_option_fetch(cag_option_context *context)
{
  char *c;
  int old_index, new_index;

  // Reset our identifier to a question mark, which indicates an "unknown"
  // option. The value is set to NULL, to make sure we are not carrying the
  // parameter from the previous option to this one.
  context->identifier = '?';
  context->value = NULL;

  // Check whether there are any options left to parse and remember the old
  // index as well as the new index. In the end we will move the option junk to
  // the beginning, so that non option arguments can be read.
  old_index = context->index;
  new_index = cag_option_find_next(context);
  if (new_index >= 0) {
    context->index = new_index;
  } else {
    return false;
  }

  // Grab a pointer to the beginning of the option. At this point, the next
  // character must be a '-', since if it was not the prepare function would
  // have returned false. We will skip that symbol and proceed.
  c = context->argv[context->index];
  assert(*c == '-');
  ++c;

  // Check whether this is a long option, starting with a double "--".
  if (*c == '-') {
    ++c;

    // This might be a double "--" which indicates the end of options. If this
    // is the case, we will not move to the next index. That ensures that
    // another call to the fetch function will not skip the "--".
    if (*c == '\0') {
      context->forced_end = true;
    } else {
      // We parse now the access name. All information about it will be written
      // to the context.
      cag_option_parse_access_name(context, &c);
    }
  } else {
    // This is no long option, so we can just parse an access letter.
    cag_option_parse_access_letter(context, &c);
  }

  // Move the items so that the options come first followed by non-option
  // arguments.
  cag_option_shift(context, old_index, new_index, context->index);

  return context->forced_end == false;
}

UTILS_EXPORT
char cag_option_get(const cag_option_context *context)
{
  // We just return the identifier here.
  return context->identifier;
}

UTILS_EXPORT
const char * cag_option_get_value(const cag_option_context *context)
{
  // We just return the internal value pointer of the context.
  return context->value;
}

int cag_option_get_index(const cag_option_context *context)
{
  // Either we point to a value item,
  return context->index;
}
