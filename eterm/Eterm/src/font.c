/* font.c -- Eterm font module
 *
 * This file is original work by Michael Jennings <mej@eterm.org> and
 * Tuomo Venalainen <vendu@cc.hut.fi>.  This file, and any other file
 * bearing this same message or a similar one, is distributed under
 * the GNU Public License (GPL) as outlined in the COPYING file.
 *
 * Copyright (C) 1997-1999, Michael Jennings and Tuomo Venalainen
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * 
 */

static const char cvs_ident[] = "$Id$";

#include "config.h"
#include "feature.h"

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#include "../libmej/debug.h"
#include "../libmej/mem.h"
#include "../libmej/strings.h"
#include "debug.h"
#include "command.h"
#include "font.h"
#include "startup.h"
#include "options.h"
#include "screen.h"
#include "term.h"
#include "windows.h"

char **etfonts = NULL;
unsigned char font_idx = DEF_FONT_IDX, def_font_idx = DEF_FONT_IDX, font_cnt = 0;
char *rs_font[NFONTS];
#ifdef MULTI_CHARSET
char *rs_mfont[NFONTS];
char **etmfonts = NULL;
const char *def_mfontName[] = {MFONT0, MFONT1, MFONT2, MFONT3, MFONT4};
#endif
const char *def_fontName[] = {FONT0, FONT1, FONT2, FONT3, FONT4};

static cachefont_t *font_cache = NULL, *cur_font = NULL;
static void font_cache_add(const char *name, unsigned char type, void *info);
static void font_cache_del(const void *info);
static cachefont_t *font_cache_find(const char *name, unsigned char type);
static void *font_cache_find_info(const char *name, unsigned char type);

void
eterm_font_add(char ***plist, const char *fontname, unsigned char idx) {

  char **flist = *plist;

  D_FONT(("eterm_font_add(\"%s\", %u):  plist == %8p\n", NONULL(fontname), (unsigned int) idx, plist));
  ASSERT(plist != NULL);

  if (idx >= font_cnt) {
    unsigned char new_size = sizeof(char *) * (idx + 1);

    if (etfonts) {
      etfonts = (char **) REALLOC(etfonts, new_size);
#ifdef MULTI_CHARSET
      etmfonts = (char **) REALLOC(etmfonts, new_size);
      D_FONT((" -> Reallocating fonts lists to a size of %u bytes gives %8p/%8p\n", new_size, etfonts, etmfonts));
#else
      D_FONT((" -> Reallocating fonts list to a size of %u bytes gives %8p\n", new_size, etfonts));
#endif
    } else {
      etfonts = (char **) MALLOC(new_size);
#ifdef MULTI_CHARSET
      etmfonts = (char **) MALLOC(new_size);
      D_FONT((" -> Allocating fonts lists to a size of %u bytes gives %8p/%8p\n", new_size, etfonts, etmfonts));
#else
      D_FONT((" -> Allocating fonts list to a size of %u bytes gives %8p\n", new_size, etfonts));
#endif
    }
    MEMSET(etfonts + font_cnt, 0, sizeof(char *) * (idx - font_cnt + 1));
#ifdef MULTI_CHARSET
    MEMSET(etmfonts + font_cnt, 0, sizeof(char *) * (idx - font_cnt + 1));
#endif
    font_cnt = idx + 1;
#ifdef MULTI_CHARSET
    flist = ((plist == &etfonts) ? (etfonts) : (etmfonts));
#else
    flist = etfonts;
#endif
  } else {
    if (flist[idx]) {
      if ((flist[idx] == fontname) || (!strcasecmp(flist[idx], fontname))) {
        return;
      }
      FREE(flist[idx]);
    }
  }
  flist[idx] = StrDup(fontname);
  DUMP_FONTS();
}

void
eterm_font_delete(char **flist, unsigned char idx) {

  ASSERT(idx < font_cnt);

  if (flist[idx]) {
    FREE(flist[idx]);
  }
  flist[idx] = NULL;
}

