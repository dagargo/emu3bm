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
  GHashTable *header_opcodes;

  struct emu_sfz_context *esctx;

  void sfz_parser_set_context (struct emu_sfz_context *emu_sfz_context) {
    esctx = emu_sfz_context;
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

sfz: headers;

headers: | header headers;

header: SFZ_HEADER
        {
          header = strdup (yytext);
          if (!strcmp("<global>", header)) {
            g_hash_table_remove_all (esctx->global_opcodes);
            header_opcodes = esctx->global_opcodes;
          } else if (!strcmp("<group>", header)) {
            g_hash_table_remove_all (esctx->group_opcodes);
            header_opcodes = esctx->group_opcodes;
          } else if (!strcmp("<region>", header)) {
            g_hash_table_remove_all (esctx->region_opcodes);
            header_opcodes = esctx->region_opcodes;
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
            emu3_sfz_add_region (esctx);
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
            SFZ_INTEGER { value = g_malloc(sizeof(gfloat)); *(gfloat *)value = atoi (yytext); emu_debug (2, "SFZ integer '%d' read", (gint) *(gfloat *)value); } |
            SFZ_STRING  { value = g_strdup (yytext); g_strchomp (value); emu_debug (2, "SFZ string '%s' read", (gchar *)value);
                          if (!strcmp(opcode, "key") || !strcmp(opcode, "pitch_keycenter") || !strcmp(opcode, "lokey") || !strcmp(opcode, "hikey")) {
                            gint note_num = emu_reverse_note_search (value) + 21; // Conversion of emu3 notes to MIDI notes
                            g_free (value);
                            gint *note_num_p = g_malloc(sizeof(gint));
                            *note_num_p = note_num;
                            value = note_num_p;
                          }
                        };

%%

int yyerror(const char *s)
{
  return fprintf (stderr, "line %d: %s\n", yylineno, s);
}
