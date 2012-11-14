/*
 * config.c - the platform-independent parts of the PuTTY
 * configuration box.
 */

#include <assert.h>
#include <stdlib.h>

#include "putty.h"
#include "dialog.h"
#include "storage.h"

#define PRINTER_DISABLED_STRING "None (printing disabled)"

#define HOST_BOX_TITLE "Host Name (or IP address)"
#define PORT_BOX_TITLE "Port"

/*
 * HACK: PuttyTray / PuTTY File
 * Define and struct moved up here so they can be used in storagetype_handler
 */
#define SAVEDSESSION_LEN 2048
struct sessionsaver_data {
    union control *editbox, *listbox, *loadbutton, *savebutton, *delbutton;
    union control *okbutton, *cancelbutton;
    struct sesslist sesslist;
    int midsession;
};


static void config_host_handler(union control *ctrl, void *dlg,
				void *data, int event)
{
    Config *cfg = (Config *)data;

    /*
     * This function works just like the standard edit box handler,
     * only it has to choose the control's label and text from two
     * different places depending on the protocol.
     */
    if (event == EVENT_REFRESH) {
	if (cfg->protocol == PROT_SERIAL) {
	    /*
	     * This label text is carefully chosen to contain an n,
	     * since that's the shortcut for the host name control.
	     */
	    dlg_label_change(ctrl, dlg, "Serial line");
	    dlg_editbox_set(ctrl, dlg, cfg->serline);
	} else {
	    dlg_label_change(ctrl, dlg, HOST_BOX_TITLE);
	    dlg_editbox_set(ctrl, dlg, cfg->host);
	}
    } else if (event == EVENT_VALCHANGE) {
	if (cfg->protocol == PROT_SERIAL)
	    dlg_editbox_get(ctrl, dlg, cfg->serline, lenof(cfg->serline));
	else
	    dlg_editbox_get(ctrl, dlg, cfg->host, lenof(cfg->host));
    }
}

static void config_port_handler(union control *ctrl, void *dlg,
				void *data, int event)
{
    Config *cfg = (Config *)data;
    char buf[80];

    /*
     * This function works similarly to the standard edit box handler,
     * only it has to choose the control's label and text from two
     * different places depending on the protocol.
     */
    if (event == EVENT_REFRESH) {
	if (cfg->protocol == PROT_SERIAL) {
	    /*
	     * This label text is carefully chosen to contain a p,
	     * since that's the shortcut for the port control.
	     */
	    dlg_label_change(ctrl, dlg, "Speed");
	    sprintf(buf, "%d", cfg->serspeed);
	} else {
	    dlg_label_change(ctrl, dlg, PORT_BOX_TITLE);
	    if (cfg->port != 0)
		sprintf(buf, "%d", cfg->port);
	    else
		/* Display an (invalid) port of 0 as blank */
		buf[0] = '\0';
	}
	dlg_editbox_set(ctrl, dlg, buf);
    } else if (event == EVENT_VALCHANGE) {
	dlg_editbox_get(ctrl, dlg, buf, lenof(buf));
	if (cfg->protocol == PROT_SERIAL)
	    cfg->serspeed = atoi(buf);
	else
	    cfg->port = atoi(buf);
    }
}

struct hostport {
    union control *host, *port;
};

/*
 * We export this function so that platform-specific config
 * routines can use it to conveniently identify the protocol radio
 * buttons in order to add to them.
 */
void config_protocolbuttons_handler(union control *ctrl, void *dlg,
				    void *data, int event)
{
    int button;
    Config *cfg = (Config *)data;
    struct hostport *hp = (struct hostport *)ctrl->radio.context.p;

    /*
     * This function works just like the standard radio-button
     * handler, except that it also has to change the setting of
     * the port box, and refresh both host and port boxes when. We
     * expect the context parameter to point at a hostport
     * structure giving the `union control's for both.
     */
    if (event == EVENT_REFRESH) {
	for (button = 0; button < ctrl->radio.nbuttons; button++)
	    if (cfg->protocol == ctrl->radio.buttondata[button].i)
		break;
	/* We expected that `break' to happen, in all circumstances. */
	assert(button < ctrl->radio.nbuttons);
	dlg_radiobutton_set(ctrl, dlg, button);
    } else if (event == EVENT_VALCHANGE) {
	int oldproto = cfg->protocol;
	button = dlg_radiobutton_get(ctrl, dlg);
	assert(button >= 0 && button < ctrl->radio.nbuttons);
	cfg->protocol = ctrl->radio.buttondata[button].i;
	if (oldproto != cfg->protocol) {
	    Backend *ob = backend_from_proto(oldproto);
	    Backend *nb = backend_from_proto(cfg->protocol);
	    assert(ob);
	    assert(nb);
	    /* Iff the user hasn't changed the port from the old protocol's
	     * default, update it with the new protocol's default.
	     * (This includes a "default" of 0, implying that there is no
	     * sensible default for that protocol; in this case it's
	     * displayed as a blank.)
	     * This helps with the common case of tabbing through the
	     * controls in order and setting a non-default port before
	     * getting to the protocol; we want that non-default port
	     * to be preserved. */
	    if (cfg->port == ob->default_port)
		cfg->port = nb->default_port;
	}
	dlg_refresh(hp->host, dlg);
	dlg_refresh(hp->port, dlg);
    }
}

/*
 * HACK: PuttyTray / PuTTY File
 * Storagetype radio buttons event handler
 */
void storagetype_handler(union control *ctrl, void *dlg, void *data, int event)
{
    int button;
    struct sessionsaver_data *ssd =(struct sessionsaver_data *)ctrl->generic.context.p;
	Config *cfg = (Config *)data;

    /*
     * For a standard radio button set, the context parameter gives
     * offsetof(targetfield, Config), and the extra data per button
     * gives the value the target field should take if that button
     * is the one selected.
     */
    if (event == EVENT_REFRESH) {
		button = cfg->session_storagetype; // Button index = same as storagetype number. Set according to config
		dlg_radiobutton_set(ctrl, dlg, button);
	} else if (event == EVENT_VALCHANGE) {
		button = dlg_radiobutton_get(ctrl, dlg);

		// Switch between registry and file
		if (ctrl->radio.buttondata[button].i == 0) {
			get_sesslist(&ssd->sesslist, FALSE, 0);
			get_sesslist(&ssd->sesslist, TRUE, 0);
			dlg_refresh(ssd->editbox, dlg);
			dlg_refresh(ssd->listbox, dlg);

			// Save setting into config (the whole *(int *)ATOFFSET(data, ctrl->radio.context.i) = ctrl->radio.buttondata[button].i; didn't work)
			// and I don't see why I shouldn't do it this way (it works?)
			cfg->session_storagetype = 0;
		} else {
			get_sesslist(&ssd->sesslist, FALSE, 1);
			get_sesslist(&ssd->sesslist, TRUE, 1);
			dlg_refresh(ssd->editbox, dlg);
			dlg_refresh(ssd->listbox, dlg);

			// Here as well
			cfg->session_storagetype = 1;
		}
	}
}
/** HACK: END **/

static void loggingbuttons_handler(union control *ctrl, void *dlg,
				   void *data, int event)
{
    int button;
    Config *cfg = (Config *)data;
    /* This function works just like the standard radio-button handler,
     * but it has to fall back to "no logging" in situations where the
     * configured logging type isn't applicable.
     */
    if (event == EVENT_REFRESH) {
        for (button = 0; button < ctrl->radio.nbuttons; button++)
            if (cfg->logtype == ctrl->radio.buttondata[button].i)
	        break;
    
    /* We fell off the end, so we lack the configured logging type */
    if (button == ctrl->radio.nbuttons) {
        button=0;
        cfg->logtype=LGTYP_NONE;
    }
    dlg_radiobutton_set(ctrl, dlg, button);
    } else if (event == EVENT_VALCHANGE) {
        button = dlg_radiobutton_get(ctrl, dlg);
        assert(button >= 0 && button < ctrl->radio.nbuttons);
        cfg->logtype = ctrl->radio.buttondata[button].i;
    }
}

static void numeric_keypad_handler(union control *ctrl, void *dlg,
				   void *data, int event)
{
    int button;
    Config *cfg = (Config *)data;
    /*
     * This function works much like the standard radio button
     * handler, but it has to handle two fields in Config.
     */
    if (event == EVENT_REFRESH) {
	if (cfg->nethack_keypad)
	    button = 2;
	else if (cfg->app_keypad)
	    button = 1;
	else
	    button = 0;
	assert(button < ctrl->radio.nbuttons);
	dlg_radiobutton_set(ctrl, dlg, button);
    } else if (event == EVENT_VALCHANGE) {
	button = dlg_radiobutton_get(ctrl, dlg);
	assert(button >= 0 && button < ctrl->radio.nbuttons);
	if (button == 2) {
	    cfg->app_keypad = FALSE;
	    cfg->nethack_keypad = TRUE;
	} else {
	    cfg->app_keypad = (button != 0);
	    cfg->nethack_keypad = FALSE;
	}
    }
}

static void cipherlist_handler(union control *ctrl, void *dlg,
			       void *data, int event)
{
    Config *cfg = (Config *)data;
    if (event == EVENT_REFRESH) {
	int i;

	static const struct { char *s; int c; } ciphers[] = {
	    { "3DES",			CIPHER_3DES },
	    { "Blowfish",		CIPHER_BLOWFISH },
	    { "DES",			CIPHER_DES },
	    { "AES (SSH-2 only)",	CIPHER_AES },
	    { "Arcfour (SSH-2 only)",	CIPHER_ARCFOUR },
	    { "-- warn below here --",	CIPHER_WARN }
	};

	/* Set up the "selected ciphers" box. */
	/* (cipherlist assumed to contain all ciphers) */
	dlg_update_start(ctrl, dlg);
	dlg_listbox_clear(ctrl, dlg);
	for (i = 0; i < CIPHER_MAX; i++) {
	    int c = cfg->ssh_cipherlist[i];
	    int j;
	    char *cstr = NULL;
	    for (j = 0; j < (sizeof ciphers) / (sizeof ciphers[0]); j++) {
		if (ciphers[j].c == c) {
		    cstr = ciphers[j].s;
		    break;
		}
	    }
	    dlg_listbox_addwithid(ctrl, dlg, cstr, c);
	}
	dlg_update_done(ctrl, dlg);

    } else if (event == EVENT_VALCHANGE) {
	int i;

	/* Update array to match the list box. */
	for (i=0; i < CIPHER_MAX; i++)
	    cfg->ssh_cipherlist[i] = dlg_listbox_getid(ctrl, dlg, i);

    }
}

