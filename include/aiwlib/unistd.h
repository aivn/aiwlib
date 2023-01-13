# include <direct.h>

/// Windows version of ftruncate
# include <io.h>
# define ftruncate _chsize
//# include "sleep.h"

# define PATH_MAX MAX_PATH