%{

  #include <stdio.h>
  #include <math.h>
  #include <stdlib.h>
  #include <string.h>
  #include "sfz.h"

  extern int yylineno;
  extern char *yytext;
  int yyerror(const char *msg);
  int yylex (void);

  char *header;
  char *opcode;
  gpointer value;
  GHashTable *global_opcodes = NULL;
  GHashTable *group_opcodes = NULL;
  GHashTable *region_opcodes = NULL;
  GHashTable *header_opcodes;

  struct emu_file *file;
  struct emu_sfz_context *emu_sfz_context;

  void sfz_parser_set_context (struct emu_file *file_, struct emu_sfz_context *emu_sfz_context_) {
    file = file_;
    emu_sfz_context = emu_sfz_context_;
  }

%}

%define parse.error verbose

%token SFZ_EQUAL
       SFZ_FLOAT
       SFZ_INTEGER
       SFZ_STRING
       SFZ_HEADER
       SFZ_OPCODE

%%

sfz: {
       global_opcodes = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
       group_opcodes = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
       region_opcodes = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
     }
     headers
     {
       g_hash_table_unref (global_opcodes);
       g_hash_table_unref (group_opcodes);
       g_hash_table_unref (region_opcodes);
     };

headers: | header headers;

header: SFZ_HEADER
        {
          header = strdup (yytext);
          if (!strcmp("<global>", header)) {
            g_hash_table_remove_all (global_opcodes);
            header_opcodes = global_opcodes;
          } else if (!strcmp("<group>", header)) {
            g_hash_table_remove_all (group_opcodes);
            header_opcodes = group_opcodes;
          } else if (!strcmp("<region>", header)) {
            g_hash_table_remove_all (region_opcodes);
            header_opcodes = region_opcodes;
          } else {
            emu_debug (1, "SFZ header %s not supported. Skipping...", header);
            header_opcodes = NULL;
          }
        }
        opcode_expr_list
        {
          if (!strcmp("<global>", header)) {
            emu_debug (1, "SFZ header %s read", header);
          } else if (!strcmp("<group>", header)) {
            emu_debug (1, "SFZ header %s read", header);
          } else if (!strcmp("<region>", header)) {
            emu_debug (1, "SFZ header %s read", header);
            emu3_sfz_region_add (file, emu_sfz_context, global_opcodes, group_opcodes, region_opcodes);
          }
          free (header);
        };

opcode_expr_list: | opcode_expr opcode_expr_list;

opcode_expr: SFZ_OPCODE
             {
               opcode = strdup (yytext);
             }
             SFZ_EQUAL
             opcode_val
             {
               emu_debug (2, "SFZ opcode '%s' read", opcode);
               if (header_opcodes) {
                 g_hash_table_insert (header_opcodes, opcode, value);
               }
             };

opcode_val: SFZ_FLOAT   { value = g_malloc(sizeof(gfloat)); *(gfloat *)value = atof (yytext); emu_debug (2, "SFZ float '%f' read", *(gfloat *)value); } |
            SFZ_INTEGER { value = g_malloc(sizeof(gint)); *(gint *)value = atoi (yytext); emu_debug (2, "SFZ integer '%d' read", *(gint *)value); } |
            SFZ_STRING  { value = g_strdup (yytext); g_strchomp (value); emu_debug (2, "SFZ string '%s' read", (gchar *)value); };

%%

int yyerror(const char *s)
{
  return fprintf (stderr, "line %d: %s\n", yylineno, s);
}