#ifndef NO_GSSAPI
static void gsslist_handler(union control *ctrl, void *dlg,
			    void *data, int event)
{
    Config *cfg = (Config *)data;
    if (event == EVENT_REFRESH) {
	int i;

	dlg_update_start(ctrl, dlg);
	dlg_listbox_clear(ctrl, dlg);
	for (i = 0; i < ngsslibs; i++) {
	    int id = cfg->ssh_gsslist[i];
	    assert(id >= 0 && id < ngsslibs);
	    dlg_listbox_addwithid(ctrl, dlg, gsslibnames[id], id);
	}
	dlg_update_done(ctrl, dlg);

    } else if (event == EVENT_VALCHANGE) {
	int i;

	/* Update array to match the list box. */
	for (i=0; i < ngsslibs; i++)
	    cfg->ssh_gsslist[i] = dlg_listbox_getid(ctrl, dlg, i);
    }
}
#endif

static void kexlist_handler(union control *ctrl, void *dlg,
			    void *data, int event)
{
    Config *cfg = (Config *)data;
    if (event == EVENT_REFRESH) {
	int i;

	static const struct { char *s; int k; } kexes[] = {
	    { "Diffie-Hellman group 1",		KEX_DHGROUP1 },
	    { "Diffie-Hellman group 14",	KEX_DHGROUP14 },
	    { "Diffie-Hellman group exchange",	KEX_DHGEX },
	    { "RSA-based key exchange", 	KEX_RSA },
	    { "-- warn below here --",		KEX_WARN }
	};

	/* Set up the "kex preference" box. */
	/* (kexlist assumed to contain all algorithms) */
	dlg_update_start(ctrl, dlg);
	dlg_listbox_clear(ctrl, dlg);
	for (i = 0; i < KEX_MAX; i++) {
	    int k = cfg->ssh_kexlist[i];
	    int j;
	    char *kstr = NULL;
	    for (j = 0; j < (sizeof kexes) / (sizeof kexes[0]); j++) {
		if (kexes[j].k == k) {
		    kstr = kexes[j].s;
		    break;
		}
	    }
	    dlg_listbox_addwithid(ctrl, dlg, kstr, k);
	}
	dlg_update_done(ctrl, dlg);

    } else if (event == EVENT_VALCHANGE) {
	int i;

	/* Update array to match the list box. */
	for (i=0; i < KEX_MAX; i++)
	    cfg->ssh_kexlist[i] = dlg_listbox_getid(ctrl, dlg, i);

    }
}

static void printerbox_handler(union control *ctrl, void *dlg,
			       void *data, int event)
{
    Config *cfg = (Config *)data;
    if (event == EVENT_REFRESH) {
	int nprinters, i;
	printer_enum *pe;

	dlg_update_start(ctrl, dlg);
	/*
	 * Some backends may wish to disable the drop-down list on
	 * this edit box. Be prepared for this.
	 */
	if (ctrl->editbox.has_list) {
	    dlg_listbox_clear(ctrl, dlg);
	    dlg_listbox_add(ctrl, dlg, PRINTER_DISABLED_STRING);
	    pe = printer_start_enum(&nprinters);
	    for (i = 0; i < nprinters; i++)
		dlg_listbox_add(ctrl, dlg, printer_get_name(pe, i));
	    printer_finish_enum(pe);
	}
	dlg_editbox_set(ctrl, dlg,
			(*cfg->printer ? cfg->printer :
			 PRINTER_DISABLED_STRING));
	dlg_update_done(ctrl, dlg);
    } else if (event == EVENT_VALCHANGE) {
	dlg_editbox_get(ctrl, dlg, cfg->printer, sizeof(cfg->printer));
	if (!strcmp(cfg->printer, PRINTER_DISABLED_STRING))
	    *cfg->printer = '\0';
    }
}

static void codepage_handler(union control *ctrl, void *dlg,
			     void *data, int event)
{
    Config *cfg = (Config *)data;
    if (event == EVENT_REFRESH) {
	int i;
	const char *cp, *thiscp;
	dlg_update_start(ctrl, dlg);
	thiscp = cp_name(decode_codepage(cfg->line_codepage));
	dlg_listbox_clear(ctrl, dlg);
	for (i = 0; (cp = cp_enumerate(i)) != NULL; i++)
	    dlg_listbox_add(ctrl, dlg, cp);
	dlg_editbox_set(ctrl, dlg, thiscp);
	strcpy(cfg->line_codepage, thiscp);
	dlg_update_done(ctrl, dlg);
    } else if (event == EVENT_VALCHANGE) {
	dlg_editbox_get(ctrl, dlg, cfg->line_codepage,
			sizeof(cfg->line_codepage));
	strcpy(cfg->line_codepage,
	       cp_name(decode_codepage(cfg->line_codepage)));
    }
}

