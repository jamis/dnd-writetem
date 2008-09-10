#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "writetem.h"
#include "wtstream.h"

/* ---------------------------------------------------------------------- *
 * Constant definitions
 * ---------------------------------------------------------------------- */

#define DELIM_START     "<!--%"
#define DELIM_END       "%-->"

#define MAX_POSSIBLE_BUFFER_LEN  ( 64000 )

/* ---------------------------------------------------------------------- *
 * Private (static) methods
 * ---------------------------------------------------------------------- */

static int s_wtTagReplaceHandler( wtSTREAM_t *stream, wtTAG_t **tags, wtGENERIC_t data, char *other );
static int s_wtTagReplaceIHandler( wtSTREAM_t *stream, wtTAG_t **tags, wtGENERIC_t data, char *other );
static int s_wtTagDelegateHandler( wtSTREAM_t *stream, wtTAG_t **tags, wtGENERIC_t data, char *other );
static int s_wtTagIncludeHandler( wtSTREAM_t *stream, wtTAG_t **tags, wtGENERIC_t data, char *other );
static char* s_findEndingTag( char* buf );

/* ---------------------------------------------------------------------- *
 * Public method implementation
 * ---------------------------------------------------------------------- */

void wtFreeTagList( wtTAG_t **tags ) {
  int i;

  for( i = 0; tags[ i ] != NULL; i++ ) {
    free( tags[ i ] );
  }
}


void wtWriteTemplateBuf( wtSTREAM_t  *stream,
                         char        *templateBuffer,
                         wtTAG_t    **tags ) 
{
  int   i;
  char *pos;
  char *start;
  char *end;
  char *other;

  start = templateBuffer;
  while( ( pos = strstr( start, DELIM_START ) ) != NULL ) {
    *pos = '\0';
    pos += strlen( DELIM_START );

    wtPrint( stream, start );

    end = s_findEndingTag( pos );
    
    if( end == NULL ) {
      start = pos;
      continue;
    } 
      
    *end = '\0';
    end += strlen( DELIM_END );

    /* look for a ':' to indicate additional data */
    other = pos;
		while( isalnum( *other ) || *other == '_' ) {
			other++;
		}
    if( *other == ':' ) {
      *other = 0;
      other++;
    } else {
			other = 0;
		}

    /* look for a matching token */
    for( i = 0; tags[ i ] != NULL; i++ ) {
      if( strcmp( tags[ i ]->tag, pos ) == 0 ) {
        tags[ i ]->handler( stream, tags, tags[ i ]->data, other );
        break;
      }
    }

    start = end;
  }

  wtPrint( stream, start );
}


int wtWriteTemplate( wtSTREAM_t  *stream,
                     char        *templateFile,
                     wtTAG_t    **tags ) 
{
  FILE *temFile;
  int   len;
	char *temContents;

  temFile = fopen( templateFile, "rt" );
  if( temFile == NULL ) {
    return -1;
  }

  fseek( temFile, 0, SEEK_END );
  len = ftell( temFile );
  rewind( temFile );

  temContents = (char*)malloc( len + 1 );
  fread( temContents, 1, len, temFile );
  fclose( temFile );

  temContents[ len ] = 0;

	wtWriteTemplateBuf( stream, temContents, tags );

  free( temContents );

  return 0;
}


int wtWriteTemplateToFile( FILE     *f, 
                           char     *templateFile,
                           wtTAG_t **tags )
{
  wtSTREAM_t stream;

  stream.type = stFILE;
  stream.stream.file.fp = f;
  stream.stream.file.closeable = 0;

  return wtWriteTemplate( &stream, templateFile, tags );
}


int wtWriteTemplateToBuffer( char    **buffer,
                             char     *templateFile,
                             wtTAG_t **tags )
{
  wtSTREAM_t stream;
  int        rc;

  *buffer = NULL;

  stream.type = stBUFFER;
  stream.stream.buffer.length = 0;
  stream.stream.buffer.handle = NULL;

  rc = wtWriteTemplate( &stream, templateFile, tags );

  *buffer = stream.stream.buffer.handle;

  return rc;
}


wtTAG_t *wtTagReplace( char* tag, char* data ) {
  wtTAG_t *newTag;

  newTag = (wtTAG_t*)malloc( sizeof( wtTAG_t ) );
  newTag->tag = tag;
  newTag->data = data;
  newTag->handler = s_wtTagReplaceHandler;

  return newTag;
}


