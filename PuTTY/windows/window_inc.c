#include <winsock2.h>
#include <windows.h>
#include <string.h>
#include "win_res.h"
#include <stdlib.h>
#include "storage.h"
#include <time.h>
#include <stdio.h>
#include <math.h>

#define snprintf sprintf_s
#define ATTR_INVALID 0x03FFFFU
#define ATTR_NARROW 0x800000U
#define ATTR_WIDE 0x400000U
#define ATTR_BLINK 0x200000U
#define ATTR_REVERSE 0x100000U
#define ATTR_UNDER 0x080000U
#define ATTR_BOLD 0x040000U
#define ATTR_FGSHIFT 0
#define ATTR_BGSHIFT 9
#define ATTR_FGMASK 0x0001FFU
#define ATTR_BGMASK 0x03FE00U
#define ATTR_COLOURS 0x03FFFFU
#define COLOR_DEFFG 256
#define COLOR_DEFFG_BOLD 257
#define COLOR_DEFBG 258
#define COLOR_DEFBG_BOLD 259
#define COLOR_CURFG 260
#define COLOR_CURBG 261
#define ATTR_DEFFG (COLOR_DEFFG<<ATTR_FGSHIFT)
#define ATTR_DEFBG (COLOR_DEFBG<<ATTR_BGSHIFT)
#define ATTR_DEFAULT (ATTR_DEFFG | ATTR_DEFBG)
#define VT_XWINDOWS 0
#define VT_OEMANSI 1
#define VT_OEMONLY 2
#define VT_POORMAN 3
#define VT_UNICODE 4
#define CSET_MASK 0xFFFFFF00UL
#define DIRECT_FONT(c) ((c&0xFFFFFE00)==0xF000)
#define CSET_OEMCP 0x0000F000UL
#define CSET_ACP 0x0000F100UL
#define DIRECT_CHAR(c) ((c&0xFFFFFC00)==0xD800)
#define CSET_ASCII 0x0000D800UL
#define CSET_LINEDRW 0x0000D900UL
#define CSET_SCOACS 0x0000DA00UL
#define CSET_GBCHR 0x0000DB00UL
#define CSET_OF(c) (DIRECT_CHAR(c)||DIRECT_FONT(c)?(c)&CSET_MASK:0)
#define UCSWIDE 0xDFFF
#define UCSERR (CSET_LINEDRW|'a')
#define WM_NOTIFY_PUTTYTRAY (WM_USER + 1983)
#define FONT_NORMAL 0
#define FONT_BOLD 1
#define FONT_UNDERLINE 2
#define FONT_BOLDUND 3
#define FONT_WIDE 0x04
#define FONT_HIGH 0x08
#define FONT_NARROW 0x10
#define FONT_OEM 0x20
#define FONT_OEMBOLD 0x21
#define FONT_OEMUND 0x22
#define FONT_OEMBOLDUND 0x23
#define FONT_SHIFT 5
#define FONT_MAXNO 0x2F
#define WS_EX_LAYERED 0x00080000
#define LWA_ALPHA 0x00000002
#define MENU_SYS 0
#define MENU_CTX 1
#define TIMING_TIMER_ID 1234
#define NALLCOLOURS 16+240+6
#define CTRL(x) (x^'@')
#define KCTRL(x) ((x^'@')|0x100)
#define ECHOING cfg.localecho==FORCE_ON
#define EDITING cfg.localedit==FORCE_ON
#define in_utf line_codepage==CP_UTF8

enum {BOLD_COLOURS, BOLD_SHADOW, BOLD_FONT} bold_mode;
enum {UND_LINE, UND_FONT} und_mode;

typedef void * TERM;

struct tagWindow {
	HWND hwnd;
};

struct timer_node {
	timer_fn_t fn;
	void * ctx;
	long time_val;
	char * s_name;
	struct timer_node * next;
};

struct timer_list {
	long time_now;
	int n;
	struct timer_node* head;
	struct timer_node* tail;
};

struct term_userpass_state {
	int n_pos_prompt;
	int tb_done_prompt;
	int n_pos_cursor;
};

long schedule_timer(int tn_ticks, timer_fn_t fn, void * ctx);
void expire_timer_context(void * ctx);
void timer_change_notify(long time_next);
void start_backend();
void close_session();
void cleanup_exit(int n_code);
char * do_select(SOCKET skt, int tb_startup);
void connection_fatal(void * frontend, char * fmt, ...);
void cmdline_error(char * fmt, ...);
void fatalbox(char * fmt, ...);
void modalfatalbox(char * fmt, ...);
void notify_remote_exit(void * frontend);
void enact_pending_netevent();
int taskbar_addicon(char * ts_tip, int tb_show);
void tray_updatemenu(int disableMenuItems);
void init_fonts(int n_tw, int n_th);
void deinit_fonts();
void another_font(int n);
int get_font_width(HDC hdc, TEXTMETRIC * tm);
void general_textout(HDC hdc, int x, int y, CONST RECT * lprc, unsigned short * lpString, int n_len, CONST INT * lpDx_buf);
void do_text_internal(int x, int y, wchar_t * text, int n_len, unsigned long n_attr, int n_lattr, int b_opaque);
void do_text(int x, int y, wchar_t * text, int n_len, unsigned long n_attr, int n_lattr);
void do_cursor(int x, int y, wchar_t * text, int n_len, unsigned long n_attr, int n_lattr);
int char_width(int tn_char);
void write_clip(void * frontend, wchar_t * ws_data, int* pn_attr, int n_len, int b_must_deselect);
void write_aclip(void * frontend, char * s_data, int n_len, int b_must_deselect);
void request_paste(void * frontend);
DWORD WINAPI thread_read_clipboard(void * param);
int process_clipdata(HGLOBAL clipdata, int b_unicode);
void request_resize(void * frontend, int tn_col, int tn_row);
void reset_window(int n_state);
void make_full_screen();
void clear_full_screen();
int is_full_screen();
int pt_on_topleft(POINT pt);
void do_beep();
void set_title(void * frontend, char * ts_title);
void set_icon(void * frontend, char * ts_title);
void front_set_scrollbar(int tn_total, int tn_pos, int tn_pagesize);
int paint_start();
void paint_finish();
void on_timer_flash_window(void * ctx, long n_now);
void flash_window(int mode);
void sys_cursor(void * frontend, int x, int y);
void sys_cursor_update();
int MakeWindowTransparent(HWND hWnd, int n_alpha);
char * get_ttymode(void * frontend, const char * s_mode);
void set_iconic(void * frontend, int b_iconic);
void move_window(void * frontend, int x, int y);
void set_zorder(void * frontend, int b_top);
void refresh_window(void * frontend);
void set_zoomed(void * frontend, int b_zoomed);
int is_iconic(void * frontend);
void get_window_pos(void * frontend, int* pn_x, int* pn_y);
void get_window_pixels(void * frontend, int* pn_x, int* pn_y);
char * get_window_title(void * frontend, int b_icon);
int from_backend(void * frontend, int is_stderr, const char * data, int len);
int from_backend_untrusted(void * frontend, const char * data, int len);
void frontend_keypress(void * handle);
void update_specials_menu(void * frontend);
int is_alt_pressed();
int is_ctrl_pressed();
void set_input_locale(HKL kl);
void show_mouseptr(int b_show);
void palette_set(void * frontend, int tn_i, int tn_r, int tn_g, int tn_b);
void palette_reset(void * frontend);
void set_busy_status(void * frontend, int tn_status);
void set_raw_mouse_mode(void * frontend, int tn_activate);
void ldisc_update(void * frontend, int echo, int edit);
void linedisc_send(char * s_buf, int n_len);
void linedisc_send_special(char * s_buf, int n_len);
void ldisc_edit(char * s_buf, int n_len);
int get_plen(unsigned char c);
void do_pwrite(unsigned char c);
void linedisc_send_codepage(int n_codepage, char * s_buf, int n_len);
void linedisc_send_unicode(wchar_t * p_wbuf, int n_len);
int get_userpass_input(prompts_t * p_prompts, unsigned char * ts_in, int tn_inlen);
LRESULT CALLBACK WndProc_main(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

HICON extract_icon(char *iconpath, int smallicon);
void term_invalidate(TERM);
void term_scroll(TERM, int, int);
int term_data_untrusted(TERM, const char *, int);
int term_data(TERM, const char *, int);
void term_update(TERM);
void term_invalidate_rect(TERM, int, int, int, int);
void update_ucs_charset(DWORD n_cs);
void update_ucs_line_codepage(n_codepage);
void update_ucs_linedraw(int n_vtmode);
void term_size(TERM, int, int);
void term_size(TERM, int, int);
void term_size(TERM, int, int);
void term_seen_key_event(TERM);
void term_nopaste(TERM);
void term_clrsb(TERM);
void term_copyall(TERM);
void term_pwron(TERM, int);
void term_deselect(TERM);
void term_size(TERM, int, int);
int term_app_cursor_keys(TERM);
void term_size(TERM, int, int);
void term_size(TERM, int, int);
void term_size(TERM, int, int);
TERM term_init(Config *);
void term_set_scrollback_size(TERM, int);
void term_size(TERM, int, int);

struct tagWindow win;
struct timer_list timer_list;
int compose_state=0;
int compose_char;
int compose_key;
WORD keys[3];
TERM term;
Backend * back;
void * backhandle;
char * window_name;
char * icon_name;
int must_close_session;
int session_closed;
int b_pending_netevent=0;
int n_pend_netevent_wparam;
int n_pend_netevent_lparam;
int puttyTrayVisible;
int puttyTrayFlash;
HICON puttyTrayFlashIcon;
int windowMinimized=0;
NOTIFYICONDATA puttyTray;
HFONT fonts[FONT_MAXNO];
int fontflag[FONT_MAXNO];
int font_width;
int font_height;
int font_isdbcs;
extern int font_codepage;
extern int line_codepage;
int font_dualwidth;
int font_varpitch;
int font_descent;
extern wchar_t unitab_scoacs[256];
extern wchar_t unitab_line[256];
extern wchar_t unitab_font[256];
extern wchar_t unitab_draw[256];
extern wchar_t unitab_oemcp[256];
extern unsigned char unitab_ctrl[256];
int n_ignore_clip=0;
wchar_t * clipboard_contents=NULL;
int clipboard_length;
wchar_t sel_nl[]={13,10};
int n_sel_nl=2;
int offset_width;
int offset_height;
int extra_width;
int extra_height;
int n_rows;
int n_cols;
int n_win_x;
int n_win_y;
int n_win_w;
int n_win_h;
int b_fullscreen=0;
HDC cur_hdc=0;
int n_time_nextflash;
int n_is_flashing;
int caret_x;
int caret_y;
time_t time_last_reconnect=0;
HMENU popup_menus[2];
struct sesslist sesslist;
HMENU menu_savedsess;
const struct telnet_special * specials= NULL;
HMENU menu_specials= NULL;
int n_specials=0;
int kbd_codepage;
COLORREF colours[NALLCOLOURS];
char * s_ldisc_buf=NULL;
int n_ldisc_len=0;
int n_ldisc_size=0;
int b_ldisc_quotenext=0;
int b_ldisc_special;
Config cfg;
int b_resizing;
int b_need_backend_resize;
int b_has_focus;
int b_was_zoomed=0;
int n_prev_rows;
int n_prev_cols;
int need_backend_resize;
HBITMAP caretbm;
HINSTANCE cur_instance;

LRESULT CALLBACK WndProc_main(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam){
    SECURITY_ATTRIBUTES sa;
    HANDLE filemap;
    Config * tp_config;
    char b[2048];
    STARTUPINFO info_startup;
    PROCESS_INFORMATION info_process;
    HMENU hmenu_temp;
    int tn_state;
    int n;
    char ts_temp_buf[100];
    unsigned char buf[2];
    unsigned char c;
    HIMC hIMC;
    char * ts_buff;
    HIMC hImc;
    LOGFONT lf;
    int tn_count;
    int i;
    unsigned char pc_out[100];
    unsigned char * p;
    BYTE keystate[256];
    int tn_left_alt_down;
    int tn_capsOn;
    int tn_key_down;
    int tn_shift;
    int tn_ret;
    int tn_code;
    int tb_app_cursor_keys;
    PAINTSTRUCT ps;
    HDC hdc;
    int tn_chr_l;
    int tn_chr_r;
    int tn_chr_t;
    int tn_chr_b;
    COLORREF color_defbg_temp;
    HBRUSH t_new_brush;
    HGDIOBJ t_old_brush;
    HPEN t_new_pen;
    HGDIOBJ t_old_pen;
    time_t time_now;
    int tn_w;
    int tn_h;
    int tn_col;
    int tn_row;
    LPRECT tp_rct;
    int tn_ew;
    int tn_eh;
    int tn_fw;
    int tn_fh;
    struct timer_node * p_timer_node_temp;

    switch(msg){
        case WM_COMMAND:
        case WM_SYSCOMMAND:
            switch(wparam & ~0xf){
                case IDM_ABOUT:
                    showabout(hwnd);
                    return 0;
                case IDM_CLRSB:
                    term_clrsb(term);
                    return 0;
                case IDM_COPYALL:
                    term_copyall(term);
                    return 0;
                case IDM_DUPSESS:
                    sa.nLength=sizeof(sa);
                    sa.lpSecurityDescriptor=NULL;
                    sa.bInheritHandle=TRUE;
                    filemap=NULL;
                    filemap=CreateFileMapping(INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE, 0, sizeof(Config), NULL);
                    if(filemap && filemap != INVALID_HANDLE_VALUE){
                        tp_config=(Config *) MapViewOfFile(filemap, FILE_MAP_WRITE, 0, 0, sizeof(Config));
                        if(tp_config){
                            *tp_config=cfg;
                            UnmapViewOfFile(tp_config);
                        }
                        sprintf(ts_temp_buf, "putty &%p", filemap);
                        GetModuleFileName(NULL, b, sizeof(b) - 1);
                        info_startup.cb=sizeof(info_startup);
                        info_startup.lpReserved=NULL;
                        info_startup.lpDesktop=NULL;
                        info_startup.lpTitle=NULL;
                        info_startup.dwFlags=0;
                        info_startup.cbReserved2=0;
                        info_startup.lpReserved2=NULL;
                        CreateProcess(b, ts_temp_buf, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, &info_startup, &info_process);
                        CloseHandle(filemap);
                    }
                    else{
                        break;
                    }
                    return 0;
                case IDM_FULLSCREEN:
                    if(is_full_screen()){
                        clear_full_screen();
                    }
                    else{
                        make_full_screen();
                    }
                    return 0;
                case IDM_HELP:
                    launch_help(hwnd, NULL);
                    return 0;
                case IDM_NEWSESS:
                    GetModuleFileName(NULL, b, sizeof(b) - 1);
                    info_startup.cb=sizeof(info_startup);
                    info_startup.lpReserved=NULL;
                    info_startup.lpDesktop=NULL;
                    info_startup.lpTitle=NULL;
                    info_startup.dwFlags=0;
                    info_startup.cbReserved2=0;
                    info_startup.lpReserved2=NULL;
                    CreateProcess(b, NULL, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &info_startup, &info_process);
                    return 0;
                case IDM_PASTE:
                    request_paste(NULL);
                    return 0;
                case IDM_RESET:
                    term_pwron(term, TRUE);
                    return 0;
                case IDM_RESTART:
                    if(!back){
                        logevent(NULL, "----- Session restarted -----");
                        term_pwron(term, FALSE);
                        start_backend();
                    }
                    return 0;
                case IDM_SHOWLOG:
                    showeventlog(hwnd);
                    return 0;
                case IDM_TRAYCLOSE:
                    SendMessage(hwnd, WM_CLOSE, (WPARAM)NULL, (LPARAM)NULL);
                    return 0;
                case IDM_TRAYRESTORE:
                    ShowWindow(hwnd, SW_RESTORE);
                    SetForegroundWindow(hwnd);
                    windowMinimized=FALSE;
                    if(cfg.tray != TRAY_ALWAYS){
                        taskbar_addicon(cfg.win_name_always ? window_name : icon_name, FALSE);
                    }
                    return 0;
                case IDM_VISIBLE:
                    hmenu_temp=GetSystemMenu(hwnd, FALSE);
                    if(hmenu_temp){
                        tn_state=GetMenuState(hmenu_temp, (UINT)IDM_VISIBLE, MF_BYCOMMAND);
                        if(!(tn_state & MF_CHECKED)){
                            CheckMenuItem(hmenu_temp, (UINT)IDM_VISIBLE, MF_BYCOMMAND|MF_CHECKED);
                            SetWindowPos(hwnd, (HWND)-1, 0, 0, 0, 0, SWP_NOMOVE |SWP_NOSIZE);
                        }
                        else{
                            CheckMenuItem(hmenu_temp, (UINT)IDM_VISIBLE, MF_BYCOMMAND|MF_UNCHECKED);
                            SetWindowPos(hwnd, (HWND)-2, 0, 0, 0, 0, SWP_NOMOVE |SWP_NOSIZE);
                        }
                    }
                    return 0;
                case SC_KEYMENU:
                    show_mouseptr(1);
                    if(lparam == 0){
                        PostMessage(hwnd, WM_CHAR, ' ', 0);
                    }
                    break;
                case SC_MOUSEMENU:
                    show_mouseptr(1);
                    break;
                default:
                    if(wparam >= IDM_SAVED_MIN && wparam < IDM_SAVED_MAX){
                        n=((wparam - IDM_SAVED_MIN) / IDM_SAVED_STEP) + 1;
                        if(n < sesslist.nsessions){
                            sprintf(ts_temp_buf, "putty @%s", sesslist.sessions[n]);
                            GetModuleFileName(NULL, b, sizeof(b) - 1);
                            info_startup.cb=sizeof(info_startup);
                            info_startup.lpReserved=NULL;
                            info_startup.lpDesktop=NULL;
                            info_startup.lpTitle=NULL;
                            info_startup.dwFlags=0;
                            info_startup.cbReserved2=0;
                            info_startup.lpReserved2=NULL;
                            CreateProcess(b, ts_temp_buf, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &info_startup, &info_process);
                        }
                        else{
                            break;
                        }
                    }
                    if(wparam >= IDM_SPECIAL_MIN && wparam <= IDM_SPECIAL_MAX){
                        n=(wparam - IDM_SPECIAL_MIN) / 0x10;
                        if(n >= n_specials){
                            break;
                        }
                        if(back){
                            back->special(backhandle, specials[n].code);
                        }
                        net_pending_errors();
                    }
            }
            break;
        case WM_CHAR:
            term_seen_key_event(term);
            linedisc_send_codepage(CP_ACP, (unsigned char *)&wparam, 1);
            break;
        case WM_CLOSE:
            show_mouseptr(1);
            snprintf(ts_temp_buf, 100, "%s Exit Confirmation", appname);
            if(!cfg.warn_on_close || session_closed){
                DestroyWindow(hwnd);
            }
            else if(MessageBox(hwnd, "Are you sure you want to close this session?", ts_temp_buf, MB_ICONWARNING | MB_OKCANCEL | MB_DEFBUTTON1) == IDOK){
                DestroyWindow(hwnd);
            }
            return 0;
            break;
        case WM_DESTROY:
            show_mouseptr(1);
            PostQuitMessage(0);
            return 0;
            break;
        case WM_DESTROYCLIPBOARD:
            if(!n_ignore_clip){
                term_deselect(term);
            }
            n_ignore_clip=0;
            return 0;
            break;
        case WM_ENTERSIZEMOVE:
            EnableSizeTip(1);
            b_resizing=TRUE;
            b_need_backend_resize=FALSE;
            break;
        case WM_EXITSIZEMOVE:
            EnableSizeTip(0);
            b_resizing=FALSE;
            if(b_need_backend_resize){
                printf("resize: %d x %d\n", cfg.width, cfg.height);
                n_rows=cfg.height;
                n_cols=cfg.width;
                term_size(term, n_rows, n_cols);
                if(back){
                    back->size(backhandle, n_cols, n_rows);
                }
                InvalidateRect(hwnd, NULL, TRUE);
            }
            break;
        case WM_IME_CHAR:
            term_seen_key_event(term);
            if(wparam & 0xFF00){
                buf[1] = wparam;
                buf[0] = wparam >> 8;
                linedisc_send_codepage(kbd_codepage, buf, 2);
            }
            else{
                c=(unsigned char) wparam;
                linedisc_send_codepage(kbd_codepage, &c, 1);
            }
            return 0;
            break;
        case WM_IME_COMPOSITION:
            if((lparam & GCS_RESULTSTR) == 0){
                break;
            }
            hIMC=ImmGetContext(hwnd);
            n=ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, NULL, 0);
            if(n>0){
                ts_buff=(char*)malloc(n*sizeof(char));
                ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, ts_buff, n);
                term_seen_key_event(term);
                for(i=0;i<n;i+=2){
                    linedisc_send_unicode((unsigned short *)(ts_buff+i), 1);
                }
                free(ts_buff);
            }
            ImmReleaseContext(hwnd, hIMC);
            return 1;
            break;
        case WM_IME_STARTCOMPOSITION:
            hImc=ImmGetContext(hwnd);
            GetObject(fonts[FONT_NORMAL], sizeof(LOGFONT), &lf);
            ImmSetCompositionFont(hImc, &lf);
            ImmReleaseContext(hwnd, hImc);
            break;
            break;
        case WM_INITMENUPOPUP:
            if((HMENU)wparam == menu_savedsess){
                if(sesslist.buffer){
                    sfree(sesslist.buffer);
                    sfree(sesslist.sessions);
                    sesslist.buffer=NULL;
                    sesslist.sessions=NULL;
                }
                else{
                    get_sesslist(&sesslist, TRUE, cfg.session_storagetype);
                }
                while(DeleteMenu(menu_savedsess, 0, MF_BYPOSITION));
                tn_count=sesslist.nsessions;
                if(tn_count>1){
                    if(tn_count>IDM_SAVED_COUNT+1){
                        tn_count=IDM_SAVED_COUNT+1;
                    }
                    for(i=1;i<tn_count;i++){
                        AppendMenu(menu_savedsess, MF_ENABLED, IDM_SAVED_MIN+(i-1)*IDM_SAVED_STEP, sesslist.sessions[i]);
                    }
                }
                else{
                    AppendMenu(menu_savedsess, MF_GRAYED, IDM_SAVED_MIN, "(No sessions)");
                }
                return 0;
            }
            break;
            break;
        case WM_INPUTLANGCHANGE:
            set_input_locale((HKL)lparam);
            sys_cursor_update();
            break;
            break;
        case WM_KEYDOWN:
            p=pc_out;
            tn_left_alt_down=0;
            tn_capsOn=0;
            tn_key_down=((HIWORD(lparam)&KF_UP)==0);
            tn_shift=0;
            tn_ret=GetKeyboardState(keystate);
            if(!tn_ret){
                memset(keystate, 0, sizeof(keystate));
            }
            else{
                if((HIWORD(lparam)&KF_ALTDOWN) && ((keystate[VK_RMENU]&0x80)==0)){
                    tn_left_alt_down=1;
                }
                if((keystate[VK_SHIFT]&0x80)){
                    tn_shift=0x1;
                }
                if((keystate[VK_CONTROL]&0x80)){
                    tn_shift|=0x2;
                }
                if(wparam == VK_MENU && (HIWORD(lparam)&KF_EXTENDED)){
                    keystate[VK_RMENU] = keystate[VK_MENU];
                }
                if(cfg.xlat_capslockcyr && keystate[VK_CAPITAL] != 0){
                    tn_capsOn=!tn_left_alt_down;
                    keystate[VK_CAPITAL] = 0;
                }
                if(tn_left_alt_down && (keystate[VK_CONTROL]&0x80)){
                    if(cfg.ctrlaltkeys){
                        keystate[VK_MENU] = 0;
                    }
                    else{
                        keystate[VK_RMENU] = 0x80;
                        tn_left_alt_down=0;
                    }
                }
            }
            if(tn_key_down){
                if((HIWORD(lparam)&KF_REPEAT)){
                    return 0;
                }
            }
            tn_code=0;
            if((tn_shift&2) == 0){
                tn_code=0;
                switch(wparam){
                    case VK_HOME:
                        tn_code=1;
                        break;
                    case VK_INSERT:
                        tn_code=2;
                        break;
                    case VK_DELETE:
                        tn_code=3;
                        break;
                    case VK_END:
                        tn_code=4;
                        break;
                    case VK_PRIOR:
                        tn_code=5;
                        break;
                    case VK_NEXT:
                        tn_code=6;
                        break;
                }
                if(tn_code>0){
                    p += sprintf((char *) p, "\x1B[%d~", tn_code);
                    goto finish_return;
                }
            }
            switch(wparam){
                case VK_UP:
                    tn_code='A';
                    break;
                case VK_DOWN:
                    tn_code='B';
                    break;
                case VK_RIGHT:
                    tn_code='C';
                    break;
                case VK_LEFT:
                    tn_code='D';
                    break;
                case VK_CLEAR:
                    tn_code='G';
                    break;
            }
            if(tn_code){
                tb_app_cursor_keys=term_app_cursor_keys(term);
                if(tn_shift==2){
                    tb_app_cursor_keys=!tb_app_cursor_keys;
                }
                if(!tb_app_cursor_keys){
                    p+=sprintf(p, "\x1B[%c", tn_code);
                }
                else{
                    p+=sprintf(p, "\x1BO%c", tn_code);
                }
                goto finish_return;
            }
            tn_code=0;
            switch(wparam){
                case VK_F1:
                    tn_code=1;
                    break;
                case VK_F2:
                    tn_code=2;
                    break;
                case VK_F3:
                    tn_code=3;
                    break;
                case VK_F4:
                    tn_code=4;
                    break;
                case VK_F5:
                    tn_code=5;
                    break;
                case VK_F6:
                    tn_code=6;
                    break;
                case VK_F7:
                    tn_code=7;
                    break;
                case VK_F8:
                    tn_code=8;
                    break;
                case VK_F9:
                    tn_code=9;
                    break;
                case VK_F10:
                    tn_code=10;
                    break;
                case VK_F11:
                    tn_code=11;
                    break;
                case VK_F12:
                    tn_code=12;
                    break;
            }
            if(tn_code>0){
                tn_code+=10;
                if(tn_code>=16){
                    tn_code++;
                }
                if(tn_code>=22){
                    tn_code++;
                }
                if((keystate[VK_SHIFT]&0x80)){
                    tn_code+=12;
                    if(tn_code>=27){
                        tn_code++;
                    }
                }
                p += sprintf((char *) p, "\x1B[%d~", tn_code);
                goto finish_return;
            }
            break;
            finish_return:
                tn_ret=p-pc_out;
                if(tn_ret!=0){
                    term_nopaste(term);
                    term_seen_key_event(term);
                    linedisc_send(pc_out, tn_ret);
                    show_mouseptr(0);
                }
                return 0;
            break;
        case WM_KILLFOCUS:
            show_mouseptr(1);
            b_has_focus=0;
            DestroyCaret();
            caret_x=caret_y = -1;
            term_update(term);
            break;
        case WM_MOVE:
            sys_cursor_update();
            break;
        case WM_NETEVENT:
            if(b_pending_netevent){
                enact_pending_netevent();
            }
            b_pending_netevent=1;
            n_pend_netevent_wparam=wparam;
            n_pend_netevent_lparam=lparam;
            if(WSAGETSELECTEVENT(lparam) != FD_READ){
                enact_pending_netevent();
            }
            net_pending_errors();
            return 0;
            break;
        case WM_NOTIFY_PUTTYTRAY:
            if((UINT)wparam == 1983){
                if((UINT)lparam == WM_LBUTTONDBLCLK || (cfg.tray_restore == TRUE && (UINT)lparam == WM_LBUTTONUP)){
                    if(cfg.tray != TRAY_ALWAYS){
                        taskbar_addicon(cfg.win_name_always ? window_name : icon_name, FALSE);
                    }
                    Sleep(100);
                    if(windowMinimized){
                        ShowWindow(hwnd, SW_RESTORE);
                        SetForegroundWindow(hwnd);
                        windowMinimized=FALSE;
                    }
                    else{
                        ShowWindow(hwnd, SW_MINIMIZE);
                        windowMinimized=TRUE;
                    }
                }
                else if((UINT)lparam == WM_RBUTTONUP){
                    POINT cursorpos;
                    SetForegroundWindow(hwnd);
                    show_mouseptr(1);
                    GetCursorPos(&cursorpos);
                    TrackPopupMenu(popup_menus[MENU_CTX], TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON, cursorpos.x, cursorpos.y, 0, hwnd, NULL);
                    PostMessage(hwnd, WM_NULL, 0, 0);
                }
            }
            break;
            break;
        case WM_PAINT:
            HideCaret(hwnd);
            hdc=BeginPaint(hwnd, &ps);
            tn_chr_l=(ps.rcPaint.left-offset_width)/font_width;
            tn_chr_r=(ps.rcPaint.right-offset_width-1)/font_width;
            tn_chr_t=(ps.rcPaint.top-offset_height)/font_height;
            tn_chr_b=(ps.rcPaint.bottom-offset_height-1)/font_height;
            term_invalidate_rect(term, tn_chr_l, tn_chr_t, tn_chr_r, tn_chr_b);
            if(ps.fErase || ps.rcPaint.left  < offset_width  || ps.rcPaint.top   < offset_height || ps.rcPaint.right >= offset_width + font_width*n_cols || ps.rcPaint.bottom>= offset_height + font_height*n_rows){
                color_defbg_temp=colours[ATTR_DEFBG>>ATTR_BGSHIFT];
                t_new_brush=CreateSolidBrush(color_defbg_temp);
                t_old_brush=SelectObject(hdc, t_new_brush);
                t_new_pen=CreatePen(PS_SOLID, 0, color_defbg_temp);
                t_old_pen=SelectObject(hdc, t_new_pen);
                IntersectClipRect(hdc, ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right, ps.rcPaint.bottom);
                ExcludeClipRect(hdc, offset_width, offset_height, offset_width+font_width*n_cols, offset_height+font_height*n_rows);
                Rectangle(hdc, ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right, ps.rcPaint.bottom);
                SelectObject(hdc, t_old_brush);
                DeleteObject(t_new_brush);
                SelectObject(hdc, t_old_pen);
                DeleteObject(t_new_pen);
            }
            SelectObject(hdc, GetStockObject(SYSTEM_FONT));
            SelectObject(hdc, GetStockObject(WHITE_PEN));
            EndPaint(hwnd, &ps);
            ShowCaret(hwnd);
            return 0;
            break;
        case WM_POWERBROADCAST:
            if(cfg.wakeup_reconnect){
                switch(wparam){
                    case PBT_APMRESUMESUSPEND:
                    case PBT_APMRESUMEAUTOMATIC:
                    case PBT_APMRESUMECRITICAL:
                    case PBT_APMQUERYSUSPENDFAILED:
                        if(session_closed && !back){
                            time_now=time(NULL);
                            if(time_last_reconnect && (time_now - time_last_reconnect) < 5){
                                Sleep(1000);
                            }
                            time_last_reconnect=time_now;
                            logevent(NULL, "Woken up from suspend, reconnecting...");
                            term_pwron(term, FALSE);
                            start_backend();
                        }
                        break;
                    case PBT_APMSUSPEND:
                        if(!session_closed && back){
                            logevent(NULL, "Suspend detected, disconnecting cleanly...");
                            close_session();
                        }
                        break;
                }
            }
            break;
            break;
        case WM_SETFOCUS:
            b_has_focus=1;
            CreateCaret(hwnd, caretbm, font_width, font_height);
            ShowCaret(hwnd);
            flash_window(0);
            term_update(term);
            compose_state=0;
            break;
        case WM_SIZE:
            if(wparam == SIZE_MINIMIZED){
                if(cfg.tray == TRAY_NORMAL || cfg.tray == TRAY_START || is_ctrl_pressed()){
                    taskbar_addicon(cfg.win_name_always ? window_name : icon_name, TRUE);
                    ShowWindow(hwnd, SW_HIDE);
                }
            }
            if(wparam == SIZE_RESTORED || wparam == SIZE_MAXIMIZED){
                SetWindowText(hwnd, window_name);
            }
            else if(wparam == SIZE_MINIMIZED){
                SetWindowText(hwnd, cfg.win_name_always ? window_name : icon_name);
            }
            if(wparam == SIZE_MINIMIZED){
                windowMinimized=TRUE;
            }
            else{
                windowMinimized=FALSE;
            }
            if(cfg.resize_action == RESIZE_DISABLED){
                reset_window(-1);
            }
            else{
                tn_w=LOWORD(lparam);
                tn_h=HIWORD(lparam);
                tn_w-=cfg.window_border*2;
                tn_h-=cfg.window_border*2;
                tn_col=(tn_w + font_width / 2) / font_width;
                if(tn_col<1){
                    tn_col=1;
                }
                tn_row=(tn_h + font_height / 2) / font_height;
                if(tn_row<1){
                    tn_row=1;
                }
                if(wparam == SIZE_MAXIMIZED && !b_was_zoomed){
                    b_was_zoomed=1;
                    n_prev_rows=n_rows;
                    n_prev_cols=n_cols;
                    if(cfg.resize_action == RESIZE_TERM){
                        n_rows=tn_row;
                        n_cols=tn_col;
                        term_size(term, n_rows, n_cols);
                        if(back){
                            back->size(backhandle, n_cols, n_rows);
                        }
                    }
                    reset_window(0);
                }
                else if(wparam == SIZE_RESTORED && b_was_zoomed){
                    b_was_zoomed=0;
                    if(cfg.resize_action == RESIZE_TERM){
                        n_rows=tn_row;
                        n_cols=tn_col;
                        term_size(term, n_rows, n_cols);
                        if(back){
                            back->size(backhandle, n_cols, n_rows);
                        }
                        reset_window(2);
                    }
                    else if(cfg.resize_action != RESIZE_FONT){
                        reset_window(2);
                    }
                    else{
                        reset_window(0);
                    }
                }
                else if(wparam == SIZE_MINIMIZED){
                }
                else if(cfg.resize_action == RESIZE_TERM || (cfg.resize_action == RESIZE_EITHER && !is_alt_pressed())){
                    if(b_resizing){
                        b_need_backend_resize=TRUE;
                        cfg.height=tn_row;
                        cfg.width=tn_col;
                    }
                    else{
                        n_rows=tn_row;
                        n_cols=tn_col;
                        term_size(term, n_rows, n_cols);
                        if(back){
                            back->size(backhandle, n_cols, n_rows);
                        }
                    }
                }
                else{
                    reset_window(0);
                }
            }
            sys_cursor_update();
            break;
        case WM_SIZING:
            tp_rct=(LPRECT) lparam;
            if(cfg.resize_action == RESIZE_TERM || (cfg.resize_action == RESIZE_EITHER && !is_alt_pressed())){
                if(!need_backend_resize && cfg.resize_action == RESIZE_EITHER && (cfg.height != n_rows || cfg.width != n_cols )){
                    cfg.height=n_rows;
                    cfg.width=n_cols;
                    InvalidateRect(hwnd, NULL, TRUE);
                    need_backend_resize=TRUE;
                }
                tn_w=tp_rct->right - tp_rct->left - extra_width;
                tn_h=tp_rct->bottom - tp_rct->top - extra_height;
                tn_col=(tn_w + font_width / 2) / font_width;
                if(tn_col<1){
                    tn_col=1;
                }
                tn_row=(tn_h + font_height / 2) / font_height;
                if(tn_row<1){
                    tn_row=1;
                }
                UpdateSizeTip(hwnd, tn_row, tn_col);
                if(wparam==WMSZ_LEFT||wparam==WMSZ_BOTTOMLEFT||wparam == WMSZ_TOPLEFT){
                    tp_rct->left += tn_w-tn_col*font_width;
                }
                else{
                    tp_rct->right -= tn_w-tn_col*font_width;
                }
                if(wparam==WMSZ_TOP||wparam==WMSZ_TOPRIGHT||wparam == WMSZ_TOPLEFT){
                    tp_rct->top += tn_h-tn_row*font_height;
                }
                else{
                    tp_rct->bottom -= tn_h-tn_row*font_height;
                }
                if(tn_w-tn_col*font_width || tn_h-tn_row*font_height){
                    return 1;
                }
                else{
                    return 0;
                }
            }
            else{
                tn_ew=extra_width + (cfg.window_border - offset_width) * 2;
                tn_eh=extra_height + (cfg.window_border - offset_height) * 2;
                tn_w=tp_rct->right - tp_rct->left - tn_ew;
                tn_h=tp_rct->bottom - tp_rct->top - tn_eh;
                tn_fw=(tn_w + n_cols/2)/n_cols;
                tn_fh=(tn_h + n_rows/2)/n_rows;
                if(wparam==WMSZ_LEFT||wparam==WMSZ_BOTTOMLEFT||wparam == WMSZ_TOPLEFT){
                    tp_rct->left += tn_w-tn_fw*n_cols;
                }
                else{
                    tp_rct->right -= tn_w-tn_fw*n_cols;
                }
                if(wparam==WMSZ_TOP||wparam==WMSZ_TOPRIGHT||wparam == WMSZ_TOPLEFT){
                    tp_rct->top += tn_h-tn_fh*n_rows;
                }
                else{
                    tp_rct->bottom -= tn_h-tn_fh*n_rows;
                }
                if(tn_w-tn_fw*n_cols || tn_h-tn_fh*n_rows){
                    return 1;
                }
                else{
                    return 0;
                }
            }
            break;
        case WM_SYSCOLORCHANGE:
            if(cfg.system_colour){
                if(cfg.system_colour){
                    colours[COLOR_DEFFG]=GetSysColor(COLOR_WINDOWTEXT);
                    colours[COLOR_DEFFG_BOLD]=GetSysColor(COLOR_WINDOWTEXT);
                    colours[COLOR_DEFBG]=GetSysColor(COLOR_WINDOW);
                    colours[COLOR_DEFBG_BOLD]=GetSysColor(COLOR_WINDOW);
                    colours[COLOR_CURFG]=GetSysColor(COLOR_HIGHLIGHTTEXT);
                    colours[COLOR_CURBG]=GetSysColor(COLOR_HIGHLIGHT);
                }
                term_invalidate(term);
            }
            break;
        case WM_SYSKEYDOWN:
            if(((HIWORD(lparam)&KF_ALTDOWN)==0)){
                break;
            }
            if(wparam == VK_RETURN && cfg.fullscreenonaltenter && (cfg.resize_action != RESIZE_DISABLED)){
                if(((HIWORD(lparam)&KF_REPEAT)==0)){
                    if(is_full_screen()){
                        clear_full_screen();
                    }
                    else{
                        make_full_screen();
                    }
                }
                return 0;
            }
            if(wparam == VK_MENU && !cfg.alt_only){
                return 0;
            }
            break;
        case WM_TIMER:
            if((UINT_PTR)wparam == TIMING_TIMER_ID){
                KillTimer(hwnd, TIMING_TIMER_ID);
                time_now=GETTICKCOUNT();
                while(timer_list.head){
                    p_timer_node_temp=timer_list.head;
                    if(p_timer_node_temp->time_val<=time_now){
                        p_timer_node_temp->fn(p_timer_node_temp->ctx, p_timer_node_temp->time_val);
                    }
                    else{
                        break;
                    }
                    timer_list.head=p_timer_node_temp->next;
                    timer_list.n--;
                    free(p_timer_node_temp);
                }
                if(!timer_list.head){
                    timer_list.tail=NULL;
                }
                if(timer_list.head){
                    timer_change_notify(timer_list.head->time_val);
                }
            }
            return 0;
            break;
        case WM_VSCROLL:
            switch(LOWORD(wparam)){
                case SB_BOTTOM:
                    term_scroll(term, -1, 0);
                    break;
                case SB_TOP:
                    term_scroll(term, +1, 0);
                    break;
                case SB_LINEDOWN:
                    term_scroll(term, 0, +1);
                    break;
                case SB_LINEUP:
                    term_scroll(term, 0, -1);
                    break;
                case SB_PAGEDOWN:
                    term_scroll(term, 0, +n_rows / 2);
                    break;
                case SB_PAGEUP:
                    term_scroll(term, 0, -n_rows / 2);
                    break;
                case SB_THUMBPOSITION:
                case SB_THUMBTRACK:
                    term_scroll(term, 1, HIWORD(wparam));
                    break;
            }
            break;
        default:
            break;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

long schedule_timer(int tn_ticks, timer_fn_t fn, void * ctx){
    long time_now;
    long time_when;
    struct timer_node * p_timer_node_temp;
    struct timer_node * p_timer_node_temp2;
    struct timer_node ** pp_timer_node_temp;
    int tn_flag;

    time_now=GETTICKCOUNT();
    time_when=tn_ticks + time_now;
    if(tn_ticks<=0){
        fn(ctx, time_when);
    }
    else if(!timer_list.head || timer_list.head->time_val>time_when){
        p_timer_node_temp=(struct timer_node *)malloc(sizeof(struct timer_node));
        p_timer_node_temp->next=timer_list.head;
        timer_list.head=p_timer_node_temp;
        if(!timer_list.tail){
            timer_list.tail=p_timer_node_temp;
        }
        timer_list.n++;
        p_timer_node_temp->time_val=time_when;
        p_timer_node_temp->fn=fn;
        p_timer_node_temp->ctx=ctx;
        timer_change_notify(time_when);
    }
    else if(timer_list.tail->time_val<time_when){
        p_timer_node_temp=(struct timer_node *)malloc(sizeof(struct timer_node));
        p_timer_node_temp->next=NULL;
        if(!timer_list.head){
            timer_list.head=p_timer_node_temp;
        }
        else{
            timer_list.tail->next=p_timer_node_temp;
        }
        timer_list.tail=p_timer_node_temp;
        timer_list.n++;
        p_timer_node_temp->time_val=time_when;
        p_timer_node_temp->fn=fn;
        p_timer_node_temp->ctx=ctx;
    }
    else{
        p_timer_node_temp2=timer_list.head;
        pp_timer_node_temp=&timer_list.head;
        p_timer_node_temp=timer_list.head;
        while(p_timer_node_temp){
            tn_flag=1;
            if(p_timer_node_temp->time_val>time_when){
                p_timer_node_temp2=(struct timer_node *)malloc(sizeof(struct timer_node));
                p_timer_node_temp2->next=p_timer_node_temp;
                *pp_timer_node_temp=p_timer_node_temp2;
                timer_list.n++;
                p_timer_node_temp=p_timer_node_temp2;
                tn_flag=2;
                p_timer_node_temp->time_val=time_when;
                p_timer_node_temp->fn=fn;
                p_timer_node_temp->ctx=ctx;
                break;
            }
            if(tn_flag==0){
                free(p_timer_node_temp);
                p_timer_node_temp=*pp_timer_node_temp;
            }
            if(tn_flag==1){
                p_timer_node_temp2=p_timer_node_temp;
                pp_timer_node_temp=&(p_timer_node_temp->next);
                p_timer_node_temp=p_timer_node_temp->next;
            }
            else if(tn_flag==2){
                p_timer_node_temp2=p_timer_node_temp->next;
                pp_timer_node_temp=&(p_timer_node_temp2->next);
                p_timer_node_temp=p_timer_node_temp2->next;
            }
        }
    }
    return time_when;
}

void expire_timer_context(void * ctx){
    struct timer_node * p_timer_node_temp;
    struct timer_node * p_timer_node_temp2;
    struct timer_node ** pp_timer_node_temp;
    int tn_flag;

    p_timer_node_temp2=timer_list.head;
    pp_timer_node_temp=&timer_list.head;
    p_timer_node_temp=timer_list.head;
    while(p_timer_node_temp){
        tn_flag=1;
        if(p_timer_node_temp->ctx==ctx){
            *pp_timer_node_temp=p_timer_node_temp->next;
            timer_list.n--;
            if(*pp_timer_node_temp==NULL){
                timer_list.tail=p_timer_node_temp2;
            }
            tn_flag=0;
        }
        if(tn_flag==0){
            free(p_timer_node_temp);
            p_timer_node_temp=*pp_timer_node_temp;
        }
        if(tn_flag==1){
            p_timer_node_temp2=p_timer_node_temp;
            pp_timer_node_temp=&(p_timer_node_temp->next);
            p_timer_node_temp=p_timer_node_temp->next;
        }
        else if(tn_flag==2){
            p_timer_node_temp2=p_timer_node_temp->next;
            pp_timer_node_temp=&(p_timer_node_temp2->next);
            p_timer_node_temp=p_timer_node_temp2->next;
        }
    }
}

void timer_change_notify(long time_next){
    int tn_ticks;

    tn_ticks=(int)(time_next - GETTICKCOUNT());
    if(tn_ticks<=0){
        tn_ticks=1;
    }
    KillTimer(hwnd, TIMING_TIMER_ID);
    SetTimer(hwnd, TIMING_TIMER_ID, tn_ticks, NULL);
}

void start_backend(){
    char * ts_realhost;
    const char * ts_err;
    char ts_temp_buf[100];
    char ts_msg_buf[1024];
    char * ts_title;
    int i;

    back=backend_from_proto(cfg.protocol);
    assert(back);
    ts_err=back->init(NULL, &backhandle, &cfg, cfg.host, cfg.port, &ts_realhost, cfg.tcp_nodelay, cfg.tcp_keepalives);
    back->provide_logctx(backhandle,  logctx);
    if(ts_err){
        snprintf(ts_temp_buf, 100, "%s Error", appname);
        snprintf(ts_msg_buf, 1024, "Unable to open connection to\n%.800s\n%s", cfg_dest(&cfg), ts_err);
        MessageBox(NULL, ts_msg_buf, ts_temp_buf, MB_ICONERROR|MB_OK);
    }
    window_name=NULL;
    icon_name=NULL;
    if(*cfg.wintitle){
        ts_title=cfg.wintitle;
    }
    else{
        snprintf(ts_temp_buf, 100, "%s - %s", ts_realhost, appname);
        ts_title=ts_temp_buf;
    }
    set_title(NULL, ts_title);
    set_icon(NULL, ts_title);
    sfree(ts_realhost);
    if(back){
        back->provide_ldisc(backhandle, (void *)1);
    }
    must_close_session=FALSE;
    session_closed=FALSE;
    for(i=0;i<lenof(popup_menus);i++){
        DeleteMenu(popup_menus[i], IDM_RESTART, MF_BYCOMMAND);
    }
    back->size(backhandle, n_rows, n_cols);
}

void close_session(){
    char ts_temp_buf[100];
    int i;

    session_closed=TRUE;
    sprintf(ts_temp_buf, "%.70s (inactive)", appname);
    set_icon(NULL, ts_temp_buf);
    set_title(NULL, ts_temp_buf);
    if(back){
        back->provide_ldisc(backhandle, NULL);
    }
    if(s_ldisc_buf){
        sfree(s_ldisc_buf);
    }
    if(back){
        back->free(backhandle);
        backhandle=NULL;
        back=NULL;
        update_specials_menu(NULL);
    }
    must_close_session=FALSE;
    for(i=0;i<lenof(popup_menus);i++){
        DeleteMenu(popup_menus[i], IDM_RESTART, MF_BYCOMMAND);
        InsertMenu(popup_menus[i], IDM_DUPSESS, MF_BYCOMMAND | MF_ENABLED, IDM_RESTART, "&Restart Session");
    }
}

void cleanup_exit(int n_code){
    taskbar_addicon("", FALSE);
    DestroyIcon(puttyTray.hIcon);
    deinit_fonts();
    sk_cleanup();
    if(cfg.protocol == PROT_SSH){
        random_save_seed();
    }
    shutdown_help();
    CoUninitialize();
    exit(n_code);
}

char * do_select(SOCKET skt, int tb_startup){
    int tn_msg;
    int tn_events;

    if(!hwnd){
        return "do_select(): internal error (hwnd==NULL)";
    }
    if(tb_startup){
        tn_msg=WM_NETEVENT;
        tn_events=(FD_CONNECT | FD_READ | FD_WRITE | FD_OOB | FD_CLOSE | FD_ACCEPT);
    }
    else{
        tn_msg=0;
        tn_events=0;
    }
    if(p_WSAAsyncSelect(skt, hwnd, tn_msg, tn_events) == SOCKET_ERROR){
        if(p_WSAGetLastError()){
            return "Network is down";
        }
        else{
            return "WSAAsyncSelect(): unknown error";
        }
    }
    return NULL;
}

void connection_fatal(void * frontend, char * fmt, ...){
    va_list ap;
    char * ts_msg;
    char ts_temp_buf[100];

    va_start(ap, fmt);
    ts_msg=dupvprintf(fmt, ap);
    va_end(ap);
    snprintf(ts_temp_buf, 100, "%s Fatal Error", appname);
    MessageBox(hwnd, ts_msg, ts_temp_buf, MB_ICONERROR|MB_OK);
    sfree(ts_msg);
    if(cfg.close_on_exit == FORCE_ON){
        PostQuitMessage(1);
    }
    else{
        must_close_session=TRUE;
    }
}

void cmdline_error(char * fmt, ...){
    va_list ap;
    char * ts_msg;
    char ts_temp_buf[100];

    va_start(ap, fmt);
    ts_msg=dupvprintf(fmt, ap);
    va_end(ap);
    snprintf(ts_temp_buf, 100, "%s Command Line Error", appname);
    MessageBox(hwnd, ts_msg, ts_temp_buf, MB_ICONERROR|MB_OK);
    sfree(ts_msg);
    exit(1);
}

void fatalbox(char * fmt, ...){
    va_list ap;
    char * ts_msg;
    char ts_temp_buf[100];

    va_start(ap, fmt);
    ts_msg=dupvprintf(fmt, ap);
    va_end(ap);
    snprintf(ts_temp_buf, 100, "%s Fatal Error", appname);
    MessageBox(hwnd, ts_msg, ts_temp_buf, MB_ICONERROR|MB_OK);
    sfree(ts_msg);
    cleanup_exit(1);
}

void modalfatalbox(char * fmt, ...){
    va_list ap;
    char * ts_msg;
    char ts_temp_buf[100];

    va_start(ap, fmt);
    ts_msg=dupvprintf(fmt, ap);
    va_end(ap);
    snprintf(ts_temp_buf, 100, "%s Fatal Error", appname);
    MessageBox(hwnd, ts_msg, ts_temp_buf, MB_SYSTEMMODAL|MB_ICONERROR|MB_OK);
    sfree(ts_msg);
    cleanup_exit(1);
}

void notify_remote_exit(void * frontend){
    int tn_exitcode;

    if(!session_closed){
        tn_exitcode=back->exitcode(backhandle);
        if(tn_exitcode>=0){
            if(cfg.close_on_exit == FORCE_ON || (cfg.close_on_exit == AUTO && tn_exitcode != INT_MAX)){
                PostQuitMessage(0);
            }
            else{
                must_close_session=TRUE;
                session_closed=TRUE;
                if(tn_exitcode != INT_MAX){
                    MessageBox(hwnd, "Connection closed by remote host", appname, MB_OK | MB_ICONINFORMATION);
                }
            }
        }
    }
}

extern int select_result(WPARAM, LPARAM);
void enact_pending_netevent(){
    static int b_reentering= 0;

    if(b_reentering){
        return;
    }
    b_pending_netevent=FALSE;
    b_reentering=1;
    select_result(n_pend_netevent_wparam, n_pend_netevent_lparam);
    b_reentering=0;
}

int taskbar_addicon(char * ts_tip, int tb_show){
    int tn_ret;

    if(tb_show){
        if(ts_tip){
            strncpy(puttyTray.szTip, ts_tip, sizeof(puttyTray.szTip));
        }
        else{
            puttyTray.szTip[0] = (TCHAR)'\0';
        }
        if(!puttyTrayVisible){
            tray_updatemenu(TRUE);
            tn_ret=Shell_NotifyIcon(NIM_ADD, &puttyTray);
            puttyTrayVisible=TRUE;
            return tn_ret;
        }
        else{
            tn_ret=Shell_NotifyIcon(NIM_MODIFY, &puttyTray);
            return tn_ret;
        }
    }
    else{
        if(puttyTrayVisible){
            tray_updatemenu(FALSE);
            tn_ret=Shell_NotifyIcon(NIM_DELETE, &puttyTray);
            puttyTrayVisible=FALSE;
            return tn_ret;
        }
    }
    return 1;
}

void tray_updatemenu(int disableMenuItems){
    MENUITEMINFO info_menuitem;

    memset(&info_menuitem, 0, sizeof(MENUITEMINFO));
    info_menuitem.cbSize=sizeof(MENUITEMINFO);
    if(disableMenuItems){
        DeleteMenu(popup_menus[MENU_CTX], IDM_TRAYSEP, MF_BYCOMMAND);
        DeleteMenu(popup_menus[MENU_CTX], IDM_TRAYRESTORE, MF_BYCOMMAND);
        DeleteMenu(popup_menus[MENU_CTX], IDM_TRAYCLOSE, MF_BYCOMMAND);
        InsertMenu(popup_menus[MENU_CTX], -1, MF_BYPOSITION | MF_SEPARATOR, IDM_TRAYSEP, 0);
        InsertMenu(popup_menus[MENU_CTX], -1, MF_BYPOSITION | MF_STRING, IDM_TRAYRESTORE, "&Restore Window");
        InsertMenu(popup_menus[MENU_CTX], -1, MF_BYPOSITION | MF_STRING, IDM_TRAYCLOSE, "&Exit");
        info_menuitem.hbmpItem=HBMMENU_POPUP_RESTORE;
        SetMenuItemInfo(popup_menus[MENU_CTX], IDM_TRAYRESTORE, FALSE, &info_menuitem);
        info_menuitem.fMask=MIIM_BITMAP;
        info_menuitem.hbmpItem=HBMMENU_POPUP_CLOSE;
        SetMenuItemInfo(popup_menus[MENU_CTX], IDM_TRAYCLOSE, FALSE, &info_menuitem);
        info_menuitem.fMask=MIIM_STATE;
        info_menuitem.fState=MFS_GRAYED;
    }
    else{
        DeleteMenu(popup_menus[MENU_CTX], IDM_TRAYSEP, MF_BYCOMMAND);
        DeleteMenu(popup_menus[MENU_CTX], IDM_TRAYRESTORE, MF_BYCOMMAND);
        DeleteMenu(popup_menus[MENU_CTX], IDM_TRAYCLOSE, MF_BYCOMMAND);
        info_menuitem.fMask=MIIM_STATE;
        info_menuitem.fState=MFS_ENABLED;
    }
    SetMenuItemInfo(popup_menus[MENU_CTX], (UINT)menu_specials, FALSE, &info_menuitem);
    SetMenuItemInfo(popup_menus[MENU_CTX], IDM_PASTE, FALSE, &info_menuitem);
    SetMenuItemInfo(popup_menus[MENU_CTX], IDM_FULLSCREEN, FALSE, &info_menuitem);
    SetMenuItemInfo(popup_menus[MENU_CTX], IDM_RESET, FALSE, &info_menuitem);
    SetMenuItemInfo(popup_menus[MENU_CTX], IDM_CLRSB, FALSE, &info_menuitem);
    SetMenuItemInfo(popup_menus[MENU_CTX], IDM_COPYALL, FALSE, &info_menuitem);
}

void init_fonts(int n_tw, int n_th){
    int i;
    HDC hdc;
    int fw_dontcare;
    int fw_bold;
    TEXTMETRIC tm;
    COLORREF t_color;
    HDC und_dc;
    HBITMAP und_bm;
    HBITMAP und_oldbm;
    int tb_gotit;
    int fontsize[3];

    for(i=0;i<FONT_MAXNO;i++){
        fonts[i]=NULL;
    }
    hdc=GetDC(hwnd);
    if(n_th){
        font_height=n_th;
    }
    else{
        font_height=cfg.font.height;
        if(font_height > 0){
            font_height=-MulDiv(font_height, GetDeviceCaps(hdc, LOGPIXELSY), 72);
        }
    }
    font_width=n_tw;
    bold_mode=cfg.bold_colour ? BOLD_COLOURS : BOLD_FONT;
    und_mode=UND_FONT;
    if(cfg.font.isbold){
        fw_dontcare=FW_BOLD;
        fw_bold=FW_HEAVY;
    }
    else{
        fw_dontcare=FW_DONTCARE;
        fw_bold=FW_BOLD;
    }
    fonts[FONT_NORMAL] = CreateFont (font_height, font_width, 0, 0, fw_dontcare, FALSE, FALSE, FALSE, cfg.font.charset, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, FONT_QUALITY(cfg.font_quality), FIXED_PITCH | FF_DONTCARE, cfg.font.name);
    fonts[FONT_BOLD] = CreateFont (font_height, font_width, 0, 0, fw_bold, FALSE, FALSE, FALSE, cfg.font.charset, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, FONT_QUALITY(cfg.font_quality), FIXED_PITCH | FF_DONTCARE, cfg.font.name);
    fonts[FONT_UNDERLINE] = CreateFont (font_height, font_width, 0, 0, fw_dontcare, FALSE, TRUE, FALSE, cfg.font.charset, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, FONT_QUALITY(cfg.font_quality), FIXED_PITCH | FF_DONTCARE, cfg.font.name);
    fontflag[0] = fontflag[1] = fontflag[2] = 1;
    SelectObject(hdc, fonts[FONT_NORMAL]);
    GetTextMetrics(hdc, &tm);
    if(!(tm.tmPitchAndFamily & TMPF_FIXED_PITCH)){
        font_varpitch=FALSE;
        font_dualwidth=(tm.tmAveCharWidth != tm.tmMaxCharWidth);
    }
    else{
        font_varpitch=TRUE;
        font_dualwidth=TRUE;
    }
    font_height=tm.tmHeight;
    font_width=get_font_width(hdc, &tm);
    font_descent=tm.tmAscent + 1;
    if(font_descent>= font_height){
        font_descent=font_height - 1;
    }
    update_ucs_charset(tm.tmCharSet);
    und_dc=CreateCompatibleDC(hdc);
    und_bm=CreateCompatibleBitmap(hdc, font_width, font_height);
    und_oldbm=SelectObject(und_dc, und_bm);
    SelectObject(und_dc, fonts[FONT_UNDERLINE]);
    SetTextAlign(und_dc, TA_TOP | TA_LEFT | TA_NOUPDATECP);
    SetTextColor(und_dc, RGB(255, 255, 255));
    SetBkColor(und_dc, RGB(0, 0, 0));
    SetBkMode(und_dc, OPAQUE);
    ExtTextOut(und_dc, 0, 0, ETO_OPAQUE, NULL, " ", 1, NULL);
    tb_gotit=FALSE;
    for(i=0;i<font_height;i++){
        t_color=GetPixel(und_dc, font_width / 2, i);
        if(t_color != RGB(0, 0, 0)){
            tb_gotit=TRUE;
        }
    }
    SelectObject(und_dc, und_oldbm);
    DeleteObject(und_bm);
    DeleteDC(und_dc);
    if(!tb_gotit){
        und_mode=UND_LINE;
        DeleteObject(fonts[FONT_UNDERLINE]);
        fonts[FONT_UNDERLINE] = 0;
    }
    for(i=0;i<3;i++){
        if(fonts[i]){
            if(SelectObject(hdc, fonts[i]) && GetTextMetrics(hdc, &tm)){
                fontsize[i] = get_font_width(hdc, &tm) + 256 * tm.tmHeight;
            }
            else{
                fontsize[i] = -i;
            }
        }
        else{
            fontsize[i] = -i;
        }
    }
    if(fontsize[FONT_UNDERLINE] != fontsize[FONT_NORMAL]){
        und_mode=UND_LINE;
        DeleteObject(fonts[FONT_UNDERLINE]);
        fonts[FONT_UNDERLINE] = 0;
    }
    if(bold_mode == BOLD_FONT && fontsize[FONT_BOLD] != fontsize[FONT_NORMAL]){
        bold_mode=BOLD_SHADOW;
        DeleteObject(fonts[FONT_BOLD]);
        fonts[FONT_BOLD] = 0;
    }
    ReleaseDC(hwnd, hdc);
    update_ucs_line_codepage(cfg.line_codepage);
    update_ucs_linedraw(cfg.vtmode);
}

void deinit_fonts(){
    int i;

    for(i=0;i<FONT_MAXNO;i++){
        if(fonts[i]){
            DeleteObject(fonts[i]);
        }
        fonts[i] = 0;
        fontflag[i] = 0;
    }
}

void another_font(int n){
}

int get_font_width(HDC hdc, TEXTMETRIC * tm){
    ABCFLOAT abc_widths['9'-'0' + 1];
    int i;
    int tn_width;

    if(!(tm->tmPitchAndFamily & TMPF_FIXED_PITCH)){
        return tm->tmAveCharWidth;
    }
    else{
        font_varpitch=TRUE;
        font_dualwidth=TRUE;
        if(GetCharABCWidthsFloat(hdc, '0', '9', abc_widths)){
            for(i=0;i<lenof(abc_widths);i++){
                tn_width=(int)(0.5 + abc_widths[i].abcfA +abc_widths[i].abcfB + abc_widths[i].abcfC);
                if(tn_width>0){
                    return tn_width;
                }
            }
        }
        else{
            return tm->tmMaxCharWidth;
        }
    }
    return 0;
}

void general_textout(HDC hdc, int x, int y, CONST RECT * lprc, unsigned short * lpString, int n_len, CONST INT * lpDx_buf){
    int tn_opt;
    int tn_x;
    int tn_x_next;
    int tb_rtl;
    int j;
    int tn_temp;
    char * ts_idx;
    int i;
    GCP_RESULTSW gcpr;
    char * ts_classbuffer;

    tn_opt=ETO_CLIPPED;
    if(n_len>0){
        tn_x=x;
        tn_x_next=x;
        for(i=0;i<n_len;){
            tb_rtl=is_rtl(lpString[i]);
            tn_x_next += lpDx_buf[i];
            for(j=i+1;j<n_len;j++){
                if(tb_rtl != is_rtl(lpString[j])){
                    break;
                }
                else{
                    tn_x_next += lpDx_buf[j];
                }
            }
            if(tb_rtl){
                tn_temp=(j-i)*2+2;
                ts_idx=(char*)malloc(tn_temp*sizeof(char));
                for(i=0;i<tn_temp;i++){
                    ts_idx[i]=0;
                }
                memset(&gcpr, 0, sizeof(gcpr));
                ts_classbuffer=(char*)malloc((j-i)*sizeof(char));
                for(i=0;i<(j-i);i++){
                    ts_classbuffer[i]=GCPCLASS_NEUTRAL;
                }
                gcpr.lStructSize=sizeof(gcpr);
                gcpr.lpGlyphs=(void *)ts_idx;
                gcpr.lpClass=(void *)ts_classbuffer;
                gcpr.nGlyphs=j-i;
                GetCharacterPlacementW(hdc, lpString, j-i, 0, &gcpr, FLI_MASK | GCP_CLASSIN | GCP_DIACRITIC);
                free(ts_classbuffer);
                ExtTextOut(hdc, tn_x, y, ETO_GLYPH_INDEX|tn_opt, lprc, ts_idx, j-i, lpDx_buf+i);
                free(ts_idx);
            }
            else{
                ExtTextOutW(hdc, tn_x, y, tn_opt, lprc, lpString+i, j-i, font_varpitch ? NULL : lpDx_buf+i);
            }
            i=j;
            tn_x=tn_x_next;
        }
    }
}

void do_text_internal(int x, int y, wchar_t * text, int n_len, unsigned long n_attr, int n_lattr, int b_opaque){
    HDC hdc=cur_hdc;
    int tb_do_underline;
    int tn_fnt_width;
    int tn_x;
    int tn_y;
    int tn_char_width;
    int tn_yoffset=0;
    int tb_force_underline=0;
    int tn_font;
    int i;
    int tn_fg;
    int tn_bg;
    int tn_temp;
    COLORREF color_fg;
    COLORREF color_bg;
    RECT rct_line;
    HBRUSH tn_bg_brush;
    int tn_xoffset;
    int tn_maxlen;
    int tn_remain;
    int tn_len;
    static int * lpDx_buf=NULL;
    static int    lpDx_len=0;
    static wchar_t * uni_buf=NULL;
    static int    uni_len=0;
    int tn_i_buff;
    int tn_i_text;
    char dbcstext[2];
    static char * direct_buf=NULL;
    static int    direct_len=0;
    static WCHAR * w_buf=NULL;
    static int    w_len=0;
    int tn_dec;
    HPEN t_new_pen;
    HGDIOBJ t_old_pen;

    if(n_lattr != LATTR_NORM && x*2 >= n_cols){
        return;
    }
    if(n_attr & ATTR_UNDER && n_lattr != LATTR_TOP){
        tb_do_underline=1;
    }
    else{
        tb_do_underline=0;
    }
    if((n_attr & TATTR_ACTCURS) && cfg.cursor_type == 0){
        n_attr &= ~(ATTR_REVERSE|ATTR_BLINK|ATTR_COLOURS);
        if(bold_mode == BOLD_COLOURS){
            n_attr &= ~ATTR_BOLD;
        }
        n_attr |= (260 << ATTR_FGSHIFT) | (261 << ATTR_BGSHIFT);
    }
    if(n_lattr == LATTR_NORM){
        tn_fnt_width=font_width;
    }
    else{
        tn_fnt_width=font_width*2;
    }
    tn_x=x*tn_fnt_width+offset_width;
    tn_y=y*font_height+offset_height;
    if(n_attr & ATTR_WIDE){
        tn_char_width=tn_fnt_width*2;
    }
    else{
        tn_char_width=tn_fnt_width;
    }
    if(text[0] >= 0x23BA && text[0] <= 0x23BD){
        switch((unsigned char)text[0]){
            case 0xBA:
                tn_yoffset=-2 * font_height / 5;
                break;
            case 0xBB:
                tn_yoffset=-1 * font_height / 5;
                break;
            case 0xBC:
                tn_yoffset=font_height / 5;
                break;
            case 0xBD:
                tn_yoffset=2 * font_height / 5;
                break;
        }
        if(n_lattr == LATTR_TOP || n_lattr == LATTR_BOT){
            tn_yoffset *= 2;
        }
        text[0] = unitab_draw['q'];
        if(tb_do_underline){
            tb_force_underline=1;
        }
    }
    tn_font=0;
    if(cfg.vtmode == VT_POORMAN && n_lattr != LATTR_NORM){
        n_lattr=LATTR_WIDE;
    }
    else{
        if(n_lattr == LATTR_NORM){
            tn_font=0;
        }
        else if(n_lattr==LATTR_WIDE){
            tn_font=FONT_WIDE;
        }
        else{
            tn_font=FONT_WIDE+FONT_HIGH;
        }
    }
    if(n_attr & ATTR_NARROW){
        tn_font |= FONT_NARROW;
    }
    if((text[0] & CSET_MASK) == CSET_OEMCP){
        tn_font |= FONT_OEM;
    }
    if(bold_mode == BOLD_FONT && (n_attr & ATTR_BOLD)){
        tn_font |= FONT_BOLD;
    }
    if(tb_do_underline && und_mode == UND_FONT && !tb_force_underline){
        tn_font |= FONT_UNDERLINE;
    }
    another_font(tn_font);
    if(!fonts[tn_font]){
        tn_font &= ~(FONT_BOLD | FONT_UNDERLINE);
        another_font(tn_font);
    }
    if(!fonts[tn_font]){
        tn_font=FONT_NORMAL;
    }
    if(tn_font & FONT_UNDERLINE){
        tb_do_underline=0;
    }
    if(DIRECT_CHAR(text[0])){
        for(i=0;i<n_len;i++){
            text[i] = 0xFFFD;
        }
    }
    tn_fg=n_attr & ATTR_FGMASK;
    tn_bg=(n_attr & ATTR_BGMASK) >> ATTR_BGSHIFT;
    if(n_attr & ATTR_REVERSE){
        tn_temp=tn_fg;
        tn_fg=tn_bg;
        tn_bg=tn_temp;
    }
    if(bold_mode == BOLD_COLOURS){
        if(n_attr & ATTR_BOLD){
            if(tn_fg < 16){
                tn_fg |= 8;
            }
            else if(tn_fg >= 256){
                tn_fg |= 1;
            }
        }
        if(n_attr & ATTR_BLINK){
            if(tn_bg < 16){
                tn_bg |= 8;
            }
            else if(tn_bg >= 256){
                tn_bg |= 1;
            }
        }
    }
    color_fg=colours[tn_fg];
    color_bg=colours[tn_bg];
    SelectObject(hdc, fonts[tn_font]);
    SetTextColor(hdc, color_fg);
    SetBkColor(hdc, color_bg);
    rct_line.left=tn_x;
    rct_line.top=tn_y;
    rct_line.right=tn_x + tn_char_width * n_len;
    rct_line.bottom=tn_y + font_height;
    if(rct_line.right > font_width*n_cols+offset_width){
        rct_line.right=font_width*n_cols+offset_width;
    }
    if(b_opaque){
        tn_bg_brush=CreateSolidBrush(color_bg);
        FillRect(hdc, &rct_line, tn_bg_brush);
        DeleteObject(tn_bg_brush);
    }
    SetBkMode(hdc, TRANSPARENT);
    tn_xoffset=0;
    if(font_varpitch){
        tn_xoffset=tn_char_width / 2;
        SetTextAlign(hdc, TA_TOP | TA_CENTER | TA_NOUPDATECP);
        tn_maxlen=1;
    }
    else{
        tn_xoffset=0;
        SetTextAlign(hdc, TA_TOP | TA_LEFT | TA_NOUPDATECP);
        tn_maxlen=n_len;
    }
    tn_x+=tn_xoffset;
    tn_y+=tn_yoffset;
    if(n_lattr==LATTR_BOT){
        tn_y-=font_height;
    }
    tn_remain=n_len;
    while(tn_remain>0){
        tn_len=(tn_maxlen < tn_remain ? tn_maxlen : tn_remain);
        if(lpDx_len<tn_len){
            sfree(lpDx_buf);
            lpDx_len=tn_len*9/8+16;
            lpDx_buf=snewn(lpDx_len, int);
        }
        for(i=0;i<tn_len;i++){
            lpDx_buf[i] = tn_char_width;
        }
        if(font_isdbcs && (text[0] & CSET_MASK) == CSET_ACP){
            if(uni_len<tn_len){
                sfree(uni_buf);
                uni_len=tn_len*9/8+16;
                uni_buf=snewn(uni_len, wchar_t);
            }
            tn_i_buff=0;
            tn_i_text=0;
            while(tn_i_text<tn_len){
                uni_buf[tn_i_buff] = 0xFFFD;
                if(IsDBCSLeadByteEx(font_codepage, (BYTE) text[tn_i_text])){
                    dbcstext[0] = text[tn_i_text] & 0xFF;
                    tn_i_text++;
                    dbcstext[1] = text[tn_i_text] & 0xFF;
                    tn_i_text++;
                    lpDx_buf[tn_i_buff] += tn_char_width;
                    MultiByteToWideChar(font_codepage, MB_USEGLYPHCHARS, dbcstext, 2, uni_buf+tn_i_buff, 1);
                }
                else{
                    dbcstext[0] = text[tn_i_text] & 0xFF;
                    tn_i_text++;
                    MultiByteToWideChar(font_codepage, MB_USEGLYPHCHARS, dbcstext, 1, uni_buf+tn_i_buff, 1);
                }
                tn_i_buff++;
            }
            ExtTextOutW(hdc, tn_x, tn_y, ETO_CLIPPED, &rct_line, uni_buf, tn_i_buff, lpDx_buf);
            if(bold_mode == BOLD_SHADOW && (n_attr & ATTR_BOLD)){
                SetBkMode(hdc, TRANSPARENT);
                ExtTextOutW(hdc, tn_x - 1, tn_y, ETO_CLIPPED, &rct_line, uni_buf, tn_i_buff, lpDx_buf);
            }
            lpDx_buf[0] = -1;
        }
        else if(DIRECT_FONT(text[0])){
            if(direct_len<tn_len){
                sfree(direct_buf);
                direct_len=tn_len*9/8+16;
                direct_buf=snewn(direct_len, char);
            }
            for(i=0;i<tn_len;i++){
                direct_buf[i] = text[i] & 0xFF;
            }
            direct_buf[tn_len]='\0';
            ExtTextOut(hdc, tn_x, tn_y, ETO_CLIPPED, &rct_line, direct_buf, tn_len, lpDx_buf);
            if(bold_mode == BOLD_SHADOW && (n_attr & ATTR_BOLD)){
                SetBkMode(hdc, TRANSPARENT);
                ExtTextOut(hdc, tn_x - 1, tn_y, ETO_CLIPPED, &rct_line, direct_buf, tn_len, lpDx_buf);
            }
        }
        else{
            if(w_len<tn_len){
                sfree(w_buf);
                w_len=tn_len*9/8+16;
                w_buf=snewn(w_len, WCHAR);
            }
            for(i=0;i<tn_len;i++){
                w_buf[i] = text[i];
            }
            general_textout(hdc, tn_x, tn_y, &rct_line, w_buf, tn_len, lpDx_buf);
            if(bold_mode == BOLD_SHADOW && (n_attr & ATTR_BOLD)){
                SetBkMode(hdc, TRANSPARENT);
                ExtTextOutW(hdc, tn_x-1, tn_y, ETO_CLIPPED, &rct_line, w_buf, tn_len, lpDx_buf);
            }
        }
        tn_x += tn_char_width * tn_len;
            text += tn_len;
            tn_remain -= tn_len;
    }
    if(tb_do_underline){
        tn_dec=font_descent;
        if(n_lattr == LATTR_BOT){
            tn_dec=tn_dec * 2 - font_height;
        }
        t_new_pen=CreatePen(PS_SOLID, 0, color_fg);
        t_old_pen=SelectObject(hdc, t_new_pen);
        MoveToEx(hdc, rct_line.left, rct_line.top + tn_dec, NULL);
        LineTo(hdc, rct_line.right, rct_line.top + tn_dec);
        SelectObject(hdc, t_old_pen);
        DeleteObject(t_new_pen);
    }
}

void do_text(int x, int y, wchar_t * text, int n_len, unsigned long n_attr, int n_lattr){
    HDC hdc=cur_hdc;
    int i;

    if(n_attr & TATTR_COMBINING){
        n_attr&=~TATTR_COMBINING;
        do_text_internal(x, y, text, 1, n_attr, n_lattr, 1);
        text++;
        for(i=1;i<n_len;i++){
            do_text_internal(x, y, text, 1, n_attr, n_lattr, 0);
            text++;
        }
    }
    else{
        do_text_internal(x, y, text, n_len, n_attr, n_lattr, 1);
    }
}

void do_cursor(int x, int y, wchar_t * text, int n_len, unsigned long n_attr, int n_lattr){
    HDC hdc=cur_hdc;
    int tn_cursor_type;
    int tn_fnt_width;
    int tn_x;
    int tn_y;
    int tn_char_width;
    COLORREF color_cursor;
    POINT pts[5];
    HPEN t_new_pen;
    HGDIOBJ t_old_pen;

    tn_cursor_type=cfg.cursor_type;
    if(n_attr & TATTR_ACTCURS && tn_cursor_type == 0){
        if(*text != UCSWIDE){
            do_text(x, y, text, n_len, n_attr, n_lattr);
            return;
        }
        else{
            tn_cursor_type=2;
            n_attr |= TATTR_RIGHTCURS;
        }
    }
    if(n_lattr == LATTR_NORM){
        tn_fnt_width=font_width;
    }
    else{
        tn_fnt_width=font_width*2;
    }
    tn_x=x*tn_fnt_width+offset_width;
    tn_y=y*font_height+offset_height;
    if(n_attr & ATTR_WIDE){
        tn_char_width=tn_fnt_width*2;
    }
    else{
        tn_char_width=tn_fnt_width;
    }
    color_cursor=colours[261];
    if(tn_cursor_type == 0){
        pts[0].x=pts[1].x = tn_x;
        pts[2].x=pts[3].x = tn_x + tn_char_width - 1;
        pts[0].y=pts[3].y = tn_y;
        pts[1].y=pts[2].y = tn_y + font_height - 1;
        pts[4]=pts[0];
        t_new_pen=CreatePen(PS_SOLID, 0, color_cursor);
        t_old_pen=SelectObject(hdc, t_new_pen);
        Polyline(hdc, pts, 5);
        SelectObject(hdc, t_old_pen);
        DeleteObject(t_new_pen);
    }
    else{
        if(n_attr & TATTR_ACTCURS){
            t_new_pen=CreatePen(PS_SOLID, 0, color_cursor);
            t_old_pen=SelectObject(hdc, t_new_pen);
        }
        else{
            t_new_pen=CreatePen(PS_DOT, 0, color_cursor);
            t_old_pen=SelectObject(hdc, t_new_pen);
        }
        if(tn_cursor_type == 1){
            t_new_pen=CreatePen(PS_SOLID, 0, color_cursor);
            t_old_pen=SelectObject(hdc, t_new_pen);
            MoveToEx(hdc, tn_x, tn_y+font_descent, NULL);
            LineTo(hdc, tn_x + tn_char_width, tn_y+font_descent);
            SelectObject(hdc, t_old_pen);
            DeleteObject(t_new_pen);
        }
        else if(n_attr & TATTR_RIGHTCURS){
            t_new_pen=CreatePen(PS_SOLID, 0, color_cursor);
            t_old_pen=SelectObject(hdc, t_new_pen);
            MoveToEx(hdc, tn_x+tn_char_width-1, tn_y, NULL);
            LineTo(hdc, tn_x + tn_char_width-1, tn_y+font_height);
            SelectObject(hdc, t_old_pen);
            DeleteObject(t_new_pen);
        }
        else{
            t_new_pen=CreatePen(PS_SOLID, 0, color_cursor);
            t_old_pen=SelectObject(hdc, t_new_pen);
            MoveToEx(hdc, tn_x+0, tn_y, NULL);
            LineTo(hdc, tn_x + 0, tn_y+font_height);
            SelectObject(hdc, t_old_pen);
            DeleteObject(t_new_pen);
        }
        SelectObject(hdc, t_old_pen);
        DeleteObject(t_new_pen);
        SelectObject(hdc, t_old_pen);
        DeleteObject(t_new_pen);
    }
}

int char_width(int tn_char){
    HDC hdc=cur_hdc;
    int tn_cw;

    if(!font_dualwidth){
        return 1;
    }
    if(!cur_hdc){
        return 1;
    }
    tn_cw=0;
    switch(tn_char & CSET_MASK){
        case CSET_ASCII:
            tn_char=unitab_line[tn_char & 0xFF];
            break;
        case CSET_LINEDRW:
            tn_char=unitab_draw[tn_char & 0xFF];
            break;
        case CSET_SCOACS:
            tn_char=unitab_scoacs[tn_char & 0xFF];
            break;
    }
    if(DIRECT_FONT(tn_char)){
        if(font_isdbcs){
            return 1;
        }
        if((tn_char&~CSET_MASK) >= ' ' && (tn_char&~CSET_MASK)<= '~'){
            return 1;
        }
        if((tn_char & CSET_MASK) == CSET_ACP){
            SelectObject(hdc, fonts[FONT_NORMAL]);
        }
        else if((tn_char & CSET_MASK) == CSET_OEMCP){
            another_font(FONT_OEM);
            if (!fonts[FONT_OEM]) return 0;
            SelectObject(hdc, fonts[FONT_OEM]);
        }
        else{
            return 0;
        }
        if(GetCharWidth32(hdc, tn_char&~CSET_MASK, tn_char&~CSET_MASK, &tn_cw) != 1 && GetCharWidth(hdc, tn_char&~CSET_MASK, tn_char&~CSET_MASK, &tn_cw) != 1){
            return 0;
        }
    }
    else{
        if (tn_char >= ' ' && tn_char <= '~') return 1;
        SelectObject(hdc, fonts[FONT_NORMAL]);
        if(GetCharWidth32W(hdc, tn_char, tn_char, &tn_cw) == 1){
        }
        else if(GetCharWidthW(hdc, tn_char, tn_char, &tn_cw) == 1){
        }
        else{
            return 0;
        }
    }
    tn_cw += font_width / 2 -1;
    tn_cw /= font_width;
    return tn_cw;
}

void write_clip(void * frontend, wchar_t * ws_data, int* pn_attr, int n_len, int b_must_deselect){
    HGLOBAL clipdata1=NULL;
    void * lock1;
    int n_len2;
    HGLOBAL clipdata2=NULL;
    void * lock2;
    HGLOBAL clipdata3= NULL;

    clipdata1=GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE,n_len*sizeof(wchar_t));
    if(!clipdata1){
        goto clip_cleanup;
    }
    lock1=GlobalLock(clipdata1);
    if(!lock1){
        goto clip_cleanup;
    }
    memcpy(lock1, ws_data, n_len * sizeof(wchar_t));
    GlobalUnlock(clipdata1);
    n_len2=WideCharToMultiByte(CP_ACP, 0, ws_data, n_len, 0, 0, NULL, NULL);
    clipdata2=GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE,n_len2);
    if(!clipdata2){
        goto clip_cleanup;
    }
    lock2=GlobalLock(clipdata2);
    if(!lock2){
        goto clip_cleanup;
    }
    WideCharToMultiByte(CP_ACP, 0, ws_data, n_len, lock2, n_len2, NULL, NULL);
    GlobalUnlock(clipdata2);
    if(!b_must_deselect){
        n_ignore_clip=1;
    }
    if(OpenClipboard(hwnd)){
        EmptyClipboard();
        SetClipboardData(CF_UNICODETEXT, clipdata1);
        SetClipboardData(CF_TEXT, clipdata2);
        if (clipdata3)
            SetClipboardData(RegisterClipboardFormat(CF_RTF), clipdata3);
        CloseClipboard();
    }
    else{
        return;
    }
    if(!b_must_deselect){
        n_ignore_clip=0;
    }
    clip_cleanup:
        if(clipdata1){
            GlobalFree(clipdata1);
        }
        if(clipdata2){
            GlobalFree(clipdata2);
        }
        if(clipdata3){
            GlobalFree(clipdata3);
        }
}

void write_aclip(void * frontend, char * s_data, int n_len, int b_must_deselect){
    HGLOBAL clipdata1=NULL;
    void * lock1;

    clipdata1=GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE,n_len+1);
    if(!clipdata1){
        goto clip_cleanup;
    }
    lock1=GlobalLock(clipdata1);
    if(!lock1){
        goto clip_cleanup;
    }
    memcpy(lock1, s_data, n_len);
    ((unsigned char *) lock1)[n_len] = 0;
    GlobalUnlock(clipdata1);
    if(!b_must_deselect){
        n_ignore_clip=1;
    }
    if(OpenClipboard(hwnd)){
        EmptyClipboard();
        SetClipboardData(CF_TEXT, clipdata1);
        CloseClipboard();
    }
    else{
        return;
    }
    if(!b_must_deselect){
        n_ignore_clip=0;
    }
    clip_cleanup:
        if(clipdata1){
            GlobalFree(clipdata1);
        }
}

void request_paste(void * frontend){
    DWORD in_threadid;

    CreateThread(NULL, 0, thread_read_clipboard, hwnd, 0, &in_threadid);
}

DWORD WINAPI thread_read_clipboard(void * param){
    HGLOBAL clipdata=NULL;
    wchar_t * ts_wbuf;
    int tn_wbuf_len;
    wchar_t * s1;
    wchar_t * s2;
    int i;

    if(OpenClipboard(NULL)){
        if((clipdata = GetClipboardData(CF_UNICODETEXT))){
            process_clipdata(clipdata, 1);
        }
        else if((clipdata = GetClipboardData(CF_TEXT))){
            process_clipdata(clipdata, 0);
        }
        if(clipboard_contents && clipboard_length>0){
            term_seen_key_event(term);
            ts_wbuf=(wchar_t*)malloc(clipboard_length*sizeof(wchar_t));
            tn_wbuf_len=0;
            s1=s2 = clipboard_contents;
            while(s1<clipboard_contents+clipboard_length){
                while(s1<clipboard_contents+clipboard_length && !((s1<=clipboard_contents+clipboard_length-n_sel_nl)&&(memcmp(s1,sel_nl,sizeof(sel_nl))==0))){
                    s1++;
                }
                for(i=0;i<s1-s2;i++){
                    ts_wbuf[tn_wbuf_len++] = s2[i];
                }
                if((s1<=clipboard_contents+clipboard_length-n_sel_nl)&&(memcmp(s1,sel_nl,sizeof(sel_nl))==0)){
                    ts_wbuf[tn_wbuf_len++] = '\r';
                    s1 += n_sel_nl;
                }
                s2=s1;
            }
            linedisc_send_unicode(ts_wbuf, tn_wbuf_len);
            free(ts_wbuf);
        }
        CloseClipboard();
    }
    return 0;
}

int process_clipdata(HGLOBAL clipdata, int b_unicode){
    wchar_t * p;
    wchar_t * p2;
    char * s;
    int n;

    if(clipboard_contents){
        sfree(clipboard_contents);
    }
    clipboard_contents=NULL;
    clipboard_length=0;
    if(b_unicode){
        p=GlobalLock(clipdata);
        if(p){
            for(p2=p; *p2; p2++);
            clipboard_length=p2 - p;
            clipboard_contents=snewn(clipboard_length + 1, wchar_t);
            memcpy(clipboard_contents, p, clipboard_length * sizeof(wchar_t));
            clipboard_contents[clipboard_length] = L'\0';
            return 1;
        }
    }
    else{
        s=GlobalLock(clipdata);
        if(s){
            n=MultiByteToWideChar(CP_ACP, 0, s, strlen(s) + 1, 0, 0);
            clipboard_contents=snewn(n, wchar_t);
            MultiByteToWideChar(CP_ACP, 0, s, strlen(s) + 1, clipboard_contents, n);
            clipboard_length=n - 1;
            clipboard_contents[clipboard_length] = L'\0';
            return 1;
        }
    }
    return 0;
}

void request_resize(void * frontend, int tn_col, int tn_row){
    static int n_state=1;
    static RECT rct_ss;
    HMONITOR h_monitor;
    MONITORINFO info_monitor;
    int tn_width;
    int tn_height;
    int tn_w;
    int tn_h;

    if(IsZoomed(hwnd) && cfg.resize_action==RESIZE_TERM){
        return;
    }
    if(cfg.resize_action == RESIZE_DISABLED){
        return;
    }
    if(tn_row == n_rows && tn_col == n_cols){
        return;
    }
    if(n_state!=1){
        h_monitor=MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        info_monitor.cbSize=sizeof(info_monitor);
        GetMonitorInfo(h_monitor, &info_monitor);
        rct_ss=info_monitor.rcMonitor;
        n_state=2;
        tn_width=(rct_ss.right-rct_ss.left-extra_width)/4;
        tn_height=(rct_ss.bottom-rct_ss.top-extra_height)/6;
        if(tn_col > tn_width || tn_row > tn_height){
            return;
        }
        if(tn_col < 15){
            tn_col=15;
        }
        if(tn_row < 1){
            tn_row=1;
        }
    }
    n_rows=tn_row;
    n_cols=tn_col;
    term_size(term, n_rows, n_cols);
    if(back){
        back->size(backhandle, n_cols, n_rows);
    }
    if(cfg.resize_action != RESIZE_FONT && !IsZoomed(hwnd)){
        tn_w=extra_width + font_width * tn_col;
        tn_h=extra_height + font_height * tn_row;
        SetWindowPos(hwnd, NULL, 0, 0, tn_w, tn_h, SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOZORDER);
    }
    else{
        reset_window(0);
    }
    InvalidateRect(hwnd, NULL, TRUE);
}

void reset_window(int n_state){
    RECT rct_c;
    RECT rct_w;
    int tn_w;
    int tn_h;
    static RECT rct_ss;
    HMONITOR h_monitor;
    MONITORINFO info_monitor;
    int tn_col;
    int tn_row;

    GetWindowRect(hwnd, &rct_w);
    GetClientRect(hwnd, &rct_c);
    tn_w=(rct_c.right-rct_c.left);
    tn_h=(rct_c.bottom-rct_c.top);
    if(tn_w == 0 || tn_h == 0){
        return;
    }
    if(cfg.resize_action == RESIZE_DISABLED){
        n_state=2;
    }
    if(n_state>1){
        deinit_fonts();
        init_fonts(0,0);
    }
    if(n_state==0 && (offset_width != (tn_w-font_width*n_cols)/2 || offset_height != (tn_h-font_height*n_rows)/2)){
        offset_width=(tn_w-font_width*n_cols)/2;
        offset_height=(tn_h-font_height*n_rows)/2;
        extra_width=(rct_w.right-rct_w.left)-(rct_c.right-rct_c.left) + offset_width*2;
        extra_height=(rct_w.bottom-rct_w.top)-(rct_c.bottom-rct_c.top) + offset_height*2;
        InvalidateRect(hwnd, NULL, TRUE);
    }
    if(IsZoomed(hwnd)){
        if(cfg.resize_action != RESIZE_TERM){
            if(font_width != (tn_w/n_cols) || font_height != (tn_h/n_rows)){
                deinit_fonts();
                init_fonts((tn_w/n_cols), (tn_h/n_rows));
                offset_width=(tn_w-font_width*n_cols)/2;
                offset_height=(tn_h-font_height*n_rows)/2;
                extra_width=(rct_w.right-rct_w.left)-(rct_c.right-rct_c.left) + offset_width*2;
                extra_height=(rct_w.bottom-rct_w.top)-(rct_c.bottom-rct_c.top) + offset_height*2;
                InvalidateRect(hwnd, NULL, TRUE);
            }
        }
        else{
            if(n_cols !=(tn_w/font_width) || n_rows!=(tn_h/font_height)){
                n_rows=(tn_h/font_height);
                n_cols=(tn_w/font_width);
                term_size(term, n_rows, n_cols);
                if(back){
                    back->size(backhandle, n_cols, n_rows);
                }
                offset_width=(tn_w-font_width*n_cols)/2;
                offset_height=(tn_h-font_height*n_rows)/2;
                extra_width=(rct_w.right-rct_w.left)-(rct_c.right-rct_c.left) + offset_width*2;
                extra_height=(rct_w.bottom-rct_w.top)-(rct_c.bottom-rct_c.top) + offset_height*2;
                InvalidateRect(hwnd, NULL, TRUE);
            }
        }
        return;
    }
    else if(n_state>0){
        offset_width=cfg.window_border;
        offset_height=cfg.window_border;
        extra_width=(rct_w.right-rct_w.left)-(rct_c.right-rct_c.left) + offset_width*2;
        extra_height=(rct_w.bottom-rct_w.top)-(rct_c.bottom-rct_c.top) + offset_height*2;
        if(tn_w != (font_width*n_cols+offset_width*2) || tn_h != (font_height*n_rows+offset_height*2)){
            SetWindowPos(hwnd, NULL, 0, 0, (font_width*n_cols+extra_width), (font_height*n_rows+extra_height), SWP_NOMOVE | SWP_NOZORDER);
        }
        InvalidateRect(hwnd, NULL, TRUE);
        return;
    }
    else if(n_state<0 && (cfg.resize_action == RESIZE_TERM || cfg.resize_action == RESIZE_EITHER)){
        offset_width=cfg.window_border;
        offset_height=cfg.window_border;
        extra_width=(rct_w.right-rct_w.left)-(rct_c.right-rct_c.left) + offset_width*2;
        extra_height=(rct_w.bottom-rct_w.top)-(rct_c.bottom-rct_c.top) + offset_height*2;
        if(tn_w != (font_width*n_cols+offset_width*2) || tn_h != (font_height*n_rows+offset_height*2)){
        }
            h_monitor=MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
            info_monitor.cbSize=sizeof(info_monitor);
            GetMonitorInfo(h_monitor, &info_monitor);
            rct_ss=info_monitor.rcMonitor;
            tn_col=(rct_ss.right-rct_ss.left-extra_width) / font_width;
            tn_row=(rct_ss.bottom-rct_ss.top-extra_height) / font_height;
            if(n_rows > tn_row || n_cols > tn_col){
                if(cfg.resize_action == RESIZE_EITHER){
                    if(n_cols > tn_col){
                        font_width=(rct_ss.right-rct_ss.left-extra_width)  / n_cols;
                    }
                    if(n_rows > tn_row){
                        font_height=(rct_ss.bottom-rct_ss.top-extra_height) / n_rows;
                    }
                    deinit_fonts();
                    init_fonts(font_width, font_height);
                    tn_col=(rct_ss.right-rct_ss.left-extra_width) / font_width;
                    tn_row=(rct_ss.bottom-rct_ss.top-extra_height) / font_height;
                }
                else{
                    tn_row=n_rows;
                    tn_col=n_cols;
                    n_rows=tn_row;
                    n_cols=tn_col;
                    term_size(term, n_rows, n_cols);
                    if(back){
                        back->size(backhandle, n_cols, n_rows);
                    }
                }
            }
            SetWindowPos(hwnd, NULL, 0, 0, (font_width*n_cols+extra_width), (font_height*n_rows+extra_height), SWP_NOMOVE | SWP_NOZORDER);
            InvalidateRect(hwnd, NULL, TRUE);
        return;
    }
    else if(font_width != ((tn_w-cfg.window_border*2)/n_cols) || font_height != ((tn_h-cfg.window_border*2)/n_rows)){
        if(font_width != (tn_w/n_cols) || font_height != (tn_h/n_rows)){
            deinit_fonts();
            init_fonts((tn_w/n_cols), (tn_h/n_rows));
            offset_width=(tn_w-font_width*n_cols)/2;
            offset_height=(tn_h-font_height*n_rows)/2;
            extra_width=(rct_w.right-rct_w.left)-(rct_c.right-rct_c.left) + offset_width*2;
            extra_height=(rct_w.bottom-rct_w.top)-(rct_c.bottom-rct_c.top) + offset_height*2;
            InvalidateRect(hwnd, NULL, TRUE);
        }
    }
}

void make_full_screen(){
    RECT rct_w;
    int tn_style;
    RECT rct_ss;
    HMONITOR h_monitor;
    MONITORINFO info_monitor;
    int i;

    if(is_full_screen()){
        return;
    }
    GetWindowRect(hwnd, &rct_w);
    n_win_x=rct_w.left;
    n_win_y=rct_w.top;
    n_win_w=rct_w.right-rct_w.left;
    n_win_h=rct_w.bottom-rct_w.top;
    printf("save win: +%d+%d-%dx%d\n", n_win_x, n_win_y, n_win_w, n_win_h);
    tn_style=GetWindowLongPtr(hwnd, GWL_STYLE);
    tn_style &= ~(WS_CAPTION | WS_BORDER | WS_THICKFRAME);
    if(cfg.scrollbar_in_fullscreen){
        tn_style |= WS_VSCROLL;
    }
    else{
        tn_style &= ~WS_VSCROLL;
    }
    SetWindowLongPtr(hwnd, GWL_STYLE, tn_style);
    h_monitor=MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    info_monitor.cbSize=sizeof(info_monitor);
    GetMonitorInfo(h_monitor, &info_monitor);
    rct_ss=info_monitor.rcMonitor;
    SetWindowPos(hwnd, HWND_TOP, rct_ss.left, rct_ss.top, (rct_ss.right-rct_ss.left), (rct_ss.bottom-rct_ss.top), SWP_FRAMECHANGED);
    reset_window(0);
    b_fullscreen=1;
    for(i=0;i<lenof(popup_menus);i++){
        CheckMenuItem(popup_menus[i], IDM_FULLSCREEN, MF_CHECKED);
    }
}

void clear_full_screen(){
    int tn_style;
    int tn_old;
    int i;

    tn_style=GetWindowLongPtr(hwnd, GWL_STYLE);
    tn_old=tn_style;
    tn_style |= WS_CAPTION | WS_BORDER;
    if(cfg.resize_action == RESIZE_DISABLED){
        tn_style &= ~WS_THICKFRAME;
    }
    else{
        tn_style |= WS_THICKFRAME;
    }
    if(cfg.scrollbar){
        tn_style |= WS_VSCROLL;
    }
    else{
        tn_style &= ~WS_VSCROLL;
    }
    if(tn_style != tn_old){
        SetWindowLongPtr(hwnd, GWL_STYLE, tn_style);
        printf("restore win: +%d+%d-%dx%d\n", n_win_x, n_win_y, n_win_w, n_win_h);
        SetWindowPos(hwnd, NULL, n_win_x, n_win_y, n_win_w, n_win_h, SWP_NOZORDER | SWP_FRAMECHANGED);
    }
    b_fullscreen=0;
    for(i=0;i<lenof(popup_menus);i++){
        CheckMenuItem(popup_menus[i], IDM_FULLSCREEN, MF_UNCHECKED);
    }
}

int is_full_screen(){
    return b_fullscreen;
}

int pt_on_topleft(POINT pt){
    HMONITOR h_monitor;
    MONITORINFO info_monitor;

    h_monitor=MonitorFromPoint(pt, MONITOR_DEFAULTTONULL);
    if(h_monitor != NULL){
        info_monitor.cbSize=sizeof(MONITORINFO);
        GetMonitorInfo(h_monitor, &info_monitor);
        if(info_monitor.rcMonitor.left == pt.x && info_monitor.rcMonitor.top == pt.y){
            return 1;
        }
    }
    return 0;
}

void do_beep(){
    int tn_mode;
    static long n_lastbeep=0;
    long n_beepdiff;
    char buf[sizeof(cfg.bell_wavefile.path) + 80];
    char ts_temp_buf[100];

    tn_mode=cfg.beep;
    if(tn_mode == BELL_DEFAULT){
        n_beepdiff=GetTickCount() - n_lastbeep;
        if(n_beepdiff >= 0 && n_beepdiff < 50){
            return;
        }
        MessageBeep(MB_OK);
        n_lastbeep=GetTickCount();
    }
    else if(tn_mode == BELL_PCSPEAKER){
        n_beepdiff=GetTickCount() - n_lastbeep;
        if(n_beepdiff >= 0 && n_beepdiff < 50){
            return;
        }
        if(osVersion.dwPlatformId == VER_PLATFORM_WIN32_NT){
            Beep(800, 100);
        }
        else{
            MessageBeep(-1);
        }
        n_lastbeep=GetTickCount();
    }
    else if(tn_mode == BELL_WAVEFILE){
        if(!PlaySound(cfg.bell_wavefile.path, NULL, SND_ASYNC | SND_FILENAME)){
            sprintf(buf, "Unable to play sound file\n%s\n" "Using default sound instead", cfg.bell_wavefile.path);
            snprintf(ts_temp_buf, 100, "%s Sound Error", appname);
            MessageBox(hwnd, buf, ts_temp_buf, MB_OK | MB_ICONEXCLAMATION);
            cfg.beep=BELL_DEFAULT;
        }
    }
    else{
        if(b_has_focus){
            flash_window(2);
        }
    }
}

void set_title(void * frontend, char * ts_title){
    sfree(window_name);
    window_name=snewn(1 + strlen(ts_title), char);
    strcpy(window_name, ts_title);
    if (cfg.win_name_always || !IsIconic(hwnd))
        SetWindowText(hwnd, ts_title);
    taskbar_addicon(cfg.win_name_always ? window_name : icon_name, puttyTrayVisible);
}

void set_icon(void * frontend, char * ts_title){
    sfree(icon_name);
    icon_name=snewn(1 + strlen(ts_title), char);
    strcpy(icon_name, ts_title);
    if (!cfg.win_name_always && IsIconic(hwnd))
        SetWindowText(hwnd, ts_title);
}

void front_set_scrollbar(int tn_total, int tn_pos, int tn_pagesize){
    SCROLLINFO si;

    if(is_full_screen() ? !cfg.scrollbar_in_fullscreen : !cfg.scrollbar){
        return;
    }
    si.cbSize=sizeof(si);
    si.fMask=SIF_ALL | SIF_DISABLENOSCROLL;
    si.nMin=0;
    si.nMax=tn_total - 1;
    si.nPage=tn_pagesize;
    si.nPos=tn_pos;
    if(hwnd){
        SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
    }
}

int paint_start(){
    if(hwnd){
        cur_hdc=GetDC(hwnd);
        return 1;
    }
    else{
        cur_hdc=0;
        return 0;
    }
}

void paint_finish(){
    if(cur_hdc){
        ReleaseDC(hwnd, cur_hdc);
    }
}

void on_timer_flash_window(void * ctx, long n_now){
    if(n_is_flashing && n_now - n_time_nextflash >= 0){
        flash_window(1);
    }
}

void flash_window(int mode){
    if(mode==0 || cfg.beep_ind==B_IND_DISABLED){
        if(n_is_flashing){
            FlashWindow(hwnd, FALSE);
            n_is_flashing=0;
            puttyTray.hIcon=puttyTrayFlashIcon;
            if (puttyTrayVisible) {
                puttyTrayFlash=FALSE;
                taskbar_addicon(cfg.win_name_always ? window_name : icon_name, TRUE);
            }
        }
    }
    else if(mode == 2){
        if(!n_is_flashing){
            n_is_flashing=1;
            puttyTrayFlashIcon=puttyTray.hIcon;
            FlashWindow(hwnd, TRUE);
            n_time_nextflash=schedule_timer(450, on_timer_flash_window, hwnd);
            if (puttyTrayVisible) {
                puttyTrayFlash=FALSE;
            }
        }
    }
    else if(mode == 1 && cfg.beep_ind == B_IND_FLASH){
        if(n_is_flashing){
            FlashWindow(hwnd, TRUE);
            n_time_nextflash=schedule_timer(450, on_timer_flash_window, hwnd);
            if (puttyTrayVisible) {
                if (!puttyTrayFlash) {
                    puttyTrayFlash=TRUE;
                    puttyTray.hIcon=NULL;
                    taskbar_addicon(cfg.win_name_always ? window_name : icon_name, TRUE);
                } else {
                    puttyTrayFlash=FALSE;
                    puttyTray.hIcon=puttyTrayFlashIcon;
                    taskbar_addicon(cfg.win_name_always ? window_name : icon_name, TRUE);
                }
            }
        }
    }
}

void sys_cursor(void * frontend, int x, int y){
    int tn_cx;
    int tn_cy;

    if(b_has_focus){
        return;
    }
    tn_cx=x*font_width+offset_width;
    tn_cy=y*font_height+offset_height;
    if(tn_cx!=caret_x || tn_cy!=caret_y){
        sys_cursor_update();
    }
}

void sys_cursor_update(){
    COMPOSITIONFORM cf;
    HIMC hIMC;

    if(b_has_focus){
        return;
    }
    if (caret_x < 0 || caret_y < 0)
        return;
    SetCaretPos(caret_x, caret_y);
    if(osVersion.dwPlatformId == VER_PLATFORM_WIN32s) return; /* 3.11 */;
    if(osVersion.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS && osVersion.dwMinorVersion == 0) return; /* 95 */;
    hIMC=ImmGetContext(hwnd);
    cf.dwStyle=CFS_POINT;
    cf.ptCurrentPos.x=caret_x;
    cf.ptCurrentPos.y=caret_y;
    ImmSetCompositionWindow(hIMC, &cf);
    ImmReleaseContext(hwnd, hIMC);
}

int MakeWindowTransparent(HWND hWnd, int n_alpha){
    static int b_transparency_initialized=0;
    static PSLWA fn_SetLayeredWindowAttributes=NULL;
    HMODULE hDLL;
    int tn_exstyle;

    if(!b_transparency_initialized){
        hDLL=LoadLibrary("user32");
        fn_SetLayeredWindowAttributes=(PSLWA) GetProcAddress(hDLL, "SetLayeredWindowAttributes");
        b_transparency_initialized=TRUE;
    }
    if(fn_SetLayeredWindowAttributes == NULL){
        return 0;
    }
    if(n_alpha<0){
        return 0;
    }
    else if(n_alpha>255){
        n_alpha=255;
    }
    tn_exstyle=GetWindowLong(hWnd, GWL_EXSTYLE);
    if(n_alpha < 255){
        SetLastError(0);
        SetWindowLong(hWnd, GWL_EXSTYLE, tn_exstyle | WS_EX_LAYERED);
        if(GetLastError()){
            return 0;
        }
        return fn_SetLayeredWindowAttributes(hWnd, RGB(255,255,255), n_alpha, LWA_ALPHA);
    }
    else{
        SetWindowLong(hWnd, GWL_EXSTYLE, tn_exstyle & ~WS_EX_LAYERED);
        return 1;
    }
}

char * get_ttymode(void * frontend, const char * s_mode){
    char * ts_val;

    ts_val=NULL;
    if(strcmp(s_mode, "ERASE")==0){
        ts_val=cfg.bksp_is_delete? "^?":"^H";
    }
    return dupstr(ts_val);
}

void set_iconic(void * frontend, int b_iconic){
    if(IsIconic(hwnd) && !b_iconic){
        ShowWindow(hwnd, SW_RESTORE);
        windowMinimized=FALSE;
    }
    else if(!IsIconic(hwnd) && b_iconic){
        ShowWindow(hwnd, SW_MINIMIZE);
        windowMinimized=TRUE;
    }
}

void move_window(void * frontend, int x, int y){
    if(cfg.resize_action == RESIZE_DISABLED || cfg.resize_action == RESIZE_FONT || IsZoomed(hwnd)){
        return;
    }
    else{
        SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }
}

