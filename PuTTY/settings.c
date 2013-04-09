/*
 * settings.c: read and write saved sessions. (platform-independent)
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "putty.h"
#include "storage.h"

/*
 * HACK: PuttyTray / Nutty
 */ 
#include "urlhack.h"

/* The cipher order given here is the default order. */
static const struct keyvalwhere ciphernames[] = {
    { "aes",        CIPHER_AES,             -1, -1 },
    { "blowfish",   CIPHER_BLOWFISH,        -1, -1 },
    { "3des",       CIPHER_3DES,            -1, -1 },
    { "WARN",       CIPHER_WARN,            -1, -1 },
    { "arcfour",    CIPHER_ARCFOUR,         -1, -1 },
    { "des",        CIPHER_DES,             -1, -1 }
};

static const struct keyvalwhere kexnames[] = {
    { "dh-gex-sha1",        KEX_DHGEX,      -1, -1 },
    { "dh-group14-sha1",    KEX_DHGROUP14,  -1, -1 },
    { "dh-group1-sha1",     KEX_DHGROUP1,   -1, -1 },
    { "rsa",                KEX_RSA,        KEX_WARN, -1 },
    { "WARN",               KEX_WARN,       -1, -1 }
};

/*
 * All the terminal modes that we know about for the "TerminalModes"
 * setting. (Also used by config.c for the drop-down list.)
 * This is currently precisely the same as the set in ssh.c, but could
 * in principle differ if other backends started to support tty modes
 * (e.g., the pty backend).
 */
const char *const ttymodes[] = {
    "INTR",	"QUIT",     "ERASE",	"KILL",     "EOF",
    "EOL",	"EOL2",     "START",	"STOP",     "SUSP",
    "DSUSP",	"REPRINT",  "WERASE",	"LNEXT",    "FLUSH",
    "SWTCH",	"STATUS",   "DISCARD",	"IGNPAR",   "PARMRK",
    "INPCK",	"ISTRIP",   "INLCR",	"IGNCR",    "ICRNL",
    "IUCLC",	"IXON",     "IXANY",	"IXOFF",    "IMAXBEL",
    "ISIG",	"ICANON",   "XCASE",	"ECHO",     "ECHOE",
    "ECHOK",	"ECHONL",   "NOFLSH",	"TOSTOP",   "IEXTEN",
    "ECHOCTL",	"ECHOKE",   "PENDIN",	"OPOST",    "OLCUC",
    "ONLCR",	"OCRNL",    "ONOCR",	"ONLRET",   "CS7",
    "CS8",	"PARENB",   "PARODD",	NULL
};

const char* urlhack_default_regex = "((((https?|ftp):\\/\\/)|www\\.)(([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+)|localhost|([a-zA-Z0-9\\-]+\\.)*[a-zA-Z0-9\\-]+\\.(com|net|org|info|biz|gov|name|edu|[a-zA-Z][a-zA-Z]))(:[0-9]+)?((\\/|\\?)[^ \"]*[^ ,;\\.:\">)])?)|(spotify:[^ ]+:[^ ]+)";

/*****************************************/
#include "settings_inc.c"
/*****************************************/
static void gpps(void *handle, const char *name, const char *def,
		 char *val, int len)
{
    if (!read_setting_s(handle, name, val, len)) {
	char *pdef;

	pdef = platform_default_s(name);
	if (pdef) {
	    strncpy(val, pdef, len);
	    sfree(pdef);
	} else {
	    strncpy(val, def, len);
	}

	val[len - 1] = '\0';
    }
}

/*
 * gppfont and gppfile cannot have local defaults, since the very
 * format of a Filename or Font is platform-dependent. So the
 * platform-dependent functions MUST return some sort of value.
 */
static void gppfont(void *handle, const char *name, FontSpec *result)
{
    if (!read_setting_fontspec(handle, name, result))
	*result = platform_default_fontspec(name);
}
static void gppfile(void *handle, const char *name, Filename *result)
{
    if (!read_setting_filename(handle, name, result))
	*result = platform_default_filename(name);
}

static void gppi(void *handle, char *name, int def, int *i)
{
    def = platform_default_i(name, def);
    *i = read_setting_i(handle, name, def);
}

/*
 * Read a set of name-value pairs in the format we occasionally use:
 *   NAME\tVALUE\0NAME\tVALUE\0\0 in memory
 *   NAME=VALUE,NAME=VALUE, in storage
 * `def' is in the storage format.
 */
static void gppmap(void *handle, char *name, char *def, char *val, int len)
{
    char *buf = snewn(2*len, char), *p, *q;
    gpps(handle, name, def, buf, 2*len);
    p = buf;
    q = val;
    while (*p) {
	while (*p && *p != ',') {
	    int c = *p++;
	    if (c == '=')
		c = '\t';
	    if (c == '\\')
		c = *p++;
	    *q++ = c;
	}
	if (*p == ',')
	    p++;
	*q++ = '\0';
    }
    *q = '\0';
    sfree(buf);
}