static void sshbug_handler(union control *ctrl, void *dlg,
			   void *data, int event)
{
    if (event == EVENT_REFRESH) {
	dlg_update_start(ctrl, dlg);
	dlg_listbox_clear(ctrl, dlg);
	dlg_listbox_addwithid(ctrl, dlg, "Auto", AUTO);
	dlg_listbox_addwithid(ctrl, dlg, "Off", FORCE_OFF);
	dlg_listbox_addwithid(ctrl, dlg, "On", FORCE_ON);
	switch (*(int *)ATOFFSET(data, ctrl->listbox.context.i)) {
	  case AUTO:      dlg_listbox_select(ctrl, dlg, 0); break;
	  case FORCE_OFF: dlg_listbox_select(ctrl, dlg, 1); break;
	  case FORCE_ON:  dlg_listbox_select(ctrl, dlg, 2); break;
	}
	dlg_update_done(ctrl, dlg);
    } else if (event == EVENT_SELCHANGE) {
	int i = dlg_listbox_index(ctrl, dlg);
	if (i < 0)
	    i = AUTO;
	else
	    i = dlg_listbox_getid(ctrl, dlg, i);
	*(int *)ATOFFSET(data, ctrl->listbox.context.i) = i;
    }
}

/* 
 * Helper function to load the session selected in the list box, if
 * any, as this is done in more than one place below. Returns 0 for
 * failure.
 */
static int load_selected_session(struct sessionsaver_data *ssd,
				 char *savedsession,
				 void *dlg, Config *cfg, int *maybe_launch)
{
    int i = dlg_listbox_index(ssd->listbox, dlg);
    int isdef;
    if (i < 0) {
	dlg_beep(dlg);
	return 0;
    }
    isdef = !strcmp(ssd->sesslist.sessions[i], "Default Settings");
    load_settings(ssd->sesslist.sessions[i], cfg);
    if (!isdef) {
	strncpy(savedsession, ssd->sesslist.sessions[i],
		SAVEDSESSION_LEN);
	savedsession[SAVEDSESSION_LEN-1] = '\0';
	if (maybe_launch)
	    *maybe_launch = TRUE;
    } else {
	savedsession[0] = '\0';
	if (maybe_launch)
	    *maybe_launch = FALSE;
    }
    dlg_refresh(NULL, dlg);
    /* Restore the selection, which might have been clobbered by
     * changing the value of the edit box. */
    dlg_listbox_select(ssd->listbox, dlg, i);
    return 1;
}

static void sessionsaver_handler(union control *ctrl, void *dlg,
				 void *data, int event)
{
    Config *cfg = (Config *)data;
    struct sessionsaver_data *ssd =
	(struct sessionsaver_data *)ctrl->generic.context.p;
    char *savedsession;

    /*
     * The first time we're called in a new dialog, we must
     * allocate space to store the current contents of the saved
     * session edit box (since it must persist even when we switch
     * panels, but is not part of the Config).
     */
    if (!ssd->editbox) {
        savedsession = NULL;
    } else if (!dlg_get_privdata(ssd->editbox, dlg)) {
	savedsession = (char *)
	    dlg_alloc_privdata(ssd->editbox, dlg, SAVEDSESSION_LEN);
	savedsession[0] = '\0';
    } else {
	savedsession = dlg_get_privdata(ssd->editbox, dlg);
    }

    if (event == EVENT_REFRESH) {
	if (ctrl == ssd->editbox) {
	    dlg_editbox_set(ctrl, dlg, savedsession);
	} else if (ctrl == ssd->listbox) {
	    int i;
	    dlg_update_start(ctrl, dlg);
	    dlg_listbox_clear(ctrl, dlg);
	    for (i = 0; i < ssd->sesslist.nsessions; i++)
		dlg_listbox_add(ctrl, dlg, ssd->sesslist.sessions[i]);
	    dlg_update_done(ctrl, dlg);
	}
    } else if (event == EVENT_VALCHANGE) {
        int top, bottom, halfway, i;
	if (ctrl == ssd->editbox) {
	    dlg_editbox_get(ctrl, dlg, savedsession,
			    SAVEDSESSION_LEN);
	    top = ssd->sesslist.nsessions;
	    bottom = -1;
	    while (top-bottom > 1) {
	        halfway = (top+bottom)/2;
	        i = strcmp(savedsession, ssd->sesslist.sessions[halfway]);
	        if (i <= 0 ) {
		    top = halfway;
	        } else {
		    bottom = halfway;
	        }
	    }
	    if (top == ssd->sesslist.nsessions) {
	        top -= 1;
	    }
	    dlg_listbox_select(ssd->listbox, dlg, top);
	}
    } else if (event == EVENT_ACTION) {
	int mbl = FALSE;
	if (!ssd->midsession &&
	    (ctrl == ssd->listbox ||
	     (ssd->loadbutton && ctrl == ssd->loadbutton))) {
	    /*
	     * The user has double-clicked a session, or hit Load.
	     * We must load the selected session, and then
	     * terminate the configuration dialog _if_ there was a
	     * double-click on the list box _and_ that session
	     * contains a hostname.
	     */
	    if (load_selected_session(ssd, savedsession, dlg, cfg, &mbl) &&
		(mbl && ctrl == ssd->listbox && cfg_launchable(cfg))) {
		dlg_end(dlg, 1);       /* it's all over, and succeeded */
	    }
	} else if (ctrl == ssd->savebutton) {
	    int isdef = !strcmp(savedsession, "Default Settings");
	    if (!savedsession[0]) {
		int i = dlg_listbox_index(ssd->listbox, dlg);
		if (i < 0) {
		    dlg_beep(dlg);
		    return;
		}
		isdef = !strcmp(ssd->sesslist.sessions[i], "Default Settings");
		if (!isdef) {
		    strncpy(savedsession, ssd->sesslist.sessions[i],
			    SAVEDSESSION_LEN);
		    savedsession[SAVEDSESSION_LEN-1] = '\0';
		} else {
		    savedsession[0] = '\0';
		}
	    }
            {
                char *errmsg = save_settings(savedsession, cfg);
                if (errmsg) {
                    dlg_error_msg(dlg, errmsg);
                    sfree(errmsg);
                }
            }
		/*
		 * HACK: PuttyTray / PuTTY File
		 * Added storagetype to get_sesslist
		 */
	    get_sesslist(&ssd->sesslist, FALSE, cfg->session_storagetype);
	    get_sesslist(&ssd->sesslist, TRUE, cfg->session_storagetype);

	    dlg_refresh(ssd->editbox, dlg);
	    dlg_refresh(ssd->listbox, dlg);
	} else if (!ssd->midsession &&
		   ssd->delbutton && ctrl == ssd->delbutton) {
	    int i = dlg_listbox_index(ssd->listbox, dlg);
	    if (i <= 0) {
		dlg_beep(dlg);
	    } else {
		del_settings(ssd->sesslist.sessions[i]);

		/*
		 * HACK: PuttyTray / PuTTY File
		 * Added storagetype to get_sesslist
		 */
		get_sesslist(&ssd->sesslist, FALSE, cfg->session_storagetype);
		get_sesslist(&ssd->sesslist, TRUE, cfg->session_storagetype);

		dlg_refresh(ssd->listbox, dlg);
	    }
	} else if (ctrl == ssd->okbutton) {
            if (ssd->midsession) {
                /* In a mid-session Change Settings, Apply is always OK. */
		dlg_end(dlg, 1);
                return;
            }
	    /*
	     * Annoying special case. If the `Open' button is
	     * pressed while no host name is currently set, _and_
	     * the session list previously had the focus, _and_
	     * there was a session selected in that which had a
	     * valid host name in it, then load it and go.
	     */
	    if (dlg_last_focused(ctrl, dlg) == ssd->listbox &&
		!cfg_launchable(cfg)) {
		Config cfg2;
		int mbl = FALSE;
		if (!load_selected_session(ssd, savedsession, dlg,
					   &cfg2, &mbl)) {
		    dlg_beep(dlg);
		    return;
		}
		/* If at this point we have a valid session, go! */
		if (mbl && cfg_launchable(&cfg2)) {
		    *cfg = cfg2;       /* structure copy */
		    cfg->remote_cmd_ptr = NULL;
		    dlg_end(dlg, 1);
		} else
		    dlg_beep(dlg);
                return;
	    }

	    /*
	     * Otherwise, do the normal thing: if we have a valid
	     * session, get going.
	     */
	    if (cfg_launchable(cfg)) {
		dlg_end(dlg, 1);
	    } else
		dlg_beep(dlg);
	} else if (ctrl == ssd->cancelbutton) {
	    dlg_end(dlg, 0);
	}
    }
}

struct charclass_data {
    union control *listbox, *editbox, *button;
};

static void charclass_handler(union control *ctrl, void *dlg,
			      void *data, int event)
{
    Config *cfg = (Config *)data;
    struct charclass_data *ccd =
	(struct charclass_data *)ctrl->generic.context.p;

    if (event == EVENT_REFRESH) {
	if (ctrl == ccd->listbox) {
	    int i;
	    dlg_update_start(ctrl, dlg);
	    dlg_listbox_clear(ctrl, dlg);
	    for (i = 0; i < 128; i++) {
		char str[100];
		sprintf(str, "%d\t(0x%02X)\t%c\t%d", i, i,
			(i >= 0x21 && i != 0x7F) ? i : ' ', cfg->wordness[i]);
		dlg_listbox_add(ctrl, dlg, str);
	    }
	    dlg_update_done(ctrl, dlg);
	}
    } else if (event == EVENT_ACTION) {
	if (ctrl == ccd->button) {
	    char str[100];
	    int i, n;
	    dlg_editbox_get(ccd->editbox, dlg, str, sizeof(str));
	    n = atoi(str);
	    for (i = 0; i < 128; i++) {
		if (dlg_listbox_issel(ccd->listbox, dlg, i))
		    cfg->wordness[i] = n;
	    }
	    dlg_refresh(ccd->listbox, dlg);
	}
    }
}

struct colour_data {
    union control *listbox, *redit, *gedit, *bedit, *button;
};

static const char *const colours[] = {
    "Default Foreground", "Default Bold Foreground",
    "Default Background", "Default Bold Background",
    "Cursor Text", "Cursor Colour",
    "ANSI Black", "ANSI Black Bold",
    "ANSI Red", "ANSI Red Bold",
    "ANSI Green", "ANSI Green Bold",
    "ANSI Yellow", "ANSI Yellow Bold",
    "ANSI Blue", "ANSI Blue Bold",
    "ANSI Magenta", "ANSI Magenta Bold",
    "ANSI Cyan", "ANSI Cyan Bold",
    "ANSI White", "ANSI White Bold"
};

static void colour_handler(union control *ctrl, void *dlg,
			    void *data, int event)
{
    Config *cfg = (Config *)data;
    struct colour_data *cd =
	(struct colour_data *)ctrl->generic.context.p;
    int update = FALSE, clear = FALSE, r, g, b;

    if (event == EVENT_REFRESH) {
	if (ctrl == cd->listbox) {
	    int i;
	    dlg_update_start(ctrl, dlg);
	    dlg_listbox_clear(ctrl, dlg);
	    for (i = 0; i < lenof(colours); i++)
		dlg_listbox_add(ctrl, dlg, colours[i]);
	    dlg_update_done(ctrl, dlg);
	    clear = TRUE;
	    update = TRUE;
	}
    } else if (event == EVENT_SELCHANGE) {
	if (ctrl == cd->listbox) {
	    /* The user has selected a colour. Update the RGB text. */
	    int i = dlg_listbox_index(ctrl, dlg);
	    if (i < 0) {
		clear = TRUE;
	    } else {
		clear = FALSE;
		r = cfg->colours[i][0];
		g = cfg->colours[i][1];
		b = cfg->colours[i][2];
	    }
	    update = TRUE;
	}
    } else if (event == EVENT_VALCHANGE) {
	if (ctrl == cd->redit || ctrl == cd->gedit || ctrl == cd->bedit) {
	    /* The user has changed the colour using the edit boxes. */
	    char buf[80];
	    int i, cval;

	    dlg_editbox_get(ctrl, dlg, buf, lenof(buf));
	    cval = atoi(buf);
	    if (cval > 255) cval = 255;
	    if (cval < 0)   cval = 0;

	    i = dlg_listbox_index(cd->listbox, dlg);
	    if (i >= 0) {
		if (ctrl == cd->redit)
		    cfg->colours[i][0] = cval;
		else if (ctrl == cd->gedit)
		    cfg->colours[i][1] = cval;
		else if (ctrl == cd->bedit)
		    cfg->colours[i][2] = cval;
	    }
	}
    } else if (event == EVENT_ACTION) {
	if (ctrl == cd->button) {
	    int i = dlg_listbox_index(cd->listbox, dlg);
	    if (i < 0) {
		dlg_beep(dlg);
		return;
	    }
	    /*
	     * Start a colour selector, which will send us an
	     * EVENT_CALLBACK when it's finished and allow us to
	     * pick up the results.
	     */
	    dlg_coloursel_start(ctrl, dlg,
				cfg->colours[i][0],
				cfg->colours[i][1],
				cfg->colours[i][2]);
	}
    } else if (event == EVENT_CALLBACK) {
	if (ctrl == cd->button) {
	    int i = dlg_listbox_index(cd->listbox, dlg);
	    /*
	     * Collect the results of the colour selector. Will
	     * return nonzero on success, or zero if the colour
	     * selector did nothing (user hit Cancel, for example).
	     */
	    if (dlg_coloursel_results(ctrl, dlg, &r, &g, &b)) {
		cfg->colours[i][0] = r;
		cfg->colours[i][1] = g;
		cfg->colours[i][2] = b;
		clear = FALSE;
		update = TRUE;
	    }
	}
    }

    if (update) {
	if (clear) {
	    dlg_editbox_set(cd->redit, dlg, "");
	    dlg_editbox_set(cd->gedit, dlg, "");
	    dlg_editbox_set(cd->bedit, dlg, "");
	} else {
	    char buf[40];
	    sprintf(buf, "%d", r); dlg_editbox_set(cd->redit, dlg, buf);
	    sprintf(buf, "%d", g); dlg_editbox_set(cd->gedit, dlg, buf);
	    sprintf(buf, "%d", b); dlg_editbox_set(cd->bedit, dlg, buf);
	}
    }
}

struct ttymodes_data {
    union control *modelist, *valradio, *valbox;
    union control *addbutton, *rembutton, *listbox;
};

static void ttymodes_handler(union control *ctrl, void *dlg,
			     void *data, int event)
{
    Config *cfg = (Config *)data;
    struct ttymodes_data *td =
	(struct ttymodes_data *)ctrl->generic.context.p;

    if (event == EVENT_REFRESH) {
	if (ctrl == td->listbox) {
	    char *p = cfg->ttymodes;
	    dlg_update_start(ctrl, dlg);
	    dlg_listbox_clear(ctrl, dlg);
	    while (*p) {
		int tabpos = strchr(p, '\t') - p;
		char *disp = dupprintf("%.*s\t%s", tabpos, p,
				       (p[tabpos+1] == 'A') ? "(auto)" :
				       p+tabpos+2);
		dlg_listbox_add(ctrl, dlg, disp);
		p += strlen(p) + 1;
		sfree(disp);
	    }
	    dlg_update_done(ctrl, dlg);
	} else if (ctrl == td->modelist) {
	    int i;
	    dlg_update_start(ctrl, dlg);
	    dlg_listbox_clear(ctrl, dlg);
	    for (i = 0; ttymodes[i]; i++)
		dlg_listbox_add(ctrl, dlg, ttymodes[i]);
	    dlg_listbox_select(ctrl, dlg, 0); /* *shrug* */
	    dlg_update_done(ctrl, dlg);
	} else if (ctrl == td->valradio) {
	    dlg_radiobutton_set(ctrl, dlg, 0);
	}
    } else if (event == EVENT_ACTION) {
	if (ctrl == td->addbutton) {
	    int ind = dlg_listbox_index(td->modelist, dlg);
	    if (ind >= 0) {
		char type = dlg_radiobutton_get(td->valradio, dlg) ? 'V' : 'A';
		int slen, left;
		char *p, str[lenof(cfg->ttymodes)];
		/* Construct new entry */
		memset(str, 0, lenof(str));
		strncpy(str, ttymodes[ind], lenof(str)-3);
		slen = strlen(str);
		str[slen] = '\t';
		str[slen+1] = type;
		slen += 2;
		if (type == 'V') {
		    dlg_editbox_get(td->valbox, dlg, str+slen, lenof(str)-slen);
		}
		/* Find end of list, deleting any existing instance */
		p = cfg->ttymodes;
		left = lenof(cfg->ttymodes);
		while (*p) {
		    int t = strchr(p, '\t') - p;
		    if (t == strlen(ttymodes[ind]) &&
			strncmp(p, ttymodes[ind], t) == 0) {
			memmove(p, p+strlen(p)+1, left - (strlen(p)+1));
			continue;
		    }
		    left -= strlen(p) + 1;
		    p    += strlen(p) + 1;
		}
		/* Append new entry */
		memset(p, 0, left);
		strncpy(p, str, left - 2);
		dlg_refresh(td->listbox, dlg);
	    } else
		dlg_beep(dlg);
	} else if (ctrl == td->rembutton) {
	    char *p = cfg->ttymodes;
	    int i = 0, len = lenof(cfg->ttymodes);
	    while (*p) {
		int multisel = dlg_listbox_index(td->listbox, dlg) < 0;
		if (dlg_listbox_issel(td->listbox, dlg, i)) {
		    if (!multisel) {
			/* Populate controls with entry we're about to
			 * delete, for ease of editing.
			 * (If multiple entries were selected, don't
			 * touch the controls.) */
			char *val = strchr(p, '\t');
			if (val) {
			    int ind = 0;
			    val++;
			    while (ttymodes[ind]) {
				if (strlen(ttymodes[ind]) == val-p-1 &&
				    !strncmp(ttymodes[ind], p, val-p-1))
				    break;
				ind++;
			    }
			    dlg_listbox_select(td->modelist, dlg, ind);
			    dlg_radiobutton_set(td->valradio, dlg,
						(*val == 'V'));
			    dlg_editbox_set(td->valbox, dlg, val+1);
			}
		    }
		    memmove(p, p+strlen(p)+1, len - (strlen(p)+1));
		    i++;
		    continue;
		}
		len -= strlen(p) + 1;
		p   += strlen(p) + 1;
		i++;
	    }
	    memset(p, 0, lenof(cfg->ttymodes) - len);
	    dlg_refresh(td->listbox, dlg);
	}
    }
}

struct environ_data {
    union control *varbox, *valbox, *addbutton, *rembutton, *listbox;
};

static void environ_handler(union control *ctrl, void *dlg,
			    void *data, int event)
{
    Config *cfg = (Config *)data;
    struct environ_data *ed =
	(struct environ_data *)ctrl->generic.context.p;

    if (event == EVENT_REFRESH) {
	if (ctrl == ed->listbox) {
	    char *p = cfg->environmt;
	    dlg_update_start(ctrl, dlg);
	    dlg_listbox_clear(ctrl, dlg);
	    while (*p) {
		dlg_listbox_add(ctrl, dlg, p);
		p += strlen(p) + 1;
	    }
	    dlg_update_done(ctrl, dlg);
	}
    } else if (event == EVENT_ACTION) {
	if (ctrl == ed->addbutton) {
	    char str[sizeof(cfg->environmt)];
	    char *p;
	    dlg_editbox_get(ed->varbox, dlg, str, sizeof(str)-1);
	    if (!*str) {
		dlg_beep(dlg);
		return;
	    }
	    p = str + strlen(str);
	    *p++ = '\t';
	    dlg_editbox_get(ed->valbox, dlg, p, sizeof(str)-1 - (p - str));
	    if (!*p) {
		dlg_beep(dlg);
		return;
	    }
	    p = cfg->environmt;
	    while (*p) {
		while (*p)
		    p++;
		p++;
	    }
	    if ((p - cfg->environmt) + strlen(str) + 2 <
		sizeof(cfg->environmt)) {
		strcpy(p, str);
		p[strlen(str) + 1] = '\0';
		dlg_listbox_add(ed->listbox, dlg, str);
		dlg_editbox_set(ed->varbox, dlg, "");
		dlg_editbox_set(ed->valbox, dlg, "");
	    } else {
		dlg_error_msg(dlg, "Environment too big");
	    }
	} else if (ctrl == ed->rembutton) {
	    int i = dlg_listbox_index(ed->listbox, dlg);
	    if (i < 0) {
		dlg_beep(dlg);
	    } else {
		char *p, *q, *str;

		dlg_listbox_del(ed->listbox, dlg, i);
		p = cfg->environmt;
		while (i > 0) {
		    if (!*p)
			goto disaster;
		    while (*p)
			p++;
		    p++;
		    i--;
		}
		q = p;
		if (!*p)
		    goto disaster;
		/* Populate controls with the entry we're about to delete
		 * for ease of editing */
		str = p;
		p = strchr(p, '\t');
		if (!p)
		    goto disaster;
		*p = '\0';
		dlg_editbox_set(ed->varbox, dlg, str);
		p++;
		str = p;
		dlg_editbox_set(ed->valbox, dlg, str);
		p = strchr(p, '\0');
		if (!p)
		    goto disaster;
		p++;
		while (*p) {
		    while (*p)
			*q++ = *p++;
		    *q++ = *p++;
		}
		*q = '\0';
		disaster:;
	    }
	}
    }
}

struct portfwd_data {
    union control *addbutton, *rembutton, *listbox;
    union control *sourcebox, *destbox, *direction;
#ifndef NO_IPV6
    union control *addressfamily;
#endif
};

static void portfwd_handler(union control *ctrl, void *dlg,
			    void *data, int event)
{
    Config *cfg = (Config *)data;
    struct portfwd_data *pfd =
	(struct portfwd_data *)ctrl->generic.context.p;

    if (event == EVENT_REFRESH) {
	if (ctrl == pfd->listbox) {
	    char *p = cfg->portfwd;
	    dlg_update_start(ctrl, dlg);
	    dlg_listbox_clear(ctrl, dlg);
	    while (*p) {
		dlg_listbox_add(ctrl, dlg, p);
		p += strlen(p) + 1;
	    }
	    dlg_update_done(ctrl, dlg);
	} else if (ctrl == pfd->direction) {
	    /*
	     * Default is Local.
	     */
	    dlg_radiobutton_set(ctrl, dlg, 0);
#ifndef NO_IPV6
	} else if (ctrl == pfd->addressfamily) {
	    dlg_radiobutton_set(ctrl, dlg, 0);
#endif
	}
    } else if (event == EVENT_ACTION) {
	if (ctrl == pfd->addbutton) {
	    char str[sizeof(cfg->portfwd)];
	    char *p;
	    int i, type;
	    int whichbutton;

	    i = 0;
#ifndef NO_IPV6
	    whichbutton = dlg_radiobutton_get(pfd->addressfamily, dlg);
	    if (whichbutton == 1)
		str[i++] = '4';
	    else if (whichbutton == 2)
		str[i++] = '6';
#endif

	    whichbutton = dlg_radiobutton_get(pfd->direction, dlg);
	    if (whichbutton == 0)
		type = 'L';
	    else if (whichbutton == 1)
		type = 'R';
	    else
		type = 'D';
	    str[i++] = type;

	    dlg_editbox_get(pfd->sourcebox, dlg, str+i, sizeof(str) - i);
	    if (!str[i]) {
		dlg_error_msg(dlg, "You need to specify a source port number");
		return;
	    }
	    p = str + strlen(str);
	    if (type != 'D') {
		*p++ = '\t';
		dlg_editbox_get(pfd->destbox, dlg, p,
				sizeof(str) - (p - str));
		if (!*p || !strchr(p, ':')) {
		    dlg_error_msg(dlg,
				  "You need to specify a destination address\n"
				  "in the form \"host.name:port\"");
		    return;
		}
	    } else
		*p = '\0';
	    p = cfg->portfwd;
	    while (*p) {
		if (strcmp(p,str) == 0) {
		    dlg_error_msg(dlg, "Specified forwarding already exists");
		    break;
		}
		while (*p)
		    p++;
		p++;
	    }
	    if (!*p) {
		if ((p - cfg->portfwd) + strlen(str) + 2 <=
		    sizeof(cfg->portfwd)) {
		    strcpy(p, str);
		    p[strlen(str) + 1] = '\0';
		    dlg_listbox_add(pfd->listbox, dlg, str);
		    dlg_editbox_set(pfd->sourcebox, dlg, "");
		    dlg_editbox_set(pfd->destbox, dlg, "");
		} else {
		    dlg_error_msg(dlg, "Too many forwardings");
		}
	    }
	} else if (ctrl == pfd->rembutton) {
	    int i = dlg_listbox_index(pfd->listbox, dlg);
	    if (i < 0)
		dlg_beep(dlg);
	    else {
		char *p, *q, *src, *dst;
		char dir;

		dlg_listbox_del(pfd->listbox, dlg, i);
		p = cfg->portfwd;
		while (i > 0) {
		    if (!*p)
			goto disaster2;
		    while (*p)
			p++;
		    p++;
		    i--;
		}
		q = p;
		if (!*p)
		    goto disaster2;
		/* Populate the controls with the entry we're about to
		 * delete, for ease of editing. */
		{
		    static const char *const afs = "A46";
		    char *afp = strchr(afs, *p);
#ifndef NO_IPV6
		    int idx = afp ? afp-afs : 0;
#endif
		    if (afp)
			p++;
#ifndef NO_IPV6
		    dlg_radiobutton_set(pfd->addressfamily, dlg, idx);
#endif
		}
		{
		    static const char *const dirs = "LRD";
		    dir = *p;
		    dlg_radiobutton_set(pfd->direction, dlg,
					strchr(dirs, dir) - dirs);
		}
		p++;
		if (dir != 'D') {
		    src = p;
		    p = strchr(p, '\t');
		    if (!p)
			goto disaster2;
		    *p = '\0';
		    p++;
		    dst = p;
		} else {
		    src = p;
		    dst = "";
		}
		p = strchr(p, '\0');
		if (!p)
		    goto disaster2;
		dlg_editbox_set(pfd->sourcebox, dlg, src);
		dlg_editbox_set(pfd->destbox, dlg, dst);
		p++;
		while (*p) {
		    while (*p)
			*q++ = *p++;
		    *q++ = *p++;
		}
		*q = '\0';
		disaster2:;
	    }
	}
    }
}