void set_zorder(void * frontend, int b_top){
    if(cfg.alwaysontop){
        return;			       /* ignore */;
    }
    SetWindowPos(hwnd, b_top ? HWND_TOP : HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

void refresh_window(void * frontend){
    InvalidateRect(hwnd, NULL, TRUE);
}

void set_zoomed(void * frontend, int b_zoomed){
    if(IsZoomed(hwnd) && !b_zoomed){
        ShowWindow(hwnd, SW_RESTORE);
    }
    else if(!IsZoomed(hwnd) && b_zoomed){
        ShowWindow(hwnd, SW_MAXIMIZE);
    }
}

int is_iconic(void * frontend){
    return IsIconic(hwnd);
}

void get_window_pos(void * frontend, int* pn_x, int* pn_y){
    RECT r;

    GetWindowRect(hwnd, &r);
    *pn_x=r.left;
    *pn_y=r.top;
}

void get_window_pixels(void * frontend, int* pn_x, int* pn_y){
    RECT r;
    GetWindowRect(hwnd, &r);
    *pn_x=r.right - r.left;
    *pn_y=r.bottom - r.top;
}

char * get_window_title(void * frontend, int b_icon){
    if(b_icon){
        return icon_name;
    }
    else{
        return window_name;
    }
}

int from_backend(void * frontend, int is_stderr, const char * data, int len){
    return term_data(term, data, len);
}

int from_backend_untrusted(void * frontend, const char * data, int len){
    return term_data_untrusted(term, data, len);
}

void frontend_keypress(void * handle){
    return;
}

void update_specials_menu(void * frontend){
    HMENU saved_menu;
    HMENU new_menu;
    int tn_nesting;
    int i;

    if(back){
        specials=back->get_specials(backhandle);
    }
    else{
        specials=NULL;
    }
    if(specials){
        saved_menu=NULL;
        new_menu=CreatePopupMenu();
        tn_nesting=1;
        for(i=0;tn_nesting>0;i++){
            assert(IDM_SPECIAL_MIN+IDM_SPECIAL_STEP*i<IDM_SPECIAL_MAX);
            switch(specials[i].code){
                case TS_SEP:
                    AppendMenu(new_menu, MF_SEPARATOR, 0, 0);
                    break;
                case TS_SUBMENU:
                    assert(tn_nesting < 2);
                    tn_nesting++;
                    saved_menu=new_menu;
                    new_menu=CreatePopupMenu();
                    AppendMenu(saved_menu, MF_POPUP|MF_ENABLED, (UINT) new_menu, specials[i].name);
                    break;
                case TS_EXITMENU:
                    tn_nesting--;
                    if(tn_nesting){
                        new_menu=saved_menu;
                        saved_menu=NULL;
                    }
                    break;
                default:
                    AppendMenu(new_menu, MF_ENABLED, IDM_SPECIAL_MIN+i*IDM_SPECIAL_STEP, specials[i].name);
                    break;
            }
        }
        n_specials=i - 1;
    }
    else{
        new_menu=NULL;
        n_specials=0;
    }
    for(i=0;i<lenof(popup_menus);i++){
        if(menu_specials){
            DeleteMenu(popup_menus[i], (UINT)menu_specials, MF_BYCOMMAND);
            DeleteMenu(popup_menus[i], IDM_SPECIALSEP, MF_BYCOMMAND);
        }
        if(new_menu){
            InsertMenu(popup_menus[i], IDM_SHOWLOG, MF_BYCOMMAND | MF_POPUP | MF_ENABLED, (UINT) new_menu, "S&pecial Command");
            InsertMenu(popup_menus[i], IDM_SHOWLOG, MF_BYCOMMAND | MF_SEPARATOR, IDM_SPECIALSEP, 0);
        }
    }
    menu_specials=new_menu;
}

int is_alt_pressed(){
    unsigned char pc_keys[256];

    if(GetKeyboardState(pc_keys)){
        if(pc_keys[VK_MENU] & 0x80 || pc_keys[VK_RMENU] & 0x80){
            return 1;
        }
    }
    return 0;
}

int is_ctrl_pressed(){
    unsigned char pc_keys[256];

    if(GetKeyboardState(pc_keys)){
        if(pc_keys[VK_CONTROL] & 0x80){
            return 1;
        }
    }
    return 0;
}

void set_input_locale(HKL kl){
    char ts_buf[20];

    GetLocaleInfo(LOWORD(kl), LOCALE_IDEFAULTANSICODEPAGE, ts_buf, sizeof(ts_buf));
    kbd_codepage=atoi(ts_buf);
}

void show_mouseptr(int b_show){
    static int b_cursor_visible=1;

    if(!cfg.hide_mouseptr){
        b_show=1;
    }
    if(b_cursor_visible && !b_show){
        ShowCursor(FALSE);
    }
    else if(!b_cursor_visible && b_show){
        ShowCursor(TRUE);
    }
    b_cursor_visible=b_show;
}

void palette_set(void * frontend, int tn_i, int tn_r, int tn_g, int tn_b){
    if(tn_i >= 16){
        tn_i += 256 - 16;
    }
    if(tn_i > NALLCOLOURS){
        return;
    }
    colours[tn_i] = RGB(tn_r, tn_g, tn_b);
    if(tn_i == (ATTR_DEFBG>>ATTR_BGSHIFT)){
        InvalidateRect(hwnd, NULL, TRUE);
    }
}

void palette_reset(void * frontend){
    InvalidateRect(hwnd, NULL, TRUE);
}

void set_busy_status(void * frontend, int tn_status){
    return;
}

void set_raw_mouse_mode(void * frontend, int tn_activate){
    return;
}

void ldisc_update(void * frontend, int echo, int edit){
}

void linedisc_send(char * s_buf, int n_len){
    if(!back){
        return;
    }
    if(n_len>0){
        if(EDITING){
            ldisc_edit(s_buf, n_len);
        }
        else{
            if(ECHOING){
                term_data(term, s_buf, n_len);
            }
            back->send(backhandle, s_buf, n_len);
        }
    }
}

void linedisc_send_special(char * s_buf, int n_len){
    if(EDITING){
        b_ldisc_special=0x100;
        ldisc_edit(s_buf, n_len);
        b_ldisc_special=0;
    }
    else if(cfg.protocol == PROT_TELNET && n_len == 1){
        if(cfg.telnet_keyboard){
            switch(s_buf[0]){
                case CTRL('?'):
                case CTRL('H'):
                    back->special(backhandle, TS_EC);
                    break;
                case CTRL('C'):
                    back->special(backhandle, TS_IP);
                    break;
                case CTRL('Z'):
                    back->special(backhandle, TS_SUSP);
                    break;
                default:
                    back->send(backhandle, s_buf, n_len);
            }
        }
        else{
            if(s_buf[0]==CTRL('M')){
                if(cfg.protocol == PROT_TELNET && cfg.telnet_newline){
                    back->special(backhandle, TS_EOL);
                }
                else{
                    back->send(backhandle, "\r", 1);
                }
            }
            else{
                back->send(backhandle, s_buf, n_len);
            }
        }
    }
    else{
        back->send(backhandle, s_buf, n_len);
    }
}

void ldisc_edit(char * s_buf, int n_len){
    int tn_char;
    int n;
    int i;

    while(n_len--){
        tn_char=(unsigned char)(*s_buf++) + b_ldisc_special;
        if(b_ldisc_quotenext){
            if(n_ldisc_len >= n_ldisc_size){
                n_ldisc_size=n_ldisc_len + 256;
                s_ldisc_buf=sresize(s_ldisc_buf, n_ldisc_size, char);
            }
            s_ldisc_buf[n_ldisc_len++] = tn_char;
            if(ECHOING){
                do_pwrite((unsigned char) tn_char);
            }
            b_ldisc_quotenext=FALSE;
        }
        else{
            switch(tn_char){
                case CTRL('V'):	       /* quote next char */;
                    b_ldisc_quotenext=TRUE;
                    break;
                case KCTRL('H'):
                case KCTRL('?'):
                    if(n_ldisc_len > 0){
                        while(1){
                            if(ECHOING){
                                n=get_plen(s_ldisc_buf[n_ldisc_len-1]);
                                for(i=0;i<n;i++){
                                    term_data(term, "\010 \010", 3);
                                }
                            }
                            n_ldisc_len--;
                            if(in_utf){
                                if(s_ldisc_buf[n_ldisc_len] < 0x80 || s_ldisc_buf[n_ldisc_len] >= 0xC0){
                                    break;
                                }
                            }
                            else{
                                break;
                            }
                        }
                    }
                    break;
                case CTRL('W'):	       /* delete word */;
                    while(n_ldisc_len > 0){
                        if(ECHOING){
                            n=get_plen(s_ldisc_buf[n_ldisc_len-1]);
                            for(i=0;i<n;i++){
                                term_data(term, "\010 \010", 3);
                            }
                        }
                        n_ldisc_len--;
                        if(n_ldisc_len > 0 && isspace((unsigned char)s_ldisc_buf[n_ldisc_len-1]) && !isspace((unsigned char)s_ldisc_buf[n_ldisc_len])){
                            break;
                        }
                    }
                    break;
                case CTRL('U'):	       /* delete line */;
                case CTRL('C'):	       /* Send IP */;
                case CTRL('\\'):	       /* Quit */;
                case CTRL('Z'):	       /* Suspend */;
                    while(n_ldisc_len > 0){
                        if(ECHOING){
                            n=get_plen(s_ldisc_buf[n_ldisc_len-1]);
                            for(i=0;i<n;i++){
                                term_data(term, "\010 \010", 3);
                            }
                        }
                        n_ldisc_len--;
                    }
                    back->special(backhandle, TS_EL);
                    if(!cfg.telnet_keyboard){
                        if(n_ldisc_len >= n_ldisc_size){
                            n_ldisc_size=n_ldisc_len + 256;
                            s_ldisc_buf=sresize(s_ldisc_buf, n_ldisc_size, char);
                        }
                        s_ldisc_buf[n_ldisc_len++] = tn_char;
                        if(ECHOING){
                            do_pwrite((unsigned char) tn_char);
                        }
                        b_ldisc_quotenext=FALSE;
                    }
                    else if(tn_char == CTRL('C')){
                        back->special(backhandle, TS_IP);
                    }
                    else if(tn_char == CTRL('Z')){
                        back->special(backhandle, TS_SUSP);
                    }
                    else if(tn_char == CTRL('\\')){
                        back->special(backhandle, TS_ABORT);
                    }
                    break;
                case CTRL('R'):	       /* redraw line */;
                    if(ECHOING){
                        term_data(term, "^R\r\n", 4);
                        for(i=0;i<n_ldisc_len;i++){
                            do_pwrite(s_ldisc_buf[i]);
                        }
                    }
                    break;
                case CTRL('D'):	       /* logout or send */;
                    if(n_ldisc_len == 0){
                        back->special(backhandle, TS_EOF);
                    }
                    else{
                        back->send(backhandle, s_ldisc_buf, n_ldisc_len);
                        n_ldisc_len=0;
                    }
                    break;
                case CTRL('J'):
                    if(cfg.protocol == PROT_RAW && n_ldisc_len > 0 && s_ldisc_buf[n_ldisc_len - 1] == '\r'){
                        if(ECHOING){
                            n=get_plen(s_ldisc_buf[n_ldisc_len-1]);
                            for(i=0;i<n;i++){
                                term_data(term, "\010 \010", 3);
                            }
                        }
                        n_ldisc_len--;
                        if(n_ldisc_len > 0){
                            back->send(backhandle, s_ldisc_buf, n_ldisc_len);
                        }
                        if(cfg.protocol == PROT_RAW){
                            back->send(backhandle, "\r\n", 2);
                        }
                        else if(cfg.protocol == PROT_TELNET && cfg.telnet_newline){
                            back->special(backhandle, TS_EOL);
                        }
                        else{
                            back->send(backhandle, "\r", 1);
                        }
                        if(ECHOING){
                            term_data(term, "\r\n", 2);
                        }
                        n_ldisc_len=0;
                        break;
                    }
                    else{
                        if(n_ldisc_len >= n_ldisc_size){
                            n_ldisc_size=n_ldisc_len + 256;
                            s_ldisc_buf=sresize(s_ldisc_buf, n_ldisc_size, char);
                        }
                        s_ldisc_buf[n_ldisc_len++] = tn_char;
                        if(ECHOING){
                            do_pwrite((unsigned char) tn_char);
                        }
                        b_ldisc_quotenext=FALSE;
                    }
                case KCTRL('M'):	       /* send with newline */;
                    if(n_ldisc_len > 0){
                        back->send(backhandle, s_ldisc_buf, n_ldisc_len);
                    }
                    if(cfg.protocol == PROT_RAW){
                        back->send(backhandle, "\r\n", 2);
                    }
                    else if(cfg.protocol == PROT_TELNET && cfg.telnet_newline){
                        back->special(backhandle, TS_EOL);
                    }
                    else{
                        back->send(backhandle, "\r", 1);
                    }
                    if(ECHOING){
                        term_data(term, "\r\n", 2);
                    }
                    n_ldisc_len=0;
                    break;
                default:
                    if(n_ldisc_len >= n_ldisc_size){
                        n_ldisc_size=n_ldisc_len + 256;
                        s_ldisc_buf=sresize(s_ldisc_buf, n_ldisc_size, char);
                    }
                    s_ldisc_buf[n_ldisc_len++] = tn_char;
                    if(ECHOING){
                        do_pwrite((unsigned char) tn_char);
                    }
                    b_ldisc_quotenext=FALSE;
                    break;
            }
        }
    }
}

int get_plen(unsigned char c){
    if((c >= 32 && c <= 126) || (c >= 160 && !in_utf)){
        return 1;
    }
    else if(c < 128){
        return 2;
    }
    else if(in_utf && c >= 0xC0){
        return 1;
    }
    else if(in_utf && c >= 0x80 && c < 0xC0){
        return 0;
    }
    else{
        return 4;
    }
}

void do_pwrite(unsigned char c){
    if((c >= 32 && c <= 126) || (!in_utf && c >= 0xA0) || (in_utf && c >= 0x80)){
        term_data(term, (char *)&c, 1);
    }
    else if(c < 128){
        char cc[2];
        cc[1] = (c == 127 ? '?' : c + 0x40);
        cc[0] = '^';
        term_data(term, cc, 2);
    }
    else{
        char cc[5];
        sprintf(cc, "<%02X>", c);
        term_data(term, cc, 4);
    }
}

void linedisc_send_codepage(int n_codepage, char * s_buf, int n_len){
    wchar_t * p_wbuf;
    int tn_size;
    int tn_wlen;

    if(n_codepage<0){
        linedisc_send(s_buf, n_len);
        return;
    }
    else{
        tn_size=n_len*2;
        p_wbuf=(wchar_t*)malloc(tn_size*sizeof(wchar_t));
        tn_wlen=mb_to_wc(n_codepage, 0, s_buf, n_len, p_wbuf, tn_size);
        linedisc_send_unicode(p_wbuf, tn_wlen);
        free(p_wbuf);
    }
}

void linedisc_send_unicode(wchar_t * p_wbuf, int n_len){
    int tn_ratio;
    int tn_linesize;
    char * ts_line;
    char * p;
    int i;
    unsigned long tn_ch;
    unsigned long tn_ch2;
    char ch;
    int tn_ret;

    tn_ratio=1;
    if(in_utf){
        tn_ratio=3;
    }
    tn_linesize=n_len * tn_ratio * 2;
    ts_line=(char*)malloc(tn_linesize*sizeof(char));
    if(in_utf){
        p=ts_line;
        for(i=0;i<n_len;i++){
            tn_ch=p_wbuf[i];
            if((tn_ch & 0xF800) == 0xD800){
                if(i< n_len-1){
                    tn_ch2=p_wbuf[i+1];
                    if((tn_ch & 0xFC00) == 0xD800 && (tn_ch2 & 0xFC00) == 0xDC00){
                        tn_ch=0x10000 + ((tn_ch & 0x3FF) << 10) + (tn_ch2 & 0x3FF);
                        i++;
                    }
                }
                else{
                    ch='.';
                }
            }
            if(tn_ch < 0x80){
                *p++ = (char) (tn_ch);
            }
            else if(tn_ch < 0x800){
                *p++ = (char) (0xC0 | (tn_ch >> 6));
                *p++ = (char) (0x80 | (tn_ch & 0x3F));
            }
            else if(tn_ch < 0x10000){
                *p++ = (char) (0xE0 | (tn_ch >> 12));
                *p++ = (char) (0x80 | ((tn_ch >> 6) & 0x3F));
                *p++ = (char) (0x80 | (tn_ch & 0x3F));
            }
            else{
                *p++ = (char) (0xF0 | (tn_ch >> 18));
                *p++ = (char) (0x80 | ((tn_ch >> 12) & 0x3F));
                *p++ = (char) (0x80 | ((tn_ch >> 6) & 0x3F));
                *p++ = (char) (0x80 | (tn_ch & 0x3F));
            }
        }
        if(p > ts_line){
            linedisc_send(ts_line, p - ts_line);
        }
    }
    else{
        tn_ret=wc_to_mb(line_codepage, p_wbuf, n_len, ts_line, tn_linesize);
        if(tn_ret>0){
            linedisc_send(ts_line, tn_ret);
        }
    }
    free(ts_line);
}

int get_userpass_input(prompts_t * p_prompts, unsigned char * ts_in, int tn_inlen){
    int tn_ret;
    struct term_userpass_state * tp_state;
    int tn_len;
    int i;
    prompt_t * tp_prompt;
    char c;

    tn_ret=cmdline_get_passwd_input(p_prompts, ts_in, tn_inlen);
    if(tn_ret==-1){
        tp_state=(struct term_userpass_state *)p_prompts->data;
        if(!tp_state){
            tp_state=snew(struct term_userpass_state);
            tp_state->n_pos_prompt=0;
            tp_state->tb_done_prompt=0;
            p_prompts->data=tp_state;
            if(p_prompts->name_reqd && p_prompts->name){
                tn_len=strlen(p_prompts->name);
                term_data_untrusted(term, p_prompts->name, tn_len);
                if(p_prompts->name[tn_len-1] != '\n'){
                    term_data_untrusted(term, "\n", 1);
                }
            }
            if(p_prompts->instruction){
                tn_len=strlen(p_prompts->instruction);
                term_data_untrusted(term, p_prompts->instruction, tn_len);
                if(p_prompts->instruction[tn_len-1] != '\n'){
                    term_data_untrusted(term, "\n", 1);
                }
            }
            for(i=0;i<(int)p_prompts->n_prompts;i++){
                memset(p_prompts->prompts[i]->result, 0, p_prompts->prompts[i]->result_len);
            }
        }
        while(tp_state->n_pos_prompt < (int)p_prompts->n_prompts){
            tp_prompt=p_prompts->prompts[tp_state->n_pos_prompt];
            if(!tp_state->tb_done_prompt){
                term_data_untrusted(term, tp_prompt->prompt, strlen(tp_prompt->prompt));
                tp_state->tb_done_prompt=1;
                tp_state->n_pos_cursor=0;
            }
            if(!ts_in || !tn_inlen){
                break;
            }
            else{
                while(tn_inlen){
                    c=*ts_in++;
                    tn_inlen--;
                    if(c==10 || c==13){
                        term_data(term, "\r\n", 2);
                        tp_prompt->result[tp_state->n_pos_cursor] = '\0';
                        tp_prompt->result[tp_prompt->result_len - 1] = '\0';
                        tp_state->n_pos_prompt++;
                        tp_state->tb_done_prompt=0;
                        break;
                    }
                    else if(c==8 || c==127){
                        if(tp_state->n_pos_cursor > 0){
                            if(tp_prompt->echo){
                                term_data(term, "\b \b", 3);
                            }
                            tp_state->n_pos_cursor--;
                        }
                    }
                    else if(c==21 || c==27){
                        while(tp_state->n_pos_cursor > 0){
                            if(tp_prompt->echo){
                                term_data(term, "\b \b", 3);
                            }
                            tp_state->n_pos_cursor--;
                        }
                    }
                    else if(c==3 || c==4){
                        term_data(term, "\r\n", 2);
                        sfree(tp_state);
                        p_prompts->data=NULL;
                        return 0;
                    }
                    else{
                        if((!tp_prompt->echo || (c >= ' ' && c <= '~') || ((unsigned char) c >= 160))){
                            if(tp_state->n_pos_cursor < (int)tp_prompt->result_len - 1){
                                tp_prompt->result[tp_state->n_pos_cursor++] = c;
                                if(tp_prompt->echo){
                                    term_data(term, &c, 1);
                                }
                            }
                        }
                    }
                }
            }
        }
        if(tp_state->n_pos_prompt < (int)p_prompts->n_prompts){
            return -1;
        }
        else{
            sfree(tp_state);
            p_prompts->data=NULL;
            return 1;
        }
    }
    return tn_ret;
}

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR s_cmdline, int n_cmdshow){
    HRESULT hr;
    int b_allow_launch=0;
    Backend * tp_b;
    char * s;
    int n;
    HANDLE filemap;
    Config * tp_cfg;
    int argc;
    char ** argv;
    int i;
    int tn_ret;
    char * s1;
    char * s2;
    int b_got_host=0;
    WNDCLASSEX wc;
    const char * s_appname=appname;
    int tn_w;
    int tn_h;
    RECT rct_temp;
    HMONITOR h_monitor;
    MONITORINFO info_monitor;
    int tn_ws;
    int tn_wsex;
    RECT rct_cr;
    RECT rct_wr;
    int tn_size;
    char * ts_bits;
    SCROLLINFO si;
    int pn_color_idx_map[]={256,257,258,259,260,261,0,8,1,9,2,10,3,11,4,12,5,13,6,14,7,15};
    int tn_i;
    unsigned char pc_color6[]={0,95,135,175,215,255};
    int tn_r;
    int tn_g;
    int tn_b;
    int tn_shade;
    MENUINFO info_menu;
    int j;
    HMENU menu_temp;
    char ts_temp_buf[100];
    HANDLE * handles;
    int n_handles;
    MSG msg;

    cur_instance = hInst;

    AllocConsole();
    freopen("conout$", "w", stdout);
    hwnd=NULL;
    back=NULL;
    hinst=cur_instance;
    flags=FLAG_VERBOSE | FLAG_INTERACTIVE;
    sk_init();
    InitCommonControls();
    defuse_showwindow();
    init_winver();
    init_help();
    hr=CoInitialize(NULL);
    if(! (hr==S_OK || hr==S_FALSE)){
        return 1;
    }
    b_allow_launch=0;
    default_protocol=be_default_protocol;
    tp_b=backend_from_proto(default_protocol);
    if(tp_b){
        default_port=tp_b->default_port;
    }
    cfg.logtype=LGTYP_NONE;
    do_defaults(NULL, &cfg);
    s=s_cmdline;
    while(*s && isspace(*s)){
        s++;
    }
    if(*s == '@'){
        n=strlen(s);
        while(n > 1 && isspace(s[n - 1])){
            n--;
        }
        s[n] = '\0';
        do_defaults(s + 1, &cfg);
        if(!cfg_launchable(&cfg)){
            do_defaults_file(s + 1, &cfg);
        }
        if(!cfg_launchable(&cfg) && !do_config()){
            cleanup_exit(0);
        }
        b_allow_launch=1;
    }
    else if(*s == '&'){
        if(sscanf(s+1, "%p", &filemap) == 1){
            tp_cfg=MapViewOfFile(filemap, FILE_MAP_READ, 0, 0, sizeof(tp_cfg));
            if(tp_cfg != NULL){
                cfg=*tp_cfg;
                UnmapViewOfFile(tp_cfg);
                CloseHandle(filemap);
            }
        }
        else if(!do_config()){
            cleanup_exit(0);
        }
        b_allow_launch=1;
    }
    else{
        split_into_argv(s_cmdline, &argc, &argv, NULL);
        for(i=0;i<argc;i++){
            s=argv[i];
            tn_ret=cmdline_process_param(s, i+1<argc?argv[i+1]:NULL, 1, &cfg);
            if(tn_ret== -2){
                cmdline_error("option \"%s\" requires an argument", s);
            }
            else if(tn_ret==2){
                i++;
            }
            else if(tn_ret==1){
                continue;
            }
            else if(strcmp(s, "-cleanup")==0 || strcmp(s, "-cleanup-during-uninstall")==0){
                if(strcmp(s, "-cleanup-during-uninstall")==0){
                    s1=dupprintf("Remove saved sessions and random seed file?\n" "\n" "If you hit Yes, ALL Registry entries associated\n" "with %s will be removed, as well as the\n" "random seed file. THIS PROCESS WILL\n" "DESTROY YOUR SAVED SESSIONS.\n" "(This only affects the currently logged-in user.)\n" "\n" "If you hit No, uninstallation will proceed, but\n" "saved sessions etc will be left on the machine.", appname);
                    s2=dupprintf("%s Uninstallation", appname);
                }
                else{
                    s1=dupprintf("This procedure will remove ALL Registry entries\n" "associated with %s, and will also remove\n" "the random seed file. (This only affects the\n" "currently logged-in user.)\n" "\n" "THIS PROCESS WILL DESTROY YOUR SAVED SESSIONS.\n" "Are you really sure you want to continue?", appname);
                    s2=dupprintf("%s Warning", appname);
                }
                if(message_box(s1, s2, MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2, HELPCTXID(option_cleanup)) == IDYES){
                    cleanup_all();
                }
                sfree(s1);
                sfree(s2);
                exit(0);
            }
            else if(strcmp(s, "-pgpfp")==0){
                pgp_fingerprints();
                exit(1);
            }
            else if(*s != '-'){
                if(b_got_host){
                    tn_ret=cmdline_process_param("-P", s, 1, &cfg);
                    assert(tn_ret == 2);
                }
                else if(strncmp(s, "telnet:", 7)==0){
                    s=s+7;
                    if(s[0] == '/' && s[1] == '/'){
                        s += 2;
                    }
                    cfg.protocol=PROT_TELNET;
                    s1=s;
                    while(*s1 && *s1 != ':' && *s1 !='/'){
                        s1++;
                    }
                    if(*s1==':'){
                        cfg.port=atoi(s1+1);
                    }
                    else{
                        cfg.port=-1;
                    }
                    if(*s1){
                        *s1='\0';
                    }
                    strncpy(cfg.host, s1, sizeof(cfg.host) - 1);
                    cfg.host[sizeof(cfg.host) - 1] = '\0';
                    b_got_host=1;
                }
                else{
                    s1=s;
                    while(*s1 && !isspace(*s1)){
                        s1++;
                    }
                    if(*s1){
                        *s1='\0';
                    }
                    strncpy(cfg.host, s, sizeof(cfg.host) - 1);
                    cfg.host[sizeof(cfg.host) - 1] = '\0';
                    b_got_host=1;
                }
            }
            else{
                cmdline_error("unknown option \"%s\"", s);
            }
        }
    }
    cmdline_run_saved(&cfg);
    if(loaded_session || b_got_host){
        b_allow_launch=1;
    }
    if(!b_allow_launch || !cfg_launchable(&cfg)){
        tn_ret=do_config();
        if(!tn_ret){
            cleanup_exit(0);
        }
    }
    s=cfg.host;
    while(*s && isspace(*s)){
        s++;
    }
    memmove(cfg.host, s, strlen(s)+1);
    if(cfg.host[0] != '\0'){
        s=strrchr(cfg.host, '@');
        if(s){
            if(s-cfg.host < sizeof(cfg.username)){
                strncpy(cfg.username, cfg.host, s - cfg.host);
                cfg.username[s - cfg.host] = '\0';
            }
            memmove(cfg.host, s + 1, 1 + strlen(s + 1));
        }
    }
    s=strchr(cfg.host, ':');
    if(s){
        if(! strchr(s+1, ':')){
            *s='\0';
        }
    }
    s=cfg.host;
    while(*s && isspace(*s)){
        s++;
    }
    memmove(cfg.host, s, strlen(s)+1);
    if(cfg.win_icon[0]){
        wc.hIcon=extract_icon(cfg.win_icon, FALSE);
        wc.hIconSm=extract_icon(cfg.win_icon, TRUE);
    }
    else{
        wc.hIcon=LoadImage(cur_instance, MAKEINTRESOURCE(IDI_MAINICON), IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR|LR_SHARED);
        wc.hIconSm=LoadImage(cur_instance, MAKEINTRESOURCE(IDI_MAINICON), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR|LR_SHARED);
    }
    wc.cbSize=sizeof(WNDCLASSEX);
    wc.style=0;
    wc.lpfnWndProc=WndProc_main;
    wc.cbClsExtra=0;
    wc.cbWndExtra=0;
    wc.hInstance=cur_instance;
    wc.hCursor=LoadCursor(NULL, IDC_IBEAM);
    wc.hbrBackground=NULL;
    wc.lpszMenuName=NULL;
    wc.lpszClassName=s_appname;
    RegisterClassEx(&wc);
    font_width=10;
    font_height=20;
    extra_width=25;
    extra_height=28;
    tn_w=extra_width + font_width * cfg.width;
    tn_h=extra_height + font_height * cfg.height;
    h_monitor=MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    info_monitor.cbSize=sizeof(info_monitor);
    GetMonitorInfo(h_monitor, &info_monitor);
    rct_temp=info_monitor.rcMonitor;
    if(tn_w>rct_temp.right-rct_temp.left){
        tn_w=rct_temp.right-rct_temp.left;
    }
    if(tn_h>rct_temp.bottom-rct_temp.top){
        tn_h=rct_temp.bottom-rct_temp.top;
    }
    tn_ws=WS_OVERLAPPEDWINDOW | WS_VSCROLL;
    tn_wsex=0;
    if(!cfg.scrollbar){
        tn_ws &= ~(WS_VSCROLL);
    }
    if(cfg.resize_action == RESIZE_DISABLED){
        tn_ws &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
    }
    if((cfg.alwaysontop)){
        tn_wsex |= WS_EX_TOPMOST;
    }
    if(cfg.sunken_edge){
        tn_wsex |= WS_EX_CLIENTEDGE;
    }
    hwnd=CreateWindowEx(tn_wsex, appname, appname, tn_ws, CW_USEDEFAULT, CW_USEDEFAULT, tn_w, tn_h, NULL, NULL, cur_instance, NULL);
    term=term_init(&cfg);
    logctx=log_init(NULL, &cfg);
    term_set_scrollback_size(term, cfg.savelines);
    n_rows=cfg.height;
    n_cols=cfg.width;
    term_size(term, n_rows, n_cols);
    if(back){
        back->size(backhandle, n_cols, n_rows);
    }
    init_fonts(0,0);
    GetWindowRect(hwnd, &rct_wr);
    GetClientRect(hwnd, &rct_cr);
    offset_width=cfg.window_border;
    offset_height=cfg.window_border;
    extra_width=rct_wr.right - rct_wr.left - rct_cr.right + rct_cr.left + offset_width*2;
    extra_height=rct_wr.bottom - rct_wr.top - rct_cr.bottom + rct_cr.top +offset_height*2;
    tn_w=extra_width + font_width * n_cols;
    tn_h=extra_height + font_height * n_rows;
    tn_ret=SetWindowPos(hwnd, NULL, 0, 0, tn_w, tn_h, SWP_NOMOVE | SWP_NOREDRAW | SWP_NOZORDER);
    tn_size=(font_width + 15) / 16 * 2 * font_height;
    ts_bits=snewn(tn_size, char);
    memset(ts_bits, 0, tn_size);
    caretbm=CreateBitmap(font_width, font_height, 1, 1, ts_bits);
    sfree(ts_bits);
    CreateCaret(hwnd, caretbm, font_width, font_height);
    si.cbSize=sizeof(si);
    si.fMask=SIF_ALL | SIF_DISABLENOSCROLL;
    si.nMin=0;
    si.nMax=n_rows - 1;
    si.nPage=n_rows;
    si.nPos=0;
    SetScrollInfo(hwnd, SB_VERT, &si, FALSE);
    start_backend();
    for(i=0;i<22;i++){
        tn_i=pn_color_idx_map[i];
        colours[tn_i]=RGB(cfg.colours[i][0], cfg.colours[i][1], cfg.colours[i][2]);
    }
    tn_i=16;
    for(tn_r=0;tn_r<6;tn_r++){
        for(tn_g=0;tn_g<6;tn_g++){
            for(tn_b=0;tn_b<6;tn_b++){
                colours[tn_i]=RGB(pc_color6[tn_r], pc_color6[tn_g], pc_color6[tn_b]);
                tn_i++;
            }
        }
    }
    for(tn_i=0;tn_i<24;tn_i++){
        tn_shade=tn_i*10+8;
        colours[tn_i+232]=RGB(tn_shade, tn_shade, tn_shade);
    }
    if(cfg.system_colour){
        colours[COLOR_DEFFG]=GetSysColor(COLOR_WINDOWTEXT);
        colours[COLOR_DEFFG_BOLD]=GetSysColor(COLOR_WINDOWTEXT);
        colours[COLOR_DEFBG]=GetSysColor(COLOR_WINDOW);
        colours[COLOR_DEFBG_BOLD]=GetSysColor(COLOR_WINDOW);
        colours[COLOR_CURFG]=GetSysColor(COLOR_HIGHLIGHTTEXT);
        colours[COLOR_CURBG]=GetSysColor(COLOR_HIGHLIGHT);
    }
    puttyTray.cbSize=sizeof(NOTIFYICONDATA);
    puttyTray.hWnd=hwnd;
    puttyTray.uID=1983;
    puttyTray.uFlags=NIF_MESSAGE | NIF_ICON | NIF_TIP;
    puttyTray.uCallbackMessage=WM_NOTIFY_PUTTYTRAY;
    puttyTray.hIcon=wc.hIconSm;
    memset(&info_menu, 0, sizeof(MENUINFO));
    info_menu.cbSize=sizeof(MENUINFO);
    info_menu.fMask=MIM_STYLE;
    info_menu.dwStyle=MNS_NOCHECK | MNS_AUTODISMISS;
    SetMenuInfo(popup_menus[MENU_CTX], &info_menu);
    popup_menus[MENU_SYS] = GetSystemMenu(hwnd, FALSE);
    popup_menus[MENU_CTX] = CreatePopupMenu();
    AppendMenu(popup_menus[MENU_CTX], MF_ENABLED, IDM_PASTE, "&Paste");
    menu_savedsess=CreateMenu();
    sesslist.buffer=NULL;
    sesslist.sessions=NULL;
    for(j=0;j<lenof(popup_menus);j++){
        menu_temp=popup_menus[j];
        AppendMenu(menu_temp, MF_SEPARATOR, 0, NULL);
        AppendMenu(menu_temp, MF_STRING, IDM_SHOWLOG, "&Event Log");
        AppendMenu(menu_temp, MF_SEPARATOR, 0, NULL);
        AppendMenu(menu_temp, MF_STRING, IDM_NEWSESS, "Ne&w Session");
        AppendMenu(menu_temp, MF_STRING, IDM_DUPSESS, "&Duplicate Session");
        AppendMenu(menu_temp, MF_STRING, IDM_RESTART, "&Restart Session");
        AppendMenu(menu_temp, MF_POPUP, (UINT_PTR)menu_savedsess, "Sa&ved Sessions");
        AppendMenu(menu_temp, MF_STRING, IDM_RECONF, "Chan&ge Settings...");
        AppendMenu(menu_temp, MF_SEPARATOR, 0, NULL);
        AppendMenu(menu_temp, MF_STRING, IDM_COPYALL, "C&opy All to Clipboard");
        AppendMenu(menu_temp, MF_STRING, IDM_CLRSB, "C&lear Scrollback");
        AppendMenu(menu_temp, MF_STRING, IDM_RESET, "Rese&t Terminal");
        AppendMenu(menu_temp, MF_SEPARATOR, 0, NULL);
        AppendMenu(menu_temp, MF_STRING, IDM_FULLSCREEN, "&Full Screen");
        AppendMenu(menu_temp, MF_STRING, IDM_VISIBLE, "Alwa&ys on top");
        AppendMenu(menu_temp, MF_STRING, IDM_HELP, "&Help");
        AppendMenu(menu_temp, MF_SEPARATOR, 0, NULL);
        snprintf(ts_temp_buf, 100, "&About %s", appname);
        AppendMenu(menu_temp, MF_STRING, IDM_ABOUT, ts_temp_buf);
    }
    set_input_locale(GetKeyboardLayout(0));
    puttyTrayVisible=FALSE;
    if(cfg.tray == TRAY_START || cfg.tray == TRAY_ALWAYS){
        taskbar_addicon(cfg.win_name_always ? window_name : icon_name, TRUE);
    }
    if(cfg.tray == TRAY_START){
        ShowWindow(hwnd, SW_HIDE);
        windowMinimized=TRUE;
    }
    else{
        ShowWindow(hwnd, n_cmdshow);
        SetForegroundWindow(hwnd);
        b_has_focus=GetForegroundWindow() == hwnd;
        UpdateWindow(hwnd);
    }
    if(cfg.transparency >= 50 && cfg.transparency < 255){
        MakeWindowTransparent(hwnd, cfg.transparency);
    }
    while(1){
        handles=handle_get_events(&n_handles);
        n=MsgWaitForMultipleObjects(n_handles, handles, FALSE, INFINITE, QS_ALLINPUT);
        if((unsigned)(n - WAIT_OBJECT_0) < (unsigned)n_handles){
            handle_got_event(handles[n - WAIT_OBJECT_0]);
            sfree(handles);
            if(must_close_session){
                close_session();
            }
        }
        else{
            sfree(handles);
        }
        while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)){
            if(msg.message == WM_QUIT){
                goto finished;
            }
            if(!(IsWindow(logbox) && IsDialogMessage(logbox, &msg))){
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            if(must_close_session){
                close_session();
            }
        }
        b_has_focus=GetForegroundWindow() == hwnd;
        if(b_pending_netevent){
            enact_pending_netevent();
        }
        net_pending_errors();
    }
    finished:
    cleanup_exit(msg.wParam);
    return msg.wParam;
    return 0;
}

