/*
 * windlg.c - dialogs for PuTTY(tel), including the configuration dialog.
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include <ctype.h>
#include <time.h>

#include "putty.h"
#include "ssh.h"
#include "win_res.h"
#include "storage.h"
#include "dialog.h"

#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>

#ifdef MSVC4
#define TVINSERTSTRUCT  TV_INSERTSTRUCT
#define TVITEM          TV_ITEM
#define ICON_BIG        1
#endif

/*
 * These are the various bits of data required to handle the
 * portable-dialog stuff in the config box. Having them at file
 * scope in here isn't too bad a place to put them; if we were ever
 * to need more than one config box per process we could always
 * shift them to a per-config-box structure stored in GWL_USERDATA.
 */
static struct controlbox *ctrlbox;
/*
 * ctrls_base holds the OK and Cancel buttons: the controls which
 * are present in all dialog panels. ctrls_panel holds the ones
 * which change from panel to panel.
 */
static struct winctrls ctrls_base, ctrls_panel;
static struct dlgparam dp;


#define PRINTER_DISABLED_STRING "None (printing disabled)"

void force_normal(HWND hwnd)
{
    static int recurse = 0;

    WINDOWPLACEMENT wp;

    if (recurse)
	return;
    recurse = 1;

    wp.length = sizeof(wp);
    if (GetWindowPlacement(hwnd, &wp) && wp.showCmd == SW_SHOWMAXIMIZED) {
	wp.showCmd = SW_SHOWNORMAL;
	SetWindowPlacement(hwnd, &wp);
    }
    recurse = 0;
}



/*
 * Null dialog procedure.
 */
static int CALLBACK NullDlgProc(HWND hwnd, UINT msg,
				WPARAM wParam, LPARAM lParam)
{
    return 0;
}

enum {
    IDCX_ABOUT = IDC_ABOUT,
    IDCX_TVSTATIC,
    IDCX_TREEVIEW,
    IDCX_STDBASE,
    IDCX_PANELBASE = IDCX_STDBASE + 32
};

struct treeview_faff {
    HWND treeview;
    HTREEITEM lastat[4];
};

static HTREEITEM treeview_insert(struct treeview_faff *faff,
				 int level, char *text, char *path)
{
    TVINSERTSTRUCT ins;
    int i;
    HTREEITEM newitem;
    ins.hParent = (level > 0 ? faff->lastat[level - 1] : TVI_ROOT);
    ins.hInsertAfter = faff->lastat[level];
#if _WIN32_IE >= 0x0400 && defined NONAMELESSUNION
#define INSITEM DUMMYUNIONNAME.item
#else
#define INSITEM item
#endif
    ins.INSITEM.mask = TVIF_TEXT | TVIF_PARAM;
    ins.INSITEM.pszText = text;
    ins.INSITEM.cchTextMax = strlen(text)+1;
    ins.INSITEM.lParam = (LPARAM)path;
    newitem = TreeView_InsertItem(faff->treeview, &ins);
    if (level > 0)
	TreeView_Expand(faff->treeview, faff->lastat[level - 1],
			(level > 1 ? TVE_COLLAPSE : TVE_EXPAND));
    faff->lastat[level] = newitem;
    for (i = level + 1; i < 4; i++)
	faff->lastat[i] = NULL;
    return newitem;
}

/*
 * Create the panelfuls of controls in the configuration box.
 */
static void create_controls(HWND hwnd, char *path)
{
    struct ctlpos cp;
    int index;
    int base_id;
    struct winctrls *wc;

    if (!path[0]) {
	/*
	 * Here we must create the basic standard controls.
	 */
	ctlposinit(&cp, hwnd, 3, 3, 245);
	wc = &ctrls_base;
	base_id = IDCX_STDBASE;
    } else {
	/*
	 * Otherwise, we're creating the controls for a particular
	 * panel.
	 */
	ctlposinit(&cp, hwnd, 100, 3, 13);
	wc = &ctrls_panel;
	base_id = IDCX_PANELBASE;
    }

    for (index=-1; (index = ctrl_find_path(ctrlbox, path, index)) >= 0 ;) {
	struct controlset *s = ctrlbox->ctrlsets[index];
	winctrl_layout(&dp, wc, &cp, s, &base_id);
    }
}


void show_help(HWND hwnd)
{
    launch_help(hwnd, NULL);
}

void defuse_showwindow(void)
{
    /*
     * Work around the fact that the app's first call to ShowWindow
     * will ignore the default in favour of the shell-provided
     * setting.
     */
    {
	HWND hwnd;
	hwnd = CreateDialog(hinst, MAKEINTRESOURCE(dialog_about), NULL, NullDlgProc);
	ShowWindow(hwnd, SW_HIDE);
	SetActiveWindow(hwnd);
	DestroyWindow(hwnd);
    }
}

/* ************************************************************ */
#include "windlg_inc.c"
/* ************************************************************ */
int verify_ssh_host_key(void *frontend, char *host, int port, char *keytype,
                        char *keystr, char *fingerprint,
                        void (*callback)(void *ctx, int result), void *ctx)
{
    int ret;

    static const char absentmsg[] =
	"The server's host key is not cached in the registry. You\n"
	"have no guarantee that the server is the computer you\n"
	"think it is.\n"
	"The server's %s key fingerprint is:\n"
	"%s\n"
	"If you trust this host, hit Yes to add the key to\n"
	"%s's cache and carry on connecting.\n"
	"If you want to carry on connecting just once, without\n"
	"adding the key to the cache, hit No.\n"
	"If you do not trust this host, hit Cancel to abandon the\n"
	"connection.\n";

    static const char wrongmsg[] =
	"WARNING - POTENTIAL SECURITY BREACH!\n"
	"\n"
	"The server's host key does not match the one %s has\n"
	"cached in the registry. This means that either the\n"
	"server administrator has changed the host key, or you\n"
	"have actually connected to another computer pretending\n"
	"to be the server.\n"
	"The new %s key fingerprint is:\n"
	"%s\n"
	"If you were expecting this change and trust the new key,\n"
	"hit Yes to update %s's cache and continue connecting.\n"
	"If you want to carry on connecting but without updating\n"
	"the cache, hit No.\n"
	"If you want to abandon the connection completely, hit\n"
	"Cancel. Hitting Cancel is the ONLY guaranteed safe\n" "choice.\n";

    static const char mbtitle[] = "%s Security Alert";

    /*
     * Verify the key against the registry.
     */
    ret = verify_host_key(host, port, keytype, keystr);

    if (ret == 0)		       /* success - key matched OK */
	return 1;
    else if (ret == 2) {	       /* key was different */
	int mbret;
	char *text = dupprintf(wrongmsg, appname, keytype, fingerprint,
			       appname);
	char *caption = dupprintf(mbtitle, appname);
	mbret = message_box(text, caption,
			    MB_ICONWARNING | MB_YESNOCANCEL | MB_DEFBUTTON3,
			    HELPCTXID(errors_hostkey_changed));
	assert(mbret==IDYES || mbret==IDNO || mbret==IDCANCEL);
	sfree(text);
	sfree(caption);
	if (mbret == IDYES) {
	    store_host_key(host, port, keytype, keystr);
	    return 1;
	} else if (mbret == IDNO)
	    return 1;
    } else if (ret == 1) {	       /* key was absent */
	int mbret;
	char *text = dupprintf(absentmsg, keytype, fingerprint, appname);
	char *caption = dupprintf(mbtitle, appname);
	mbret = message_box(text, caption,
			    MB_ICONWARNING | MB_YESNOCANCEL | MB_DEFBUTTON3,
			    HELPCTXID(errors_hostkey_absent));
	assert(mbret==IDYES || mbret==IDNO || mbret==IDCANCEL);
	sfree(text);
	sfree(caption);
	if (mbret == IDYES) {
	    store_host_key(host, port, keytype, keystr);
	    return 1;
	} else if (mbret == IDNO)
	    return 1;
    }
    return 0;	/* abandon the connection */
}

/*
 * Ask whether the selected algorithm is acceptable (since it was
 * below the configured 'warn' threshold).
 */
int askalg(void *frontend, const char *algtype, const char *algname,
	   void (*callback)(void *ctx, int result), void *ctx)
{
    static const char mbtitle[] = "%s Security Alert";
    static const char msg[] =
	"The first %s supported by the server\n"
	"is %.64s, which is below the configured\n"
	"warning threshold.\n"
	"Do you want to continue with this connection?\n";
    char *message, *title;
    int mbret;

    message = dupprintf(msg, algtype, algname);
    title = dupprintf(mbtitle, appname);
    mbret = MessageBox(NULL, message, title,
		       MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2);
    socket_reselect_all();
    sfree(message);
    sfree(title);
    if (mbret == IDYES)
	return 1;
    else
	return 0;
}

/*
 * Ask whether to wipe a session log file before writing to it.
 * Returns 2 for wipe, 1 for append, 0 for cancel (don't log).
 */
int askappend(void *frontend, Filename filename,
	      void (*callback)(void *ctx, int result), void *ctx)
{
    static const char msgtemplate[] =
	"The session log file \"%.*s\" already exists.\n"
	"You can overwrite it with a new session log,\n"
	"append your session log to the end of it,\n"
	"or disable session logging for this session.\n"
	"Hit Yes to wipe the file, No to append to it,\n"
	"or Cancel to disable logging.";
    char *message;
    char *mbtitle;
    int mbret;

    message = dupprintf(msgtemplate, FILENAME_MAX, filename.path);
    mbtitle = dupprintf("%s Log to File", appname);

    mbret = MessageBox(NULL, message, mbtitle,
		       MB_ICONQUESTION | MB_YESNOCANCEL | MB_DEFBUTTON3);

    socket_reselect_all();

    sfree(message);
    sfree(mbtitle);

    if (mbret == IDYES)
	return 2;
    else if (mbret == IDNO)
	return 1;
    else
	return 0;
}

/*
 * Warn about the obsolescent key file format.
 * 
 * Uniquely among these functions, this one does _not_ expect a
 * frontend handle. This means that if PuTTY is ported to a
 * platform which requires frontend handles, this function will be
 * an anomaly. Fortunately, the problem it addresses will not have
 * been present on that platform, so it can plausibly be
 * implemented as an empty function.
 */
void old_keyfile_warning(void)
{
    static const char mbtitle[] = "%s Key File Warning";
    static const char message[] =
	"You are loading an SSH-2 private key which has an\n"
	"old version of the file format. This means your key\n"
	"file is not fully tamperproof. Future versions of\n"
	"%s may stop supporting this private key format,\n"
	"so we recommend you convert your key to the new\n"
	"format.\n"
	"\n"
	"You can perform this conversion by loading the key\n"
	"into PuTTYgen and then saving it again.";

    char *msg, *title;
    msg = dupprintf(message, appname);
    title = dupprintf(mbtitle, appname);

    MessageBox(NULL, msg, title, MB_OK);

    socket_reselect_all();

    sfree(msg);
    sfree(title);
}