/*
 * Write a set of name/value pairs in the above format.
 */
static void wmap(void *handle, char const *key, char const *value, int len)
{
    char *buf = snewn(2*len, char), *p;
    const char *q;
    p = buf;
    q = value;
    while (*q) {
	while (*q) {
	    int c = *q++;
	    if (c == '=' || c == ',' || c == '\\')
		*p++ = '\\';
	    if (c == '\t')
		c = '=';
	    *p++ = c;
	}
	*p++ = ',';
	q++;
    }
    *p = '\0';
    write_setting_s(handle, key, buf);
    sfree(buf);
}

static int key2val(const struct keyvalwhere *mapping,
                   int nmaps, char *key)
{
    int i;
    for (i = 0; i < nmaps; i++)
	if (!strcmp(mapping[i].s, key)) return mapping[i].v;
    return -1;
}

static const char *val2key(const struct keyvalwhere *mapping,
                           int nmaps, int val)
{
    int i;
    for (i = 0; i < nmaps; i++)
	if (mapping[i].v == val) return mapping[i].s;
    return NULL;
}

/*
 * Helper function to parse a comma-separated list of strings into
 * a preference list array of values. Any missing values are added
 * to the end and duplicates are weeded.
 * XXX: assumes vals in 'mapping' are small +ve integers
 */
static void gprefs(void *sesskey, char *name, char *def,
		   const struct keyvalwhere *mapping, int nvals,
		   int *array)
{
    char commalist[256];
    char *p, *q;
    int i, j, n, v, pos;
    unsigned long seen = 0;	       /* bitmap for weeding dups etc */

    /*
     * Fetch the string which we'll parse as a comma-separated list.
     */
    gpps(sesskey, name, def, commalist, sizeof(commalist));

    /*
     * Go through that list and convert it into values.
     */
    n = 0;
    p = commalist;
    while (1) {
        while (*p && *p == ',') p++;
        if (!*p)
            break;                     /* no more words */

        q = p;
        while (*p && *p != ',') p++;
        if (*p) *p++ = '\0';

        v = key2val(mapping, nvals, q);
        if (v != -1 && !(seen & (1 << v))) {
	    seen |= (1 << v);
	    array[n++] = v;
	}
    }

    /*
     * Now go through 'mapping' and add values that weren't mentioned
     * in the list we fetched. We may have to loop over it multiple
     * times so that we add values before other values whose default
     * positions depend on them.
     */
    while (n < nvals) {
        for (i = 0; i < nvals; i++) {
	    assert(mapping[i].v < 32);

	    if (!(seen & (1 << mapping[i].v))) {
                /*
                 * This element needs adding. But can we add it yet?
                 */
                if (mapping[i].vrel != -1 && !(seen & (1 << mapping[i].vrel)))
                    continue;          /* nope */

                /*
                 * OK, we can work out where to add this element, so
                 * do so.
                 */
                if (mapping[i].vrel == -1) {
                    pos = (mapping[i].where < 0 ? n : 0);
                } else {
                    for (j = 0; j < n; j++)
                        if (array[j] == mapping[i].vrel)
                            break;
                    assert(j < n);     /* implied by (seen & (1<<vrel)) */
                    pos = (mapping[i].where < 0 ? j : j+1);
                }

                /*
                 * And add it.
                 */
                for (j = n-1; j >= pos; j--)
                    array[j+1] = array[j];
                array[pos] = mapping[i].v;
                n++;
            }
        }
    }
}

/* 
 * Write out a preference list.
 */
static void wprefs(void *sesskey, char *name,
		   const struct keyvalwhere *mapping, int nvals,
		   int *array)
{
    char *buf, *p;
    int i, maxlen;

    for (maxlen = i = 0; i < nvals; i++) {
	const char *s = val2key(mapping, nvals, array[i]);
	if (s) {
            maxlen += (maxlen > 0 ? 1 : 0) + strlen(s);
        }
    }

    buf = snewn(maxlen + 1, char);
    p = buf;

    for (i = 0; i < nvals; i++) {
	const char *s = val2key(mapping, nvals, array[i]);
	if (s) {
            p += sprintf(p, "%s%s", (p > buf ? "," : ""), s);
	}
    }

    assert(p - buf == maxlen);
    *p = '\0';

    write_setting_s(sesskey, name, buf);

    sfree(buf);
}

char *save_settings(char *section, Config * cfg)
{
    void *sesskey;
    char *errmsg;

    sesskey = open_settings_w(section, &errmsg);
    if (!sesskey)
	return errmsg;
    save_open_settings(sesskey, cfg);
    close_settings_w(sesskey);
    return NULL;
}

void load_settings(char *section, Config * cfg)
{
    void *sesskey;

    sesskey = open_settings_r(section);
    load_open_settings(sesskey, cfg);
    close_settings_r(sesskey);

    if (cfg_launchable(cfg))
        add_session_to_jumplist(section);
}

/*
 * HACK: PuttyTray / PuTTY File
 * Quick hack to load defaults from file
 */
void load_settings_file(char *section, Config * cfg)
{
    void *sesskey;
	set_storagetype(1);
    sesskey = open_settings_r(section);
    load_open_settings(sesskey, cfg);
    close_settings_r(sesskey);
}

void do_defaults(char *session, Config * cfg)
{
    load_settings(session, cfg);
}

/*
 * HACK: PuttyTray / PuTTY File
 * Quick hack to load defaults from file
 */
void do_defaults_file(char *session, Config * cfg)
{
    load_settings_file(session, cfg);
}

static int sessioncmp(const void *av, const void *bv)
{
    const char *a = *(const char *const *) av;
    const char *b = *(const char *const *) bv;

    /*
     * Alphabetical order, except that "Default Settings" is a
     * special case and comes first.
     */
    if (!strcmp(a, "Default Settings"))
	return -1;		       /* a comes first */
    if (!strcmp(b, "Default Settings"))
	return +1;		       /* b comes first */
    /*
     * FIXME: perhaps we should ignore the first & in determining
     * sort order.
     */
    return strcmp(a, b);	       /* otherwise, compare normally */
}

/*
 * HACK: PuttyTray / PuTTY File
 * Updated get_sesslist with storagetype
 */
int get_sesslist(struct sesslist *list, int allocate, int storagetype) // HACK: PuTTYTray / PuTTY File - changed return type
{
    char otherbuf[2048];
    int buflen, bufsize, i;
    char *p, *ret;
    void *handle;
	
	// HACK: PUTTY FILE
	int autoswitch = 0;
	if (storagetype > 1) {
		storagetype = storagetype - 2;
		autoswitch = 1;
	}

    if (allocate) {
	buflen = bufsize = 0;
	list->buffer = NULL;
	if ((handle = enum_settings_start(storagetype)) != NULL) { // HACK: PuTTYTray / PuTTY File - storagetype
	    do {
		ret = enum_settings_next(handle, otherbuf, sizeof(otherbuf));
		if (ret) {
		    int len = strlen(otherbuf) + 1;
		    if (bufsize < buflen + len) {
			bufsize = buflen + len + 2048;
			list->buffer = sresize(list->buffer, bufsize, char);
		    }
		    strcpy(list->buffer + buflen, otherbuf);
		    buflen += strlen(list->buffer + buflen) + 1;
		}
	    } while (ret);
	    enum_settings_finish(handle);
	}
	list->buffer = sresize(list->buffer, buflen + 1, char);
	list->buffer[buflen] = '\0';

	/*
	 * HACK: PuttyTray / PuTTY File
	 * Switch to file mode if registry is empty (and in registry mode)
	 */
	if (autoswitch == 1 && storagetype != 1 && buflen == 0) {
		storagetype = 1;

		// Ok, this is a copy of the code above. Crude but working
		buflen = bufsize = 0;
		list->buffer = NULL;
		if ((handle = enum_settings_start(1)) != NULL) { // Force file storage type
			do {
			ret = enum_settings_next(handle, otherbuf, sizeof(otherbuf));
			if (ret) {
				int len = strlen(otherbuf) + 1;
				if (bufsize < buflen + len) {
				bufsize = buflen + len + 2048;
				list->buffer = sresize(list->buffer, bufsize, char);
				}
				strcpy(list->buffer + buflen, otherbuf);
				buflen += strlen(list->buffer + buflen) + 1;
			}
			} while (ret);
			enum_settings_finish(handle);
		}
		list->buffer = sresize(list->buffer, buflen + 1, char);
		list->buffer[buflen] = '\0';
	}

	/*
	 * HACK: PuttyTray / PuTTY File
	 * If registry is empty AND file store is empty, show empty registry
	 */
	if (autoswitch == 1 && storagetype == 1 && buflen == 0) {
		storagetype = 0;
		set_storagetype(storagetype);
	}


	/*
	 * Now set up the list of sessions. Note that "Default
	 * Settings" must always be claimed to exist, even if it
	 * doesn't really.
	 */

	p = list->buffer;
	list->nsessions = 1;	       /* "Default Settings" counts as one */
	while (*p) {
	    if (strcmp(p, "Default Settings"))
		list->nsessions++;
	    while (*p)
		p++;
	    p++;
	}

	list->sessions = snewn(list->nsessions + 1, char *);
	list->sessions[0] = "Default Settings";
	p = list->buffer;
	i = 1;
	while (*p) {
	    if (strcmp(p, "Default Settings"))
		list->sessions[i++] = p;
	    while (*p)
		p++;
	    p++;
	}

	qsort(list->sessions, i, sizeof(char *), sessioncmp);
    } else {
	sfree(list->buffer);
	sfree(list->sessions);
	list->buffer = NULL;
	list->sessions = NULL;
    }

	/*
	 * HACK: PuttyTray / PuTTY File
	 * Return storagetype
	 */
	return storagetype;
}