wtTAG_t *wtTagReplaceI( char* tag, int data ) {
  wtTAG_t *newTag;

  newTag = (wtTAG_t*)malloc( sizeof( wtTAG_t ) );
  newTag->tag = tag;
  newTag->data = (wtGENERIC_t)data;
  newTag->handler = s_wtTagReplaceIHandler;

  return newTag;
}


wtTAG_t *wtTagDelegate( char* tag, wtDELEGATE_t *handler ) {
  wtTAG_t *newTag;

  newTag = (wtTAG_t*)malloc( sizeof( wtTAG_t ) );
  newTag->tag = tag;
  newTag->data = handler;
  newTag->handler = s_wtTagDelegateHandler;

  return newTag;
}


wtTAG_t *wtTagInclude( char* tag, char* file ) {
	wtTAG_t *newTag;

  newTag = (wtTAG_t*)malloc( sizeof( wtTAG_t ) );
  newTag->tag = tag;
	newTag->data = file;
	newTag->handler = s_wtTagIncludeHandler;

	return newTag;
}


int wtConditionalHandler( wtSTREAM_t *stream, wtTAG_t** tags, wtGENERIC_t data, char* other ) {
  wtIF_t *ifData;
  int     rc;

  ifData = (wtIF_t*)data;

  if( ifData->value == NULL ) {
    ifData->value = "";
  }
  if( other == NULL ) {
    other = "";
  }

  rc = strcmp( ifData->value, other );
  if( ifData->neg ) {
    if( rc == 0 ) {
      rc = 1;
    } else {
      rc = 0;
    }
  }

  if( rc == 0 ) {
    wtPrint( stream, ifData->text );
  }

  return 0;
}


int wtConditionalDisplayHandler( wtSTREAM_t *stream, wtTAG_t** tags, wtGENERIC_t data, char* other ) {
  wtIF_t *ifData;
  int     rc;
	char*   p;

  ifData = (wtIF_t*)data;

  if( ifData->value == NULL ) {
    ifData->value = "";
  }
  if( other == NULL ) {
		return 0;
  }

	p = strchr( other, ':' );
	if( p == 0 ) {
		return 0;
	}

	*p = 0;
	p++;

  rc = strcmp( ifData->value, other );
  if( ifData->neg ) {
    if( rc == 0 ) {
      rc = 1;
    } else {
      rc = 0;
    }
  }

  if( rc == 0 ) {
		wtWriteTemplateBuf( stream, p, tags );
  }

  return 0;
}


/* ---------------------------------------------------------------------- *
 * Private (static) method implementation
 * ---------------------------------------------------------------------- */

static int s_wtTagReplaceHandler( wtSTREAM_t *stream, wtTAG_t **tags, wtGENERIC_t data, char* other ) {
  wtPrint( stream, (char*)data );
  return 0;
}


static int s_wtTagReplaceIHandler( wtSTREAM_t *stream, wtTAG_t **tags, wtGENERIC_t data, char* other ) {
  char buffer[256];

  sprintf( buffer, "%d", (int)data );
  wtPrint( stream, buffer );

  return 0;
}


static int s_wtTagDelegateHandler( wtSTREAM_t *stream, wtTAG_t **tags, wtGENERIC_t data, char* other ) {
  wtDELEGATE_t *d;

  d = (wtDELEGATE_t*)data;
  return d->handler( stream, tags, d->userData, other );
}


static int s_wtTagIncludeHandler( wtSTREAM_t *stream, wtTAG_t **tags, wtGENERIC_t data, char* other ) {
	char* fname;

	fname = ( data != 0 ? (char*)data : other );

	if( fname == 0 ) {
		return 0;
	}

	return wtWriteTemplate( stream, fname, tags );
}


static char* s_findEndingTag( char* buf ) {
	int   etagLen;
	int   stagLen;
	int   depth;
	char* p;

	stagLen = strlen( DELIM_START );
	etagLen = strlen( DELIM_END );

	depth = 1;
	for( p = buf; *p != 0; p++ ) {
		if( strncmp( p, DELIM_START, stagLen ) == 0 ) {
			depth++;
		} else if( strncmp( p, DELIM_END, etagLen ) == 0 ) {
			depth--;
			if( depth < 1 ) {
				return p;
			}
		}
	}

	return 0;
}