static void
font_cache_add(const char *name, unsigned char type, void *info) {

  cachefont_t *font;

  D_FONT(("font_cache_add(%s, %d, %8p) called.\n", NONULL(name), type, info));

  font = (cachefont_t *) MALLOC(sizeof(cachefont_t));
  font->name = StrDup(name);
  font->type = type;
  font->ref_cnt = 1;
  switch (type) {
  case FONT_TYPE_X: font->fontinfo.xfontinfo = (XFontStruct *) info; break;
  case FONT_TYPE_TTF:  break;
  case FONT_TYPE_FNLIB:  break;
  default:  break;
  }

  if (font_cache == NULL) {
    font_cache = cur_font = font;
    font->next = NULL;
  } else {
    cur_font->next = font;
    font->next = NULL;
    cur_font = font;
  }
}

static void
font_cache_del(const void *info) {

  cachefont_t *current, *tmp;

  D_FONT(("font_cache_del(%8p) called.\n", info));

  if (font_cache == NULL) {
    return;
  }
  if (((font_cache->type == FONT_TYPE_X) && (font_cache->fontinfo.xfontinfo == (XFontStruct *) info))) {
    D_FONT((" -> Match found at font_cache (%8p).  Font name is \"%s\"\n", font_cache, NONULL(font_cache->name)));
    if (--(font_cache->ref_cnt) == 0) {
      D_FONT(("    -> Reference count is now 0.  Deleting from cache.\n"));
      current = font_cache;
      font_cache = current->next;
      XFreeFont(Xdisplay, (XFontStruct *) info);
      FREE(current->name);
      FREE(current);
    } else {
      D_FONT(("    -> Reference count is %d.  Returning.\n", font_cache->ref_cnt));
    }
    return;
  } else if ((font_cache->type == FONT_TYPE_TTF) && (0)) {
  } else if ((font_cache->type == FONT_TYPE_FNLIB) && (0)) {
  } else {
    for (current = font_cache; current->next; current = current->next) {
      if (((current->next->type == FONT_TYPE_X) && (current->next->fontinfo.xfontinfo == (XFontStruct *) info))) {
        D_FONT((" -> Match found at current->next (%8p, current == %8p).  Font name is \"%s\"\n", current->next, current, NONULL(current->next->name)));
        if (--(current->next->ref_cnt) == 0) {
          D_FONT(("    -> Reference count is now 0.  Deleting from cache.\n"));
          tmp = current->next;
          current->next = current->next->next;
          XFreeFont(Xdisplay, (XFontStruct *) info);
          FREE(tmp->name);
          FREE(tmp);
        } else {
          D_FONT(("    -> Reference count is %d.  Returning.\n", font_cache->ref_cnt));
        }
        return;
      } else if ((current->next->type == FONT_TYPE_TTF) && (0)) {
      } else if ((current->next->type == FONT_TYPE_FNLIB) && (0)) {
      }
    }
  }
}

static cachefont_t *
font_cache_find(const char *name, unsigned char type) {

  cachefont_t *current;

  ASSERT_RVAL(name != NULL, NULL);

  D_FONT(("font_cache_find(%s, %d) called.\n", NONULL(name), type));

  for (current = font_cache; current; current = current->next) {
    D_FONT((" -> Checking current (%8p), type == %d, name == %s\n", current, current->type, NONULL(current->name)));
    if ((current->type == type) && !strcasecmp(current->name, name)) {
      D_FONT(("    -> Match!\n"));
      return (current);
    }
  }
  D_FONT(("font_cache_find():  No matches found. =(\n"));
  return ((cachefont_t *) NULL);
}

static void *
font_cache_find_info(const char *name, unsigned char type) {

  cachefont_t *current;

  REQUIRE_RVAL(name != NULL, NULL);

  D_FONT(("font_cache_find_info(%s, %d) called.\n", NONULL(name), type));

  for (current = font_cache; current; current = current->next) {
    D_FONT((" -> Checking current (%8p), type == %d, name == %s\n", current, current->type, NONULL(current->name)));
    if ((current->type == type) && !strcasecmp(current->name, name)) {
      D_FONT(("    -> Match!\n"));
      switch (type) {
      case FONT_TYPE_X: return ((void *) current->fontinfo.xfontinfo); break;
      case FONT_TYPE_TTF: return (NULL); break;
      case FONT_TYPE_FNLIB: return (NULL); break;
      default: return (NULL); break;
      }
    }
  }
  D_FONT(("font_cache_find_info():  No matches found. =(\n"));
  return (NULL);
}

void *
load_font(const char *name, const char *fallback, unsigned char type)
{

  cachefont_t *font;
  XFontStruct *xfont;

  D_FONT(("load_font(%s, %s, %d) called.\n", NONULL(name), NONULL(fallback), type));

  if (type == 0) {
    type = FONT_TYPE_X;
  }
  if (name == NULL) {
    if (fallback) {
      name = fallback;
      fallback = "fixed";
    } else {
      name = "fixed";
#ifdef MULTI_CHARSET
      fallback = "-misc-fixed-medium-r-normal--13-120-75-75-c-60-iso10646-1";
#else
      fallback = "-misc-fixed-medium-r-normal--13-120-75-75-c-60-iso8859-1";
#endif
    }
  } else if (fallback == NULL) {
    fallback = "fixed";
  }
  D_FONT((" -> Using name == \"%s\" and fallback == \"%s\"\n", name, fallback));

  if ((font = font_cache_find(name, type)) != NULL) {
    font_cache_add_ref(font);
    D_FONT((" -> Font found in cache.  Incrementing reference count to %d and returning existing data.\n", font->ref_cnt));
    switch (type) {
    case FONT_TYPE_X: return ((void *) font->fontinfo.xfontinfo); break;
    case FONT_TYPE_TTF: return (NULL); break;
    case FONT_TYPE_FNLIB: return (NULL); break;
    default: return (NULL); break;
    }
  }
  if (type == FONT_TYPE_X) {
    if ((xfont = XLoadQueryFont(Xdisplay, name)) == NULL) {
      print_error("Unable to load font \"%s\".  Falling back on \"%s\"\n", name, fallback);
      if ((xfont = XLoadQueryFont(Xdisplay, fallback)) == NULL) {
        fatal_error("Couldn't load the fallback font either.  Giving up.");
      } else {
        font_cache_add(fallback, type, (void *) xfont);
      }
    } else {
      font_cache_add(name, type, (void *) xfont);
    }
    return ((void *) xfont);
  } else if (type == FONT_TYPE_TTF) {
    return (NULL);
  } else if (type == FONT_TYPE_FNLIB) {
    return (NULL);
  }
  ASSERT_NOTREACHED_RVAL(NULL);
}

void
free_font(const void *info)
{

  ASSERT(info != NULL);

  font_cache_del(info);
}

void
change_font(int init, const char *fontname)
{
#ifndef NO_BOLDFONT
  static XFontStruct *boldFont = NULL;
#endif
  short idx = 0;
  int fh, fw = 0;
  register unsigned long i;
  register int cw;

  D_FONT(("change_font(%d, \"%s\"):  def_font_idx == %u, font_idx == %u\n", init, NONULL(fontname), (unsigned int) def_font_idx, (unsigned int) font_idx));

  if (init) {
    font_idx = def_font_idx;
    ASSERT(etfonts != NULL);
    ASSERT(etfonts[font_idx] != NULL);
#ifdef MULTI_CHARSET
    ASSERT(etmfonts != NULL);
    ASSERT(etmfonts[font_idx] != NULL);
#endif    
  } else {
    ASSERT(fontname != NULL);

    switch (*fontname) {
      case '\0':
	font_idx = def_font_idx;
	fontname = NULL;
	break;

	/* special (internal) prefix for font commands */
      case FONT_CMD:
	idx = atoi(++fontname);
	switch (*fontname) {
	  case '+':
	    NEXT_FONT(idx);
	    break;

	  case '-':
	    PREV_FONT(idx);
	    break;

	  default:
	    if (*fontname != '\0' && !isdigit(*fontname))
	      return;
            BOUND(idx, 0, (font_cnt - 1));
	    font_idx = idx;
	    break;
	}
	fontname = NULL;
	break;

      default:
        for (idx = 0; idx < font_cnt; idx++) {
          if (!strcasecmp(etfonts[idx], fontname)) {
            font_idx = idx;
            fontname = NULL;
            break;
	  }
	}
	break;
    }
    if (fontname != NULL) {
      eterm_font_add(&etfonts, fontname, font_idx);
    }
  }
  D_FONT((" -> Changing to font index %u (\"%s\")\n", (unsigned int) font_idx, NONULL(etfonts[font_idx])));
  if (TermWin.font) {
    if (font_cache_find_info(etfonts[font_idx], FONT_TYPE_X) != TermWin.font) {
      free_font(TermWin.font);
      TermWin.font = load_font(etfonts[font_idx], "fixed", FONT_TYPE_X);
    }
  } else {
    TermWin.font = load_font(etfonts[font_idx], "fixed", FONT_TYPE_X);
  }

#ifndef NO_BOLDFONT
  if (init && rs_boldFont != NULL) {
    boldFont = load_font(rs_boldFont, "-misc-fixed-bold-r-semicondensed--13-120-75-75-c-60-iso8859-1", FONT_TYPE_X);
  }
#endif

#ifdef MULTI_CHARSET
  if (TermWin.mfont) {
    if (font_cache_find_info(etmfonts[font_idx], FONT_TYPE_X) != TermWin.mfont) {
      free_font(TermWin.mfont);
      TermWin.mfont = load_font(etmfonts[font_idx], "k14", FONT_TYPE_X);
    }
  } else {
    TermWin.mfont = load_font(etmfonts[font_idx], "k14", FONT_TYPE_X);
  }
# ifdef USE_XIM
  if (xim_input_context) {
    if (TermWin.fontset) {
      XFreeFontSet(Xdisplay, TermWin.fontset);
    }
    TermWin.fontset = create_fontset(etfonts[font_idx], etmfonts[font_idx]);
    xim_set_fontset();
  }
# endif
#endif /* MULTI_CHARSET */

  if (!init) {
    XSetFont(Xdisplay, TermWin.gc, TermWin.font->fid);
  }

  fw = TermWin.font->min_bounds.width;
  fh = TermWin.font->ascent + TermWin.font->descent + rs_line_space;

  D_FONT(("Font information:  Ascent == %hd, Descent == %hd\n", TermWin.font->ascent, TermWin.font->descent));
  if (TermWin.font->min_bounds.width == TermWin.font->max_bounds.width)
    TermWin.fprop = 0;	/* Mono-spaced (fixed width) font */
  else
    TermWin.fprop = 1;	/* Proportional font */
  if (TermWin.fprop == 1)
    for (i = TermWin.font->min_char_or_byte2; i <= TermWin.font->max_char_or_byte2; i++) {
      cw = TermWin.font->per_char[i].width;
      MAX_IT(fw, cw);
    }
  /* not the first time thru and sizes haven't changed */
  if (fw == TermWin.fwidth && fh == TermWin.fheight)
    return;			/* TODO: not return; check MULTI_CHARSET if needed */

  TermWin.fwidth = fw;
  TermWin.fheight = fh;

  /* check that size of boldFont is okay */
#ifndef NO_BOLDFONT
  TermWin.boldFont = NULL;
  if (boldFont != NULL) {

    fw = boldFont->min_bounds.width;
    fh = boldFont->ascent + boldFont->descent + rs_line_space;
    if (TermWin.fprop == 0) {	/* bold font must also be monospaced */
      if (fw != boldFont->max_bounds.width)
	fw = -1;
    } else {
      for (i = 0; i < 256; i++) {
	if (!isprint(i))
	  continue;
	cw = boldFont->per_char[i].width;
	MAX_IT(fw, cw);
      }
    }

    if (fw == TermWin.fwidth && fh == TermWin.fheight) {
      TermWin.boldFont = boldFont;
    }
  }
#endif /* NO_BOLDFONT */

  set_colorfgbg();

  TermWin.width = TermWin.ncol * TermWin.fwidth;
  TermWin.height = TermWin.nrow * TermWin.fheight;

  if (init) {
    szHint.width_inc = TermWin.fwidth;
    szHint.height_inc = TermWin.fheight;

    szHint.min_width = szHint.base_width + szHint.width_inc;
    szHint.min_height = szHint.base_height + szHint.height_inc;

    szHint.width = szHint.base_width + TermWin.width;
    szHint.height = szHint.base_height + TermWin.height;

    szHint.flags = PMinSize | PResizeInc | PBaseSize | PWinGravity;
  } else {
    parent_resize();
  }
  return;
}
