#include <winsock2.h>
#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include "tree234.h"
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
#define NO_SELECTION 0
#define ABOUT_TO 1
#define DRAGGING 2
#define SELECTED 3
#define VBELL_TIMEOUT (TICKSPERSEC/10)
#define TBLINK_DELAY ((TICKSPERSEC*9+19)/20)
#define CBLINK_DELAY (CURSORBLINK)
#define VBELL_DELAY (VBELL_TIMEOUT)
#define UPDATE_DELAY ((TICKSPERSEC+49)/50)
#define termstate_TOPLEVEL 0
#define termstate_SEEN_ESC 1
#define termstate_SEEN_CSI 2
#define termstate_SEEN_OSC 3
#define termstate_SEEN_QUERY 4
#define termstate_DO_CTRLS 5
#define termstate_PRINTING 6
#define ARGS_MAX 32
#define OSC_STR_MAX 100
#define RLE_NoMatch 0
#define RLE_Match 1
#define RLE_Run 2

typedef struct pos pos;
typedef struct termchar termchar;
typedef struct termline termline;
typedef struct tagTerminal Terminal;

struct tagTerminal {
	int cols;
	int rows;
	tree234 *screen;
	int alt_sblines;
	int alt_which;
	tree234 *alt_screen;
	int selstate;
	unsigned char* pc_tabs;
	int window_update_pending;
	int cblinker;
	int tblinker;
	int in_vbell;
	int b_seen_disp_event;
	pos curs;
	int cset_attr[2];
	int default_attr;
	int marg_t;
	int marg_b;
	int curr_attr;
	int wrapnext;
	int cset;
	int utf;
	int sco_acs;
	int save_curr_attr;
	int save_wrapnext;
	int save_cset;
	int save_utf;
	int save_sco_acs;
	pos save_curs;
	int save_csattr;
	termchar basic_erase_char;
	termchar erase_char;
	tree234 *p_scrollback;
	int n_scrollback_count;
	int n_scrollback_size;
	int b_in_term_out;
	bufchain inbuf;
	int n_termstate;
	int n_state_2;
	int n_esc_args;
	int pn_esc_args[ARGS_MAX];
	int disptop;
	termline **disptext;
	int dispcursx;
	int dispcursy;
	int curstype;
	int cursor_on;
	int rvideo;
	int blink_is_real;
	int wrap;
	int insert;
	char id_string[1024];
	int b_insert;
	int b_echo;
	int b_crlf;
	int b_big_cursor;
	int b_app_cursor_keys;
	int b_rvideo;
	int b_cursor_on;
	int n_utf_state;
	int n_utf_size;
	unsigned long n_utf_char;
	bufchain printer_buf;
	printer_job *p_print_job;
	void *logctx;
	int dec_om;
	int n_osc_strlen;
	wchar_t s_osc_str[OSC_STR_MAX];
};

struct pos {
	int x;
	int y;
};

struct termchar {
	unsigned long chr;
	unsigned long attr;
	int cc_next;
};

struct termline {
	unsigned short lattr;
	int cols;
	int size;
	int cc_free;
	struct termchar *chars;
	int b_temporary;
};

struct beep_node {
	unsigned long n_ticks;
	struct beep_node * next;
};

struct beep_list {
	int b_overloaded;
	int n_last_tick;
	int n;
	struct beep_node* head;
	struct beep_node* tail;
};

void term_copyall(Terminal * term);
void term_deselect(Terminal * term);
void term_nopaste(Terminal * term);
Terminal * term_init(Config * mycfg);
void term_free(Terminal * term);
void power_on(Terminal * term, int tb_clear);
void term_size(Terminal * term, int n_row, int n_col);
void term_pwron(Terminal * term, int tb_clear);
void term_text_blink_on_timer(Terminal * term, long time_now);
void term_cursor_blink_on_timer(Terminal * term, long time_now);
void term_vbell_on_timer(Terminal * term, long time_now);
void term_update_on_timer(Terminal * term, long time_now);
void term_seen_key_event(Terminal * term);
void move_cursor(Terminal * term, int x, int y, int n_marg_clip);
termline * newline(Terminal * term, int cols, int bce);
void check_boundary(Terminal * term, int x, int y);
void insch(Terminal * term, int x);
void resizeline(Terminal * term, termline * p_line, int tn_cols);
int termchars_equal_override(termchar * a, termchar * b, unsigned long bchr, unsigned long battr);
int termchars_equal(termchar * a, termchar * b);
void copy_termchar(termline * p_line, int x, termchar * p_char);
void move_termchar(termline * p_line, termchar * dest, termchar * src);
int find_last_nonempty_line(Terminal * term, tree234 * screen);
void add_cc(termline * p_line, int col, unsigned long chr);
void clear_cc(termline * p_line, int col);
int cmp_cc(termchar * p_char_a, termchar * p_char_b);
void term_set_scrollback_size(Terminal * term, int n_save);
void term_clrsb(Terminal * term);
int sblines(Terminal * term);
void scroll(Terminal * term, int tn_top, int tn_bot, int tn_lines, int b_scrollback);
termline * lineptr(Terminal * term, int y);
void unlineptr(termline * p_line);
void term_scroll(Terminal * term, int tn_rel, int tn_off);
int term_data(Terminal * term, char * ts_data, int tn_len);
int term_data_untrusted(Terminal * term, char * ts_data, int tn_len);
void term_out(Terminal * term);
void term_invalidate(Terminal * term);
void term_invalidate_rect(Terminal * term, int left, int top, int right, int bottom);
void term_update(Terminal * term);
void do_paint(Terminal * term);
void * compressline(termline * p_line);
termline * decompressline(void * buf);
void term_debug_screen(Terminal * term);
int term_app_cursor_keys(Terminal * term);
void term_provide_logctx(Terminal * term, void * logctx);

void linedisc_send(char * s_buf, int n_len);
void front_set_scrollbar(int, int, int);
void front_set_scrollbar(int, int, int);
void do_beep();
void front_set_scrollbar(int, int, int);
int paint_start();
void paint_finish();
void front_set_scrollbar(int, int, int);

extern Config cfg;
extern wchar_t unitab_scoacs[256];
extern wchar_t unitab_line[256];
extern wchar_t unitab_font[256];
extern wchar_t unitab_draw[256];
extern wchar_t unitab_oemcp[256];
extern unsigned char unitab_ctrl[256];
extern int line_codepage;
Terminal g_term;
struct beep_list sl_beep;
extern int font_isdbcs;
extern int b_has_focus;

void term_copyall(Terminal * term){
    return;
}

void term_deselect(Terminal * term){
    return;
}

void term_nopaste(Terminal * term){
    return;
}

Terminal * term_init(Config * mycfg){
    Terminal * term;

    term=&g_term;
    term->rows=-1;
    term->cols=-1;
    term->screen=NULL;
    term->alt_sblines=0;
    term->alt_which=0;
    term->alt_screen=NULL;
    term->selstate=NO_SELECTION;
    term->pc_tabs=NULL;
    term->window_update_pending=0;
    term->cblinker=0;
    term->tblinker=0;
    term->b_seen_disp_event=0;
    term->basic_erase_char.chr=CSET_ASCII | ' ';
    term->basic_erase_char.attr=ATTR_DEFAULT;
    term->basic_erase_char.cc_next=0;
    term->erase_char=term->basic_erase_char;
    term->p_scrollback=NULL;
    term->n_scrollback_count=0;
    term->b_in_term_out=0;
    bufchain_init(&term->inbuf);
    term->n_termstate=termstate_TOPLEVEL;
    term->n_esc_args=0;
    term->pn_esc_args[ARGS_MAX];
    term->disptext=NULL;
    term->dispcursx=-1;
    term->dispcursy=-1;
    term->curstype=0;
    term->wrap=1;
    strcpy(term->id_string, "\033[?6c");
    term->b_insert=0;
    term->b_echo=0;
    term->b_crlf=0;
    term->b_big_cursor=0;
    term->b_app_cursor_keys=0;
    term->b_rvideo=0;
    term->b_cursor_on=0;
    term->n_utf_state=0;
    term->n_utf_size=0;
    bufchain_init(&term->printer_buf);
    term->p_print_job=NULL;
    term->logctx=NULL;
    power_on(term, TRUE);
    return term;
}

void term_free(Terminal * term){
    termline * p_line;
    int i;

    while(1){
        p_line=delpos234(term->screen, 0);
        if(p_line==NULL){
            break;
        }
        if(p_line){
            sfree(p_line->chars);
            sfree(p_line);
        }
    }
    sfree(term->screen);
    term->screen=NULL;
    while(1){
        p_line=delpos234(term->p_scrollback, 0);
        if(p_line==NULL){
            break;
        }
        sfree(p_line);
    }
    sfree(term->p_scrollback);
    term->p_scrollback=NULL;
    bufchain_clear(&term->inbuf);
    if(term->disptext){
        for(i=0;i<term->rows;i++){
            if(term->disptext[i]){
                sfree(term->disptext[i]->chars);
                sfree(term->disptext[i]);
            }
        }
    }
    sfree(term->disptext);
    if(term->p_print_job){
        printer_finish_job(term->p_print_job);
    }
    bufchain_clear(&term->printer_buf);
    expire_timer_context(term);
}

void power_on(Terminal * term, int tb_clear){
    int i;
    int tn_count;

    if(term->cols >0){
        for(i=0;i<term->cols;i++){
            term->pc_tabs[i] = (i % 8 == 0 ? 1 : 0);
        }
    }
    term->in_vbell=FALSE;
    term->curs.x=0;
    term->curs.y=0;
    term->default_attr=ATTR_DEFAULT;
    term->curr_attr=ATTR_DEFAULT;
    term->cset=0;
    term->utf=0;
    term->wrapnext=0;
    term->cset_attr[0] = CSET_ASCII;
    term->cset_attr[1] = CSET_ASCII;
    term->sco_acs=0;
    term->save_curs=term->curs;
    term->save_curr_attr=term->curr_attr;
    term->save_wrapnext=term->wrapnext;
    term->save_cset=term->cset;
    term->save_utf=term->utf;
    term->save_sco_acs=term->sco_acs;
    term->save_csattr=term->cset_attr[term->cset];
    term->marg_t=0;
    if(term->rows>0){
        term->marg_b=term->rows-1;
    }
    else{
        term->marg_b=0;
    }
    if(term->screen){
        if(tb_clear){
            term->disptop=0;
            term_invalidate(term);
            tn_count=find_last_nonempty_line(term, term->screen)+1;
            fprintf(stdout, "    :[erase_screen_2] tn_count=%d\n", tn_count);
            scroll(term, 0, tn_count-1, tn_count, 1);
        }
        term->curs.y=find_last_nonempty_line(term, term->screen) + 1;
        if(term->curs.y == term->rows){
            term->curs.y--;
            scroll(term, 0, term->rows - 1, 1, TRUE);
        }
    }
    term->disptop=0;
    term->cursor_on=1;
    term->rvideo=0;
    term->blink_is_real=cfg.blinktext;
    term->wrap=cfg.wrap_mode;
    term->insert=0;
    term->dec_om=cfg.dec_om;
    if(term->p_print_job){
        printer_finish_job(term->p_print_job);
    }
    bufchain_clear(&term->printer_buf);
    term->erase_char=term->basic_erase_char;
}

void term_size(Terminal * term, int n_row, int n_col){
    int tn_row_old;
    int tn_col_old;
    int tn_cursor_adjust;
    char * ts_line;
    termline * p_line;
    int tn_shrink;
    int i;
    termline ** newdisp;
    int j;
    int tn_scroll;

    if(n_row == term->rows && n_col == term->cols){
        return;
    }
    if(term->rows == -1){
        term->screen=newtree234(NULL);
        term->rows=0;
    }
    if(n_row < 1){
        n_row=1;
    }
    if(n_col < 1){
        n_col=1;
    }
    tn_row_old=term->rows;
    tn_col_old=term->cols;
    tn_cursor_adjust=0;
    while(term->rows < n_row){
        if(term->n_scrollback_count > 0){
            term->n_scrollback_count--;
            ts_line=delpos234(term->p_scrollback, term->n_scrollback_count);
            p_line=decompressline(ts_line);
            sfree(ts_line);
            addpos234(term->screen, p_line, 0);
            term->rows ++;
            tn_cursor_adjust++;
        }
        else{
            p_line=newline(term, n_col, FALSE);
            addpos234(term->screen, p_line, count234(term->screen));
            term->rows ++;
        }
    }
    while(term->rows > n_row){
        if(term->curs.y < term->rows - 1){
            p_line=delpos234(term->screen, term->rows-1);
            sfree(p_line);
            term->rows --;
        }
        else{
            p_line=delpos234(term->screen, 0);
            if(term->n_scrollback_count>term->n_scrollback_size-1){
                tn_shrink=term->n_scrollback_count-term->n_scrollback_size-1;
                for(i=0;i<tn_shrink;i++){
                    ts_line=delpos234(term->p_scrollback, 0);
                    term->n_scrollback_count--;
                    sfree(ts_line);
                }
            }
            fprintf(stdout, "    :[scrollback_shrink_to] term->n_scrollback_size=%d, term->n_scrollback_count=%d\n", term->n_scrollback_size, term->n_scrollback_count);
            addpos234(term->p_scrollback, compressline(p_line), term->n_scrollback_count);
            term->n_scrollback_count++;
            if(p_line){
                sfree(p_line->chars);
                sfree(p_line);
            }
            term->rows --;
            tn_cursor_adjust--;
        }
    }
    term->rows=n_row;
    term->cols=n_col;
    term->pc_tabs=realloc(term->pc_tabs, term->cols);
    if(tn_col_old<0){
        tn_col_old=0;
    }
    for(i=tn_col_old;i<term->cols;i++){
        term->pc_tabs[i] = (i % 8 == 0 ? 1 : 0);
    }
    term->marg_t=0;
    term->marg_b=n_row - 1;
    term->wrapnext=0;
    term->curs.y += tn_cursor_adjust;
    term->save_curs.y += tn_cursor_adjust;
    if(term->save_curs.x<0){
        term->save_curs.x=0;
    }
    else if(term->save_curs.x>term->cols-1){
        term->save_curs.x=term->cols-1;
    }
    if(term->save_curs.y<0){
        term->save_curs.y=0;
    }
    else if(term->save_curs.y>term->rows-1){
        term->save_curs.y=term->rows-1;
    }
    if(term->curs.x<0){
        term->curs.x=0;
    }
    else if(term->curs.x>term->cols-1){
        term->curs.x=term->cols-1;
    }
    if(term->curs.y<0){
        term->curs.y=0;
    }
    else if(term->curs.y>term->rows-1){
        term->curs.y=term->rows-1;
    }
    term->disptop=0;
    newdisp=snewn(n_row, termline *);
    for(i=0;i<n_row;i++){
        newdisp[i] = newline(term, n_col, FALSE);
        for(j=0;j<n_col;j++){
            newdisp[i]->chars[j].attr=ATTR_INVALID;
        }
    }
    if(term->disptext){
        for(i=0;i<tn_row_old;i++){
            if(term->disptext[i]){
                sfree(term->disptext[i]->chars);
                sfree(term->disptext[i]);
            }
        }
    }
    sfree(term->disptext);
    term->disptext=newdisp;
    term->dispcursx=term->dispcursy = -1;
    tn_scroll=sblines(term);
    front_set_scrollbar(tn_scroll + term->rows, tn_scroll + term->disptop, term->rows);
    term_update(term);
}

void term_pwron(Terminal * term, int tb_clear){
    power_on(term, tb_clear);
    term_update(term);
}

void term_text_blink_on_timer(Terminal * term, long time_now){
    term->tblinker=!term->tblinker;
    schedule_timer(TBLINK_DELAY, term_text_blink_on_timer, term);
    term_update(term);
}

void term_cursor_blink_on_timer(Terminal * term, long time_now){
    term->cblinker=!term->cblinker;
    schedule_timer(CBLINK_DELAY, term_cursor_blink_on_timer, term);
    term_update(term);
}

void term_vbell_on_timer(Terminal * term, long time_now){
    term->in_vbell=0;
    term_update(term);
}

void term_update_on_timer(Terminal * term, long time_now){
    term->window_update_pending=0;
    term_update(term);
}

void term_seen_key_event(Terminal * term){
    struct beep_node * p_beep_node_temp;

    sl_beep.b_overloaded=0;
    while(sl_beep.head){
        p_beep_node_temp=sl_beep.head;
        sl_beep.head=p_beep_node_temp->next;
        sl_beep.n--;
        free(p_beep_node_temp);
    }
    if(!sl_beep.head){
        sl_beep.tail=NULL;
    }
    if(cfg.scroll_on_key){
        term->disptop=0;
        term->b_seen_disp_event=1;
        if(!term->window_update_pending){
            term->window_update_pending=1;
            schedule_timer(UPDATE_DELAY, term_update_on_timer, term);
        }
    }
}

void move_cursor(Terminal * term, int x, int y, int n_marg_clip){
    if(x<0){
        x=0;
    }
    else if(x>term->cols-1){
        x=term->cols-1;
    }
    if(n_marg_clip==2){
        if(y<term->marg_t){
            y=term->marg_t;
        }
        else if(y>term->marg_b){
            y=term->marg_b;
        }
    }
    else if(n_marg_clip){
        if((term->curs.y >= term->marg_t) && y < term->marg_t){
            y=term->marg_t;
        }
        if((term->curs.y <= term->marg_b) && y > term->marg_b){
            y=term->marg_b;
        }
    }
    if(y<0){
        y=0;
    }
    else if(y>term->rows-1){
        y=term->rows-1;
    }
    term->curs.x=x;
    term->curs.y=y;
    term->wrapnext=0;
}

termline * newline(Terminal * term, int cols, int bce){
    termline * p_line;
    int j;

    p_line=snew(termline);
    p_line->chars=snewn(cols, termchar);
    for(j=0;j<cols;j++){
        p_line->chars[j] = (bce ? term->erase_char : term->basic_erase_char);
    }
    p_line->cols=p_line->size = cols;
    p_line->lattr=LATTR_NORM;
    p_line->cc_free=0;
    return p_line;
}

void check_boundary(Terminal * term, int x, int y){
    termline * p_line;

    if(x == 0 || x > term->cols){
        return;
    }
    p_line=index234(term->screen, y);
    if(x == term->cols){
        p_line->lattr &= ~LATTR_WRAPPED2;
    }
    else if(p_line->chars[x].chr == UCSWIDE){
        clear_cc(p_line, x-1);
        clear_cc(p_line, x);
        p_line->chars[x-1].chr=' ' | CSET_ASCII;
        p_line->chars[x] = p_line->chars[x-1];
    }
}

void insch(Terminal * term, int x){
    int tb_dir;
    int tn_remain;
    pos pos_t;
    termline * p_line;
    int j;

    tb_dir=(x < 0 ? -1 : +1);
    x=(x < 0 ? -x : x);
    if(x > term->cols - term->curs.x){
        x=term->cols - term->curs.x;
    }
    tn_remain=term->cols-(term->curs.x+x);
    pos_t.y=term->curs.y;
    pos_t.x=term->curs.x + x;
    check_boundary(term, term->curs.x, term->curs.y);
    p_line=index234(term->screen, term->curs.y);
    if(tb_dir < 0){
        check_boundary(term, term->curs.x + x, term->curs.y);
        for(j=0;j<tn_remain;j++){
            move_termchar(p_line, p_line->chars+term->curs.x+j, p_line->chars+term->curs.x+j+x);
        }
        for(j=0;j<x;j++){
            clear_cc(p_line, term->curs.x+tn_remain+j);
            p_line->chars[term->curs.x+tn_remain+j] = term->erase_char;
        }
    }
    else{
        for(j=tn_remain; j-- ;){
        }
        for(j=0;j<tn_remain;j++){
            move_termchar(p_line, p_line->chars+term->cols-1-j, p_line->chars+term->cols-1-x-j);
        }
        for(j=0;j<x;j++){
            clear_cc(p_line, term->curs.x + x+j);
            p_line->chars[term->curs.x + x+j] = term->erase_char;
        }
    }
}

void resizeline(Terminal * term, termline * p_line, int tn_cols){
    int tn_cols_old;
    int i;

    tn_cols_old=p_line->cols;
    if(tn_cols_old < tn_cols){
        p_line->size += tn_cols - tn_cols_old;
        p_line->chars=sresize(p_line->chars, p_line->size, termchar);
        p_line->cols=tn_cols;
        memmove(p_line->chars+tn_cols, p_line->chars+tn_cols_old, (p_line->size-p_line->cols) * sizeof(termchar));
        for(i=tn_cols_old;i<tn_cols;i++){
            p_line->chars[i] = term->basic_erase_char;
        }
    }
    else if(tn_cols_old > tn_cols){
        for(i=tn_cols;i<tn_cols_old;i++){
            clear_cc(p_line, i);
        }
        memmove(p_line->chars+tn_cols, p_line->chars+tn_cols_old, (p_line->size-p_line->cols) * sizeof(termchar));
        p_line->size += tn_cols - tn_cols_old;
        p_line->chars=sresize(p_line->chars, p_line->size, termchar);
        p_line->cols=tn_cols;
    }
    if(p_line->cols != tn_cols){
        for(i = 0; i < tn_cols_old && i < tn_cols; i++){
            if(p_line->chars[i].cc_next){
                p_line->chars[i].cc_next += tn_cols - tn_cols_old;
            }
        }
        if(p_line->cc_free){
            p_line->cc_free += tn_cols - tn_cols_old;
        }
    }
}

int termchars_equal_override(termchar * a, termchar * b, unsigned long bchr, unsigned long battr){
    if(a->chr != bchr){
        return 0;
    }
    if((a->attr &~ DATTR_MASK) != (battr &~ DATTR_MASK)){
        return 0;
    }
    return cmp_cc(a, b);
}

int termchars_equal(termchar * a, termchar * b){
    if(a->chr != b->chr){
        return 0;
    }
    if((a->attr &~ DATTR_MASK) != (b->attr &~ DATTR_MASK)){
        return 0;
    }
    return cmp_cc(a, b);
}

void copy_termchar(termline * p_line, int x, termchar * p_char){
    clear_cc(p_line, x);
    p_line->chars[x] = *p_char;
    p_line->chars[x].cc_next=0;
    while(p_char->cc_next){
        p_char += p_char->cc_next;
        add_cc(p_line, x, p_char->chr);
    }
}

void move_termchar(termline * p_line, termchar * dest, termchar * src){
    clear_cc(p_line, dest - p_line->chars);
    *dest=*src;
    if(src->cc_next){
        dest->cc_next=src->cc_next - (dest-src);
    }
    src->cc_next=0;
}

int find_last_nonempty_line(Terminal * term, tree234 * screen){
    int n;
    int i;
    termline * p_line;
    int j;

    n=count234(screen);
    for(i=n-1;i>-1;i--){
        p_line=index234(screen, i);
        for(j=0;j<p_line->cols;j++){
            if(!termchars_equal(&p_line->chars[j], &term->erase_char)){
                return i;
            }
        }
    }
    return 0;
}

void add_cc(termline * p_line, int col, unsigned long chr){
    int tn_old_size;
    int i;
    int tn_free_x;
    int tn_x;

    assert(col >= 0 && col < p_line->cols);
    if(!p_line->cc_free){
        tn_old_size=p_line->size;
        p_line->size += 16 + (p_line->size - p_line->cols) / 2;
        p_line->chars=sresize(p_line->chars, p_line->size, termchar);
        p_line->cc_free=tn_old_size;
        for(i=tn_old_size;i<p_line->size-1;i++){
            p_line->chars[i].cc_next=1;
        }
        p_line->chars[i].cc_next=0;
    }
    tn_free_x=p_line->cc_free;
    if(p_line->chars[tn_free_x].cc_next){
        p_line->cc_free=tn_free_x + p_line->chars[tn_free_x].cc_next;
    }
    else{
        p_line->cc_free=0;
    }
    p_line->chars[tn_free_x].cc_next=0;
    p_line->chars[tn_free_x].chr=chr;
    if(tn_free_x){
        tn_x=col;
        while(p_line->chars[tn_x].cc_next){
            tn_x += p_line->chars[tn_x].cc_next;
        }
        p_line->chars[tn_x].cc_next=tn_free_x - tn_x;
    }
}

void clear_cc(termline * p_line, int col){
    int tn_x;

    assert(col >= 0 && col < p_line->cols);
    if(!p_line->chars[col].cc_next){
        return;
    }
    if(p_line->cc_free){
        tn_x=col;
        while(p_line->chars[tn_x].cc_next){
            tn_x += p_line->chars[tn_x].cc_next;
        }
        p_line->chars[tn_x].cc_next=p_line->cc_free - tn_x;
    }
    p_line->cc_free=col + p_line->chars[col].cc_next;
    p_line->chars[col].cc_next=0;
}

int cmp_cc(termchar * p_char_a, termchar * p_char_b){
    while(p_char_a->cc_next || p_char_b->cc_next){
        if(!p_char_a->cc_next || !p_char_b->cc_next){
            return 0;
        }
        p_char_a += p_char_a->cc_next;
        p_char_b += p_char_b->cc_next;
        if(p_char_a->chr != p_char_b->chr){
            return 0;
        }
    }
    return 1;
}

void term_set_scrollback_size(Terminal * term, int n_save){
    int tn_shrink;
    int i;
    char * ts_line;

    if(!term->p_scrollback){
        term->p_scrollback=newtree234(NULL);
        term->n_scrollback_count=0;
    }
    term->n_scrollback_size=n_save;
    if(term->n_scrollback_count>term->n_scrollback_size){
        tn_shrink=term->n_scrollback_count-term->n_scrollback_size;
        for(i=0;i<tn_shrink;i++){
            ts_line=delpos234(term->p_scrollback, 0);
            term->n_scrollback_count--;
            sfree(ts_line);
        }
    }
}

void term_clrsb(Terminal * term){
    int tn_shrink;
    int i;
    char * ts_line;

    if(term->n_scrollback_count>0){
        tn_shrink=term->n_scrollback_count-0;
        for(i=0;i<tn_shrink;i++){
            ts_line=delpos234(term->p_scrollback, 0);
            term->n_scrollback_count--;
            sfree(ts_line);
        }
    }
    term->alt_sblines=0;
}

int sblines(Terminal * term){
    int tn_lines;

    tn_lines=term->n_scrollback_count;
    if(cfg.erase_to_scrollback && term->alt_which){
        tn_lines += term->alt_sblines;
    }
    return tn_lines;
}

void scroll(Terminal * term, int tn_top, int tn_bot, int tn_lines, int b_scrollback){
    termline * p_line;
    int j;
    int tn_shrink;
    int i;
    char * ts_line;

    if(tn_top != 0 || term->alt_which != 0){
        b_scrollback=FALSE;
    }
    if(tn_lines< 0){
        while(tn_lines< 0){
            p_line=delpos234(term->screen, tn_bot);
            resizeline(term, p_line, term->cols);
            p_line->lattr=LATTR_NORM;
            for(j=0;j<term->cols;j++){
                clear_cc(p_line, j);
                p_line->chars[j] = term->erase_char;
            }
            addpos234(term->screen, p_line, tn_top);
            tn_lines++;
        }
    }
    else{
        while(tn_lines> 0){
            p_line=delpos234(term->screen, tn_top);
            if(b_scrollback && term->n_scrollback_size > 0){
                if(term->n_scrollback_count>term->n_scrollback_size-1){
                    tn_shrink=term->n_scrollback_count-term->n_scrollback_size-1;
                    for(i=0;i<tn_shrink;i++){
                        ts_line=delpos234(term->p_scrollback, 0);
                        term->n_scrollback_count--;
                        sfree(ts_line);
                    }
                }
                fprintf(stdout, "    :[scrollback_shrink_to] term->n_scrollback_size=%d, term->n_scrollback_count=%d\n", term->n_scrollback_size, term->n_scrollback_count);
                addpos234(term->p_scrollback, compressline(p_line), term->n_scrollback_count);
                term->n_scrollback_count++;
                if(term->disptop > -term->n_scrollback_size && term->disptop < 0){
                    term->disptop--;
                }
            }
            resizeline(term, p_line, term->cols);
            p_line->lattr=LATTR_NORM;
            for(j=0;j<term->cols;j++){
                clear_cc(p_line, j);
                p_line->chars[j] = term->erase_char;
            }
            addpos234(term->screen, p_line, tn_bot);
            tn_lines--;
        }
    }
}

termline * lineptr(Terminal * term, int y){
    termline * p_line;
    tree234 * whichtree;
    int tn_idx;
    int altlines;
    void * cline;

    if(y >= 0){
        whichtree=term->screen;
        tn_idx=y;
    }
    else{
        altlines=0;
        if(cfg.erase_to_scrollback && term->alt_which){
            altlines=term->alt_sblines;
        }
        if(y < -altlines){
            whichtree=term->p_scrollback;
            tn_idx=y + altlines + count234(term->p_scrollback);
        }
        else{
            whichtree=term->alt_screen;
            tn_idx=y + term->alt_sblines;
        }
    }
    if(whichtree == term->p_scrollback){
        cline=index234(whichtree, tn_idx);
        p_line=decompressline(cline);
        p_line->b_temporary=1;
    }
    else{
        p_line=index234(whichtree, tn_idx);
        p_line->b_temporary=0;
    }
    resizeline(term, p_line, term->cols);
    return p_line;
}

void unlineptr(termline * p_line){
    if(p_line->b_temporary){
        if(p_line){
            sfree(p_line->chars);
            sfree(p_line);
        }
    }
}

void term_scroll(Terminal * term, int tn_rel, int tn_off){
    int tn_sbtop;
    int tn_scroll;

    if(tn_rel<0){
        term->disptop=tn_off;
    }
    else if(tn_rel>0){
        term->disptop=tn_off - sblines(term);
    }
    else if(tn_rel==0){
        term->disptop += tn_off;
    }
    tn_sbtop=-sblines(term);
    if(term->disptop < tn_sbtop){
        term->disptop=tn_sbtop;
    }
    if(term->disptop > 0){
        term->disptop=0;
    }
    tn_scroll=sblines(term);
    front_set_scrollbar(tn_scroll + term->rows, tn_scroll + term->disptop, term->rows);
    term_update(term);
}

int term_data(Terminal * term, char * ts_data, int tn_len){
    bufchain_add(&term->inbuf, ts_data, tn_len);
    if(!term->b_in_term_out && term->selstate != DRAGGING){
        term->b_in_term_out=TRUE;
        term_out(term);
        term->b_in_term_out=FALSE;
    }
    return 0;
}

int term_data_untrusted(Terminal * term, char * ts_data, int tn_len){
    int i;

    for(i=0;i<tn_len;i++){
        if(ts_data[i] == '\n'){
            term_data(term, "\r\n", 2);
        }
        else if(ts_data[i] & 0x60){
            term_data(term, ts_data + i, 1);
        }
    }
    return 0;
}

void term_out(Terminal * term){
    unsigned long c;
    int tn_unget= -1;
    int n_chars_in_buffer=0;
    unsigned char localbuf[256];
    unsigned char  * s_localbuf;
    void * tp_tmp_data;
    int tn_tmp_len;
    int tn_tmp_size;
    int tn_cset;
    termline * p_line;
    unsigned long tn_ticks;
    struct beep_node * p_beep_node_temp;
    pos old_curs;
    int tn_width;
    int tn_x;
    int n;
    int i;
    int j;
    int tn_count;
    int tn_shrink;
    char * ts_line;
    int tn_scroll;
    int tn_arg;
    int tn_r;
    int tn_g;
    int tn_b;
    int tn_color_idx;
    char ts_temp_buf[100];
    int tn_top;
    int tn_bot;
    unsigned int tn_max;
    int tn_val;
    int tn_a0;
    int tn_a1;
    int tn_a2;
    int tn_a3;

    while(1){
        if(tn_unget == -1){
            if(n_chars_in_buffer == 0){
                if(bufchain_size(&term->inbuf)>0){
                    bufchain_prefix(&term->inbuf, &tp_tmp_data, &n_chars_in_buffer);
                    if(n_chars_in_buffer > sizeof(localbuf)){
                        n_chars_in_buffer=sizeof(localbuf);
                    }
                    memcpy(localbuf, tp_tmp_data, n_chars_in_buffer);
                    bufchain_consume(&term->inbuf, n_chars_in_buffer);
                    s_localbuf=localbuf;
                }
                else{
                    break;
                }
            }
            c=*s_localbuf++;
            n_chars_in_buffer--;
        }
        else{
            c=tn_unget;
            tn_unget=-1;
        }
        if(term->n_termstate==termstate_PRINTING){
            if(c == '\033'){
                term->n_state_2=1;
            }
            else if(c == '[' && term->n_state_2 == 1){
                term->n_state_2=2;
            }
            else if(c == '4' && term->n_state_2 == 2){
                term->n_state_2=3;
            }
            else if(c == 'i' && term->n_state_2 == 3){
                tn_tmp_size=bufchain_size(&term->printer_buf);
                while(tn_tmp_size>0){
                    bufchain_prefix(&term->printer_buf, &tp_tmp_data, &tn_tmp_len);
                    printer_job_data(term->p_print_job, tp_tmp_data, tn_tmp_len);
                    bufchain_consume(&term->printer_buf, tn_tmp_len);
                    tn_tmp_size=bufchain_size(&term->printer_buf);
                }
                printer_finish_job(term->p_print_job);
                term->p_print_job=NULL;
                term->n_termstate=termstate_TOPLEVEL;
            }
            else{
                if(term->n_state_2>0){
                    bufchain_add(&term->printer_buf, "\033[4i", term->n_state_2);
                }
                bufchain_add(&term->printer_buf, &c, 1);
                term->n_state_2=0;
            }
            continue;
        }
        if(term->n_termstate == termstate_TOPLEVEL){
            if(line_codepage==CP_UTF8){
                if(term->n_utf_state==0){
                    if(c<0x80){
                        if(unitab_ctrl[c] != 0xFF){
                            c=unitab_ctrl[c];
                        }
                        else{
                            c=((unsigned char)c)|CSET_ASCII;
                        }
                    }
                    else{
                        if(c < 0x80){
                        }
                        else if((c & 0xe0) == 0xc0){
                            term->n_utf_size=term->n_utf_state = 1;
                            term->n_utf_char=(c & 0x1f);
                        }
                        else if((c & 0xf0) == 0xe0){
                            term->n_utf_size=term->n_utf_state = 2;
                            term->n_utf_char=(c & 0x0f);
                        }
                        else if((c & 0xf8) == 0xf0){
                            term->n_utf_size=term->n_utf_state = 3;
                            term->n_utf_char=(c & 0x07);
                        }
                        else if((c & 0xfc) == 0xf8){
                            term->n_utf_size=term->n_utf_state = 4;
                            term->n_utf_char=(c & 0x03);
                        }
                        else if((c & 0xfe) == 0xfc){
                            term->n_utf_size=term->n_utf_state = 5;
                            term->n_utf_char=(c & 0x01);
                        }
                        else{
                            c=UCSERR;
                        }
                        continue;
                    }
                }
                else{
                    if((c & 0xC0) != 0x80){
                        tn_unget=c;
                        c=UCSERR;
                        term->n_utf_state=0;
                    }
                    else{
                        term->n_utf_char=(term->n_utf_char << 6) | (c & 0x3f);
                        term->n_utf_state--;
                    }
                    if(term->n_utf_state>0){
                        continue;
                    }
                    else{
                        c=term->n_utf_char;
                        if(c < 0x80 || (c < 0x800 && term->n_utf_size >= 2) || (c < 0x10000 && term->n_utf_size >= 3) || (c < 0x200000 && term->n_utf_size >= 4) || (c < 0x4000000 && term->n_utf_size >= 5)){
                            c=UCSERR;
                        }
                        else if(c == 0x2028 || c == 0x2029){
                            c=0x85;
                        }
                        else if(c < 0xA0){
                            c=0xFFFD;
                        }
                        else if(c >= 0xD800 && c < 0xE000){
                            c=UCSERR;
                        }
                        else if(c > 0x10FFFF){
                            c=UCSERR;
                        }
                        else if(c >= 0xE0000 && c <= 0xE007F){
                            continue;
                        }
                        else if(c == 0xFEFF){
                            continue;
                        }
                        else if(c == 0xFFFE || c == 0xFFFF){
                            c=UCSERR;
                        }
                    }
                }
            }
            else if(term->sco_acs && (c!='\033' && c!='\012' && c!='\015' && c!='\b')){
                if(term->sco_acs == 2){
                    c |= 0x80;
                }
                c |= CSET_SCOACS;
            }
            else{
                tn_cset=term->cset_attr[term->cset];
                if(tn_cset==CSET_LINEDRW){
                    if(unitab_ctrl[c] != 0xFF){
                        c=unitab_ctrl[c];
                    }
                    else{
                        c=((unsigned char)c)|CSET_LINEDRW;
                    }
                }
                else if(tn_cset==CSET_GBCHR || tn_cset==CSET_ASCII){
                    if(tn_cset==CSET_GBCHR && c=='#'){
                        c='}' | CSET_LINEDRW;
                    }
                    else{
                        if(unitab_ctrl[c] != 0xFF){
                            c=unitab_ctrl[c];
                        }
                        else{
                            c=((unsigned char)c)|CSET_ASCII;
                        }
                    }
                }
                else if(tn_cset==CSET_SCOACS){
                    if(c>=' '){
                        c=((unsigned char)c) | CSET_SCOACS;
                    }
                }
            }
        }
        if(c=='\033'){
            term->n_termstate=termstate_SEEN_ESC;
        }
        else if((c&~0x1F)== 0||c=='\177' && term->n_termstate < termstate_DO_CTRLS){
            switch(c){
                case '\177':
                    if(term->curs.x && !term->wrapnext){
                        term->curs.x--;
                    }
                    term->wrapnext=FALSE;
                    if(!cfg.no_dbackspace){
                        check_boundary(term, term->curs.x, term->curs.y);
                        check_boundary(term, term->curs.x+1, term->curs.y);
                        p_line=index234(term->screen, term->curs.y);
                        clear_cc(p_line, term->curs.x);
                        p_line->chars[term->curs.x] = term->erase_char;
                    }
                    break;
                case '\005':
                    linedisc_send(term->id_string, strlen(term->id_string));
                    break;
                case '\007':
                    term->b_seen_disp_event=1;
                    if(!term->window_update_pending){
                        term->window_update_pending=1;
                        schedule_timer(UPDATE_DELAY, term_update_on_timer, term);
                    }
                    tn_ticks=GETTICKCOUNT();
                    if(!sl_beep.b_overloaded){
                        p_beep_node_temp=(struct beep_node *)malloc(sizeof(struct beep_node));
                        p_beep_node_temp->next=NULL;
                        if(!sl_beep.head){
                            sl_beep.head=p_beep_node_temp;
                        }
                        else{
                            sl_beep.tail->next=p_beep_node_temp;
                        }
                        sl_beep.tail=p_beep_node_temp;
                        sl_beep.n++;
                        p_beep_node_temp->n_ticks=tn_ticks;
                    }
                    while(sl_beep.head && sl_beep.head->n_ticks < tn_ticks - 2*TICKSPERSEC){
                        p_beep_node_temp=sl_beep.head;
                        sl_beep.head=sl_beep.head->next;
                        if(!sl_beep.head){
                            sl_beep.tail=NULL;
                        }
                        sl_beep.n--;
                        free(p_beep_node_temp);
                    }
                    if(sl_beep.b_overloaded && tn_ticks - sl_beep.n_last_tick >= 5*TICKSPERSEC){
                        sl_beep.b_overloaded=0;
                    }
                    else if(!sl_beep.b_overloaded && sl_beep.n >= 5){
                        sl_beep.b_overloaded=1;
                    }
                    sl_beep.n_last_tick=tn_ticks;
                    if(!sl_beep.b_overloaded){
                        do_beep();
                        if(cfg.beep == BELL_VISUAL){
                            if(!term->in_vbell){
                                term->in_vbell=1;
                                schedule_timer(VBELL_DELAY, term_vbell_on_timer, term);
                            }
                        }
                    }
                    break;
                case '\010':
                    term->b_seen_disp_event=1;
                    if(!term->window_update_pending){
                        term->window_update_pending=1;
                        schedule_timer(UPDATE_DELAY, term_update_on_timer, term);
                    }
                    if(term->curs.x == 0 && (term->curs.y == 0 || term->wrap == 0)){
                    }
                    else if(term->curs.x == 0 && term->curs.y > 0){
                        term->curs.x=term->cols - 1;
                        term->curs.y--;
                    }
                    else if(term->wrapnext){
                        term->wrapnext=FALSE;
                    }
                    else{
                        term->curs.x--;
                    }
                    break;
                case '\011':
                    term->b_seen_disp_event=1;
                    if(!term->window_update_pending){
                        term->window_update_pending=1;
                        schedule_timer(UPDATE_DELAY, term_update_on_timer, term);
                    }
                    old_curs=term->curs;
                    p_line=index234(term->screen, term->curs.y);
                    do{
                        term->curs.x++;
                    }while(term->curs.x<term->cols-1 && !term->pc_tabs[term->curs.x]);
                    if((p_line->lattr & LATTR_MODE) != LATTR_NORM){
                        if(term->curs.x >= term->cols / 2){
                            term->curs.x=term->cols / 2 - 1;
                        }
                    }
                    else{
                        if(term->curs.x >= term->cols){
                            term->curs.x=term->cols - 1;
                        }
                    }
                    break;
                case '\012':
                    term->b_seen_disp_event=1;
                    if(!term->window_update_pending){
                        term->window_update_pending=1;
                        schedule_timer(UPDATE_DELAY, term_update_on_timer, term);
                    }
                    if(term->curs.y == term->marg_b){
                        scroll(term, term->marg_t, term->marg_b, 1, TRUE);
                    }
                    else if(term->curs.y < term->rows - 1){
                        term->curs.y++;
                    }
                    term->wrapnext=FALSE;
                    if(cfg.lfhascr){
                        term->curs.x=0;
                        term->wrapnext=FALSE;
                    }
                    if(term->logctx){
                        logtraffic(term->logctx, (unsigned char) c, LGTYP_ASCII);
                    }
                    break;
                case '\013':
                    term->b_seen_disp_event=1;
                    if(!term->window_update_pending){
                        term->window_update_pending=1;
                        schedule_timer(UPDATE_DELAY, term_update_on_timer, term);
                    }
                    if(term->curs.y == term->marg_b){
                        scroll(term, term->marg_t, term->marg_b, 1, TRUE);
                    }
                    else if(term->curs.y < term->rows - 1){
                        term->curs.y++;
                    }
                    term->wrapnext=FALSE;
                    if(cfg.lfhascr){
                        term->curs.x=0;
                        term->wrapnext=FALSE;
                    }
                    if(term->logctx){
                        logtraffic(term->logctx, (unsigned char) c, LGTYP_ASCII);
                    }
                    break;
                case '\014':
                    term->b_seen_disp_event=1;
                    if(!term->window_update_pending){
                        term->window_update_pending=1;
                        schedule_timer(UPDATE_DELAY, term_update_on_timer, term);
                    }
                    if(term->curs.y == term->marg_b){
                        scroll(term, term->marg_t, term->marg_b, 1, TRUE);
                    }
                    else if(term->curs.y < term->rows - 1){
                        term->curs.y++;
                    }
                    term->wrapnext=FALSE;
                    if(cfg.lfhascr){
                        term->curs.x=0;
                        term->wrapnext=FALSE;
                    }
                    if(term->logctx){
                        logtraffic(term->logctx, (unsigned char) c, LGTYP_ASCII);
                    }
                    break;
                case '\015':
                    term->b_seen_disp_event=1;
                    if(!term->window_update_pending){
                        term->window_update_pending=1;
                        schedule_timer(UPDATE_DELAY, term_update_on_timer, term);
                    }
                    if(cfg.crhaslf){
                        if(term->curs.y == term->marg_b){
                            scroll(term, term->marg_t, term->marg_b, 1, TRUE);
                        }
                        else if(term->curs.y < term->rows - 1){
                            term->curs.y++;
                        }
                        term->wrapnext=FALSE;
                    }
                    term->curs.x=0;
                    term->wrapnext=FALSE;
                    if(term->logctx){
                        logtraffic(term->logctx, (unsigned char) c, LGTYP_ASCII);
                    }
                    break;
                case '\016':
                    term->cset=1;
                    break;
                case '\017':
                    term->cset=0;
                    break;
                case '\033':
                default:
                    term->n_termstate=termstate_TOPLEVEL;
                    break;
            }
        }
        else if(term->n_termstate == termstate_TOPLEVEL){
            p_line=index234(term->screen, term->curs.y);
            tn_width=0;
            if(DIRECT_CHAR(c)){
                tn_width=1;
            }
            else{
                tn_width=(cfg.cjk_ambig_wide ?  mk_wcwidth_cjk((wchar_t) c) : mk_wcwidth((wchar_t) c));
            }
            if(term->wrapnext && term->wrap && tn_width > 0){
                p_line->lattr |= LATTR_WRAPPED;
                if(term->curs.y == term->marg_b){
                    scroll(term, term->marg_t, term->marg_b, 1, TRUE);
                }
                else if(term->curs.y < term->rows - 1){
                    term->curs.y++;
                }
                term->wrapnext=FALSE;
                term->curs.x=0;
                term->wrapnext=FALSE;
                term->wrapnext=0;
                p_line=index234(term->screen, term->curs.y);
            }
            if(term->insert && tn_width > 0){
                insch(term, tn_width);
            }
            if(((c & CSET_MASK) == CSET_ASCII || (c & CSET_MASK) == 0) && term->logctx){
                logtraffic(term->logctx, (unsigned char) c, LGTYP_ASCII);
            }
            if(tn_width==0){
                if(term->curs.x > 0){
                    tn_x=term->curs.x - 1;
                    if(term->wrapnext){
                        tn_x++;
                    }
                    if(p_line->chars[tn_x].chr == UCSWIDE){
                        assert(tn_x > 0);
                        tn_x--;
                    }
                    add_cc(p_line, tn_x, c);
                    term->b_seen_disp_event=1;
                    if(!term->window_update_pending){
                        term->window_update_pending=1;
                        schedule_timer(UPDATE_DELAY, term_update_on_timer, term);
                    }
                }
                continue;
            }
            else{
                if(tn_width==2){
                    check_boundary(term, term->curs.x, term->curs.y);
                    check_boundary(term, term->curs.x+2, term->curs.y);
                    if(term->curs.x == term->cols-1){
                        clear_cc(p_line, term->curs.x);
                        p_line->chars[term->curs.x] = term->erase_char;
                        p_line->lattr |= LATTR_WRAPPED;
                        if(term->curs.y == term->marg_b){
                            scroll(term, term->marg_t, term->marg_b, 1, TRUE);
                        }
                        else if(term->curs.y < term->rows - 1){
                            term->curs.y++;
                        }
                        term->wrapnext=FALSE;
                        term->curs.x=0;
                        term->wrapnext=FALSE;
                        term->wrapnext=0;
                        p_line=index234(term->screen, term->curs.y);
                        p_line->lattr |= LATTR_WRAPPED2;
                        check_boundary(term, term->curs.x, term->curs.y);
                        check_boundary(term, term->curs.x+2, term->curs.y);
                    }
                    clear_cc(p_line, term->curs.x);
                    p_line->chars[term->curs.x].chr=c;
                    p_line->chars[term->curs.x].attr=term->curr_attr;
                    term->curs.x++;
                    clear_cc(p_line, term->curs.x);
                    p_line->chars[term->curs.x].chr=UCSWIDE;
                    p_line->chars[term->curs.x].attr=term->curr_attr;
                }
                else if(tn_width==1){
                    check_boundary(term, term->curs.x, term->curs.y);
                    check_boundary(term, term->curs.x+1, term->curs.y);
                    clear_cc(p_line, term->curs.x);
                    p_line->chars[term->curs.x].chr=c;
                    p_line->chars[term->curs.x].attr=term->curr_attr;
                }
                term->curs.x++;
                if(term->curs.x == term->cols){
                    term->curs.x--;
                    term->wrapnext=TRUE;
                }
                term->b_seen_disp_event=1;
                if(!term->window_update_pending){
                    term->window_update_pending=1;
                    schedule_timer(UPDATE_DELAY, term_update_on_timer, term);
                }
            }
        }
        else if(term->n_termstate==termstate_SEEN_ESC){
            term->n_termstate=termstate_TOPLEVEL;
            if(c >= ' ' && c <= '/'){
                term->n_state_2=c;
                term->n_termstate=termstate_SEEN_QUERY;
            }
            else if(c=='['){
                term->n_termstate=termstate_SEEN_CSI;
                term->n_state_2=0;
                term->n_esc_args=1;
                term->pn_esc_args[0] = 0;
                term->pn_esc_args[1] = 0;
            }
            else if(c==']'){
                term->n_termstate=termstate_SEEN_OSC;
                term->n_state_2=0;
                term->pn_esc_args[0] = 0;
                term->pn_esc_args[1] = 0;
                term->n_osc_strlen=0;
            }
            else{
                switch(c){
                    case '7':
                        term->save_curs=term->curs;
                        term->save_curr_attr=term->curr_attr;
                        term->save_wrapnext=term->wrapnext;
                        term->save_cset=term->cset;
                        term->save_utf=term->utf;
                        term->save_sco_acs=term->sco_acs;
                        term->save_csattr=term->cset_attr[term->cset];
                        break;
                    case '8':
                        term->b_seen_disp_event=1;
                        if(!term->window_update_pending){
                            term->window_update_pending=1;
                            schedule_timer(UPDATE_DELAY, term_update_on_timer, term);
                        }
                        term->curs=term->save_curs;
                        term->curr_attr=term->save_curr_attr;
                        term->wrapnext=term->save_wrapnext;
                        term->cset=term->save_cset;
                        term->utf=term->save_utf;
                        term->sco_acs=term->save_sco_acs;
                        term->cset_attr[term->cset] = term->save_csattr;
                        if(term->curs.x<0){
                            term->curs.x=0;
                        }
                        else if(term->curs.x>term->cols-1){
                            term->curs.x=term->cols-1;
                        }
                        if(term->curs.y<0){
                            term->curs.y=0;
                        }
                        else if(term->curs.y>term->rows-1){
                            term->curs.y=term->rows-1;
                        }
                        if(term->wrapnext && term->curs.x < term->cols-1){
                            term->wrapnext=0;
                        }
                        break;
                    case 'D':
                        term->b_seen_disp_event=1;
                        if(!term->window_update_pending){
                            term->window_update_pending=1;
                            schedule_timer(UPDATE_DELAY, term_update_on_timer, term);
                        }
                        if(term->curs.y == term->marg_b){
                            scroll(term, term->marg_t, term->marg_b, 1, TRUE);
                        }
                        else if(term->curs.y < term->rows - 1){
                            term->curs.y++;
                        }
                        term->wrapnext=FALSE;
                        break;
                    case 'E':
                        term->b_seen_disp_event=1;
                        if(!term->window_update_pending){
                            term->window_update_pending=1;
                            schedule_timer(UPDATE_DELAY, term_update_on_timer, term);
                        }
                        if(term->curs.y == term->marg_b){
                            scroll(term, term->marg_t, term->marg_b, 1, TRUE);
                        }
                        else if(term->curs.y < term->rows - 1){
                            term->curs.y++;
                        }
                        term->wrapnext=FALSE;
                        term->curs.x=0;
                        term->wrapnext=FALSE;
                        break;
                    case 'M':
                        term->b_seen_disp_event=1;
                        if(!term->window_update_pending){
                            term->window_update_pending=1;
                            schedule_timer(UPDATE_DELAY, term_update_on_timer, term);
                        }
                        if(term->curs.y == term->marg_t){
                            scroll(term, term->marg_t, term->marg_b, -1, TRUE);
                        }
                        else if(term->curs.y > 0){
                            term->curs.y--;
                        }
                        term->wrapnext=FALSE;
                        break;
                    case 'Z':
                        linedisc_send(term->id_string, strlen(term->id_string));
                        break;
                    case 'c':
                        term->b_seen_disp_event=1;
                        if(!term->window_update_pending){
                            term->window_update_pending=1;
                            schedule_timer(UPDATE_DELAY, term_update_on_timer, term);
                        }
                        power_on(term, TRUE);
                        break;
                    case 'H':
                        term->pc_tabs[term->curs.x]=1;
                        break;
                }
            }
        }
        else if(term->n_termstate==termstate_SEEN_CSI){
            if(isdigit(c)){
                if(term->n_esc_args <= ARGS_MAX){
                    term->pn_esc_args[term->n_esc_args - 1] = 10 * term->pn_esc_args[term->n_esc_args - 1] + c - '0';
                }
            }
            else if(c == ';'){
                if(term->n_esc_args < ARGS_MAX){
                    term->pn_esc_args[term->n_esc_args++] = 0;
                }
            }
            else if(term->n_state_2==0){
                if(c<'@'){
                    term->n_state_2=c;
                }
                else{
                    term->n_termstate=termstate_TOPLEVEL;
                    switch(c){
                        case 'A':
                            if(!term->pn_esc_args[0]){
                                term->pn_esc_args[0]=1;
                            }
                            move_cursor(term, term->curs.x, term->curs.y - term->pn_esc_args[0], 1);
                            term->b_seen_disp_event=1;
                            if(!term->window_update_pending){
                                term->window_update_pending=1;
                                schedule_timer(UPDATE_DELAY, term_update_on_timer, term);
                            }
                            break;
                        case 'B':
                            if(!term->pn_esc_args[0]){
                                term->pn_esc_args[0]=1;
                            }
                            move_cursor(term, term->curs.x, term->curs.y + term->pn_esc_args[0], 1);
                            term->b_seen_disp_event=1;
                            if(!term->window_update_pending){
                                term->window_update_pending=1;
                                schedule_timer(UPDATE_DELAY, term_update_on_timer, term);
                            }
                            break;
                        case 'C':
                            if(!term->pn_esc_args[0]){
                                term->pn_esc_args[0]=1;
                            }
                            move_cursor(term, term->curs.x + term->pn_esc_args[0], term->curs.y, 1);
                            term->b_seen_disp_event=1;
                            if(!term->window_update_pending){
                                term->window_update_pending=1;
                                schedule_timer(UPDATE_DELAY, term_update_on_timer, term);
                            }
                            break;
                        case 'D':
                            if(!term->pn_esc_args[0]){
                                term->pn_esc_args[0]=1;
                            }
                            move_cursor(term, term->curs.x - term->pn_esc_args[0], term->curs.y, 1);
                            term->b_seen_disp_event=1;
                            if(!term->window_update_pending){
                                term->window_update_pending=1;
                                schedule_timer(UPDATE_DELAY, term_update_on_timer, term);
                            }
                            break;
                        case 'E':
                            if(!term->pn_esc_args[0]){
                                term->pn_esc_args[0]=1;
                            }
                            move_cursor(term, 0, term->curs.y + term->pn_esc_args[0], 1);
                            term->b_seen_disp_event=1;
                            if(!term->window_update_pending){
                                term->window_update_pending=1;
                                schedule_timer(UPDATE_DELAY, term_update_on_timer, term);
                            }
                            break;
                        case 'F':
                            if(!term->pn_esc_args[0]){
                                term->pn_esc_args[0]=1;
                            }
                            move_cursor(term, 0, term->curs.y - term->pn_esc_args[0], 1);
                            term->b_seen_disp_event=1;
                            if(!term->window_update_pending){
                                term->window_update_pending=1;
                                schedule_timer(UPDATE_DELAY, term_update_on_timer, term);
                            }
                            break;
                        case 'G':
                            if(!term->pn_esc_args[0]){
                                term->pn_esc_args[0]=1;
                            }
                            move_cursor(term, term->pn_esc_args[0] - 1, term->curs.y, 0);
                            term->b_seen_disp_event=1;
                            if(!term->window_update_pending){
                                term->window_update_pending=1;
                                schedule_timer(UPDATE_DELAY, term_update_on_timer, term);
                            }
                            break;
                        case 'H':
                            if(!term->pn_esc_args[0]){
                                term->pn_esc_args[0]=1;
                            }
                            if(!term->pn_esc_args[1]){
                                term->pn_esc_args[1]=1;
                            }
                            move_cursor(term, term->pn_esc_args[1] - 1, ((term->dec_om ? term->marg_t : 0) + term->pn_esc_args[0] - 1), (term->dec_om ? 2 : 0));
                            term->b_seen_disp_event=1;
                            if(!term->window_update_pending){
                                term->window_update_pending=1;
                                schedule_timer(UPDATE_DELAY, term_update_on_timer, term);
                            }
                            break;
                        case 'J':
                            if(!term->pn_esc_args[0]){
                                term->pn_esc_args[0]=0;
                            }
                            n=term->pn_esc_args[0];
                            fprintf(stdout, "    :[term_do_ED] n=%d\n", n);
                            if(n==0){
                                term->disptop=0;
                                for(i=term->curs.y+1;i<term->rows;i++){
                                    p_line=index234(term->screen, i);
                                    p_line->lattr=LATTR_NORM;
                                    for(j=0;j<term->cols;j++){
                                        clear_cc(p_line, j);
                                        p_line->chars[j] = term->erase_char;
                                    }
                                }
                                check_boundary(term, term->curs.x, term->curs.y);
                                p_line=index234(term->screen, term->curs.y);
                                p_line->lattr &= ~(LATTR_WRAPPED | LATTR_WRAPPED2);
                                for(j=term->curs.x;j<term->cols;j++){
                                    clear_cc(p_line, j);
                                    p_line->chars[j] = term->erase_char;
                                }
                            }
                            else if(n==1){
                                term->disptop=0;
                                term_invalidate(term);
                                for(i=0;i<term->curs.y;i++){
                                    p_line=index234(term->screen, i);
                                    p_line->lattr=LATTR_NORM;
                                    for(j=0;j<term->cols;j++){
                                        clear_cc(p_line, j);
                                        p_line->chars[j] = term->erase_char;
                                    }
                                }
                                check_boundary(term, term->curs.x, term->curs.y);
                                p_line=index234(term->screen, term->curs.y);
                                p_line->lattr=LATTR_NORM;
                                for(j=0;j<term->curs.x+1;j++){
                                    clear_cc(p_line, j);
                                    p_line->chars[j] = term->erase_char;
                                }
                            }
                            else if(n==2){
                                term->disptop=0;
                                term_invalidate(term);
                                tn_count=find_last_nonempty_line(term, term->screen)+1;
                                fprintf(stdout, "    :[erase_screen_2] tn_count=%d\n", tn_count);
                                scroll(term, 0, tn_count-1, tn_count, 1);
                            }
                            else if(n==3){
                                term->disptop=0;
                                term_invalidate(term);
                                for(i=0;i<term->rows;i++){
                                    p_line=index234(term->screen, i);
                                    p_line->lattr=LATTR_NORM;
                                    for(j=0;j<term->cols;j++){
                                        clear_cc(p_line, j);
                                        p_line->chars[j] = term->erase_char;
                                    }
                                }
                                if(term->n_scrollback_count>0){
                                    tn_shrink=term->n_scrollback_count-0;
                                    for(i=0;i<tn_shrink;i++){
                                        ts_line=delpos234(term->p_scrollback, 0);
                                        term->n_scrollback_count--;
                                        sfree(ts_line);
                                    }
                                }
                                tn_scroll=sblines(term);
                                front_set_scrollbar(tn_scroll + term->rows, tn_scroll + term->disptop, term->rows);
                            }
                            term->b_seen_disp_event=1;
                            if(!term->window_update_pending){
                                term->window_update_pending=1;
                                schedule_timer(UPDATE_DELAY, term_update_on_timer, term);
                            }
                            break;
                        case 'K':
                            if(!term->pn_esc_args[0]){
                                term->pn_esc_args[0]=0;
                            }
                            n=term->pn_esc_args[0];
                            if(n==0){
                                check_boundary(term, term->curs.x, term->curs.y);
                                p_line=index234(term->screen, term->curs.y);
                                p_line->lattr &= ~(LATTR_WRAPPED | LATTR_WRAPPED2);
                                for(j=term->curs.x;j<term->cols;j++){
                                    clear_cc(p_line, j);
                                    p_line->chars[j] = term->erase_char;
                                }
                            }
                            else if(n==1){
                                check_boundary(term, term->curs.x, term->curs.y);
                                p_line=index234(term->screen, term->curs.y);
                                p_line->lattr=LATTR_NORM;
                                for(j=0;j<term->curs.x+1;j++){
                                    clear_cc(p_line, j);
                                    p_line->chars[j] = term->erase_char;
                                }
                            }
                            else if(n==2){
                                p_line=index234(term->screen, term->curs.y);
                                p_line->lattr=LATTR_NORM;
                                for(j=0;j<term->cols;j++){
                                    clear_cc(p_line, j);
                                    p_line->chars[j] = term->erase_char;
                                }
                            }
                            term->b_seen_disp_event=1;
                            if(!term->window_update_pending){
                                term->window_update_pending=1;
                                schedule_timer(UPDATE_DELAY, term_update_on_timer, term);
                            }
                            break;
                        case 'c':
                            linedisc_send(term->id_string, strlen(term->id_string));
                            break;
                        case 'd':
                            if(!term->pn_esc_args[0]){
                                term->pn_esc_args[0]=1;
                            }
                            move_cursor(term, term->curs.x, ((term->dec_om ? term->marg_t : 0) + term->pn_esc_args[0] - 1), (term->dec_om ? 2 : 0));
                            term->b_seen_disp_event=1;
                            if(!term->window_update_pending){
                                term->window_update_pending=1;
                                schedule_timer(UPDATE_DELAY, term_update_on_timer, term);
                            }
                            break;
                        case 'f':
                            if(!term->pn_esc_args[0]){
                                term->pn_esc_args[0]=1;
                            }
                            if(!term->pn_esc_args[1]){
                                term->pn_esc_args[1]=1;
                            }
                            move_cursor(term, term->pn_esc_args[1] - 1, ((term->dec_om ? term->marg_t : 0) + term->pn_esc_args[0] - 1), (term->dec_om ? 2 : 0));
                            term->b_seen_disp_event=1;
                            if(!term->window_update_pending){
                                term->window_update_pending=1;
                                schedule_timer(UPDATE_DELAY, term_update_on_timer, term);
                            }
                            break;
                        case 'g':
                            if(term->pn_esc_args[0]==0){
                                term->pc_tabs[term->curs.x]=0;
                            }
                            else if(term->pn_esc_args[0]==3){
                                for(i=0;i<term->cols;i++){
                                    term->pc_tabs[i]=0;
                                }
                            }
                            break;
                        case 'h':
                            for(i=0;i<term->n_esc_args;i++){
                                tn_arg=term->pn_esc_args[i];
                                if(term->pn_esc_args[i]==4){
                                    term->b_insert=1;
                                }
                                else if(term->pn_esc_args[i]==12){
                                    term->b_echo=0;
                                }
                                else if(term->pn_esc_args[i]==20){
                                    term->b_crlf=1;
                                }
                                else if(term->pn_esc_args[i]==34){
                                    term->b_big_cursor=0;
                                }
                            }
                            break;
                        case 'i':
                            if(term->n_esc_args==1){
                                if(term->pn_esc_args[0]==5 && cfg.printer){
                                    bufchain_clear(&term->printer_buf);
                                    term->p_print_job=printer_start_job(cfg.printer);
                                    term->n_state_2=0;
                                    term->n_termstate=termstate_PRINTING;
                                }
                            }
                            break;
                        case 'l':
                            for(i=0;i<term->n_esc_args;i++){
                                tn_arg=term->pn_esc_args[i];
                                if(term->pn_esc_args[i]==4){
                                    term->b_insert=0;
                                }
                                else if(term->pn_esc_args[i]==12){
                                    term->b_echo=1;
                                }
                                else if(term->pn_esc_args[i]==20){
                                    term->b_crlf=0;
                                }
                                else if(term->pn_esc_args[i]==34){
                                    term->b_big_cursor=1;
                                }
                            }
                            break;
                        case 'm':
                            if(term->n_esc_args==0){
                                term->n_esc_args=1;
                            }
                            for(i=0;i<term->n_esc_args;i++){
                                n=term->pn_esc_args[i];
                                if(n>=30 && n<38){
                                    term->curr_attr &= ~ATTR_FGMASK;
                                    term->curr_attr |= (n-30);
                                }
                                else if(n==39){
                                    term->curr_attr &= ~ATTR_FGMASK;
                                    term->curr_attr |= (COLOR_DEFFG);
                                }
                                else if(n>=40 && n<48){
                                    term->curr_attr &= ~ATTR_BGMASK;
                                    term->curr_attr |= (n-40)<<ATTR_BGSHIFT;
                                }
                                else if(n==49){
                                    term->curr_attr &= ~ATTR_BGMASK;
                                    term->curr_attr |= (COLOR_DEFBG)<<ATTR_BGSHIFT;
                                }
                                else if(n==38 || n==48){
                                    if(term->n_esc_args>2){
                                        if(term->pn_esc_args[i+1]==5){
                                            if(n==38){
                                                term->curr_attr &= ~ATTR_FGMASK;
                                                term->curr_attr |= (term->pn_esc_args[i+2] & 0xFF);
                                            }
                                            else{
                                                term->curr_attr &= ~ATTR_BGMASK;
                                                term->curr_attr |= (term->pn_esc_args[i+2] & 0xFF)<<ATTR_BGSHIFT;
                                            }
                                        }
                                        else if(term->pn_esc_args[i+1]==2 && term->n_esc_args>5){
                                            tn_r=((term->pn_esc_args[i+2]&0xff)-55+20)/40;
                                            if(tn_r<0){
                                                tn_r=0;
                                            }
                                            if(tn_r>5){
                                                tn_r=5;
                                            }
                                            tn_g=((term->pn_esc_args[i+3]&0xff)-55+20)/40;
                                            if(tn_g<0){
                                                tn_g=0;
                                            }
                                            if(tn_g>5){
                                                tn_g=5;
                                            }
                                            tn_b=((term->pn_esc_args[i+4]&0xff)-55+20)/40;
                                            if(tn_b<0){
                                                tn_b=0;
                                            }
                                            if(tn_b>5){
                                                tn_b=5;
                                            }
                                            tn_color_idx=tn_r*36+tn_g*6+tn_b;
                                            if(n==38){
                                                term->curr_attr &= ~ATTR_FGMASK;
                                                term->curr_attr |= (tn_color_idx);
                                            }
                                            else{
                                                term->curr_attr &= ~ATTR_BGMASK;
                                                term->curr_attr |= (tn_color_idx)<<ATTR_BGSHIFT;
                                            }
                                        }
                                    }
                                    i=term->n_esc_args;
                                }
                                else if(n==0){
                                    term->curr_attr=term->default_attr;
                                }
                                else if(n==1){
                                    term->curr_attr |= ATTR_BOLD;
                                }
                                else if(n==4){
                                    term->curr_attr |= ATTR_UNDER;
                                }
                                else if(n==5){
                                    term->curr_attr |= ATTR_BLINK;
                                }
                                else if(n==7){
                                    term->curr_attr |= ATTR_REVERSE;
                                }
                                else if(n==22){
                                    term->curr_attr &= ~ATTR_BOLD;
                                }
                                else if(n==24){
                                    term->curr_attr &= ~ATTR_UNDER;
                                }
                                else if(n==25){
                                    term->curr_attr &= ~ATTR_BLINK;
                                }
                                else if(n==27){
                                    term->curr_attr &= ~ATTR_REVERSE;
                                }
                            }
                            break;
                        case 'n':
                            if(term->pn_esc_args[0]==6){
                                n=snprintf(ts_temp_buf, 100, "\033[%d;%dR", term->curs.y + 1, term->curs.x + 1);
                                linedisc_send(ts_temp_buf, n);
                            }
                            else{
                                linedisc_send("\033[0n", 4);
                            }
                            break;
                        case 'r':
                            if(!term->pn_esc_args[0]){
                                term->pn_esc_args[0]=1;
                            }
                            if(!term->pn_esc_args[1]){
                                term->pn_esc_args[1]=term->rows;
                            }
                            tn_top=term->pn_esc_args[0] - 1;
                            tn_bot=term->pn_esc_args[1] - 1;
                            if(tn_bot >= term->rows){
                                tn_bot=term->rows - 1;
                            }
                            if(tn_bot - tn_top > 0){
                                term->marg_t=tn_top;
                                term->marg_b=tn_bot;
                                term->curs.x=0;
                                term->curs.y=tn_top;
                                term->b_seen_disp_event=1;
                                if(!term->window_update_pending){
                                    term->window_update_pending=1;
                                    schedule_timer(UPDATE_DELAY, term_update_on_timer, term);
                                }
                            }
                            break;
                        case 's':
                            term->save_curs=term->curs;
                            term->save_curr_attr=term->curr_attr;
                            term->save_wrapnext=term->wrapnext;
                            term->save_cset=term->cset;
                            term->save_utf=term->utf;
                            term->save_sco_acs=term->sco_acs;
                            term->save_csattr=term->cset_attr[term->cset];
                            break;
                        case 'u':
                            term->b_seen_disp_event=1;
                            if(!term->window_update_pending){
                                term->window_update_pending=1;
                                schedule_timer(UPDATE_DELAY, term_update_on_timer, term);
                            }
                            term->curs=term->save_curs;
                            term->curr_attr=term->save_curr_attr;
                            term->wrapnext=term->save_wrapnext;
                            term->cset=term->save_cset;
                            term->utf=term->save_utf;
                            term->sco_acs=term->save_sco_acs;
                            term->cset_attr[term->cset] = term->save_csattr;
                            if(term->curs.x<0){
                                term->curs.x=0;
                            }
                            else if(term->curs.x>term->cols-1){
                                term->curs.x=term->cols-1;
                            }
                            if(term->curs.y<0){
                                term->curs.y=0;
                            }
                            else if(term->curs.y>term->rows-1){
                                term->curs.y=term->rows-1;
                            }
                            if(term->wrapnext && term->curs.x < term->cols-1){
                                term->wrapnext=0;
                            }
                            break;
                    }
                }
            }
            else{
                term->n_termstate=termstate_TOPLEVEL;
                if(term->n_state_2=='?' && c=='h'){
                    for(i=0;i<term->n_esc_args;i++){
                        tn_arg=term->pn_esc_args[i];
                        if(term->pn_esc_args[i]==1){
                            term->b_app_cursor_keys=1;
                        }
                        else if(term->pn_esc_args[i]==5){
                            term->b_rvideo=1;
                        }
                        else if(term->pn_esc_args[i]==12){
                        }
                        else if(term->pn_esc_args[i]==25){
                            term->b_cursor_on=1;
                        }
                        else if(term->pn_esc_args[i]==47){
                        }
                        else if(term->pn_esc_args[i]==1049){
                        }
                    }
                }
                if(term->n_state_2=='?' && c=='l'){
                    for(i=0;i<term->n_esc_args;i++){
                        tn_arg=term->pn_esc_args[i];
                        if(term->pn_esc_args[i]==1){
                            term->b_app_cursor_keys=0;
                        }
                        else if(term->pn_esc_args[i]==5){
                            term->b_rvideo=0;
                        }
                        else if(term->pn_esc_args[i]==12){
                        }
                        else if(term->pn_esc_args[i]==25){
                            term->b_cursor_on=0;
                        }
                        else if(term->pn_esc_args[i]==47){
                        }
                        else if(term->pn_esc_args[i]==1049){
                        }
                    }
                }
            }
        }
        else if(term->n_termstate==termstate_SEEN_OSC){
            if(term->n_state_2==0){
                if(isdigit(c)){
                    term->pn_esc_args[0] = 10 * term->pn_esc_args[0] + c - '0';
                }
                else if(c=='P'){
                    term->n_state_2='P';
                }
                else if(c=='R'){
                    term->n_termstate=termstate_TOPLEVEL;
                }
                else if(c=='W'){
                    term->pn_esc_args[1]='W';
                }
                else if(c==';'){
                    term->n_state_2=1;
                }
                else{
                    term->n_termstate=termstate_TOPLEVEL;
                }
            }
            else if(term->n_state_2==1){
                if(c=='\012' || c=='\015'){
                    term->n_termstate=termstate_TOPLEVEL;
                }
                else if(c==0234 || c=='\007'){
                    term->n_termstate=termstate_TOPLEVEL;
                }
                else if(c>=' '){
                    if(term->n_osc_strlen<OSC_STR_MAX){
                        term->s_osc_str[term->n_osc_strlen++]=(wchar_t)c;
                    }
                }
                else{
                    term->n_termstate=termstate_TOPLEVEL;
                }
            }
            else if(term->n_state_2=='P'){
                tn_max=(term->n_osc_strlen == 0 ? 21 : 15);
                if(c>='0' && c<='9'){
                    tn_val=c - '0';
                }
                else if(c>='A' && c <= 'A' + tn_max - 10){
                    tn_val=c - 'A' + 10;
                }
                else if(c>='a' && c <= 'a' + tn_max - 10){
                    tn_val=c - 'a' + 10;
                }
                else{
                    term->n_termstate=termstate_TOPLEVEL;
                }
                if(term->n_termstate != termstate_TOPLEVEL){
                    term->s_osc_str[term->n_osc_strlen++]=tn_val;
                    if(term->n_osc_strlen >= 7){
                        tn_a0=term->s_osc_str[0];
                        tn_a1=term->s_osc_str[1]*16+term->s_osc_str[2];
                        tn_a2=term->s_osc_str[2]*16+term->s_osc_str[4];
                        tn_a3=term->s_osc_str[3]*16+term->s_osc_str[6];
                        term_invalidate(term);
                        term->n_termstate=termstate_TOPLEVEL;
                    }
                }
            }
        }
        else{
            term->n_termstate=termstate_TOPLEVEL;
        }
    }
    if(term->n_termstate==termstate_PRINTING){
        tn_tmp_size=bufchain_size(&term->printer_buf);
        while(tn_tmp_size>0){
            bufchain_prefix(&term->printer_buf, &tp_tmp_data, &tn_tmp_len);
            printer_job_data(term->p_print_job, tp_tmp_data, tn_tmp_len);
            bufchain_consume(&term->printer_buf, tn_tmp_len);
            tn_tmp_size=bufchain_size(&term->printer_buf);
        }
    }
    if(cfg.logflush && term->logctx){
        logflush(term->logctx);
    }
}

void term_invalidate(Terminal * term){
    int i;
    int j;

    for(i=0;i<term->rows;i++){
        for(j=0;j<term->cols;j++){
            term->disptext[i]->chars[j].attr |=ATTR_INVALID;
        }
    }
    if(!term->window_update_pending){
        term->window_update_pending=1;
        schedule_timer(UPDATE_DELAY, term_update_on_timer, term);
    }
}

void term_invalidate_rect(Terminal * term, int left, int top, int right, int bottom){
    int i;
    int j;

    if(left < 0){
        left=0;
    }
    if(top < 0){
        top=0;
    }
    if(right >= term->cols){
        right=term->cols-1;
    }
    if(bottom >= term->rows){
        bottom=term->rows-1;
    }
    for(i=top;i<bottom+1;i++){
        if((term->disptext[i]->lattr & LATTR_MODE) == LATTR_NORM){
            for(j=left;j<right+1;j++){
                term->disptext[i]->chars[j].attr |= ATTR_INVALID;
            }
        }
        else{
            for(j=left/2;j<right/2+1;j++){
                term->disptext[i]->chars[j].attr |= ATTR_INVALID;
            }
        }
    }
    if(!term->window_update_pending){
        term->window_update_pending=1;
        schedule_timer(UPDATE_DELAY, term_update_on_timer, term);
    }
}

void term_update(Terminal * term){
    int tb_need_sbar_update;
    int tn_scroll;

    if(paint_start()){
        tb_need_sbar_update=term->b_seen_disp_event;
        if(term->b_seen_disp_event && cfg.scroll_on_disp){
            term->disptop=0;
            term->b_seen_disp_event=0;
        }
        if(tb_need_sbar_update){
            tn_scroll=sblines(term);
            front_set_scrollbar(tn_scroll + term->rows, tn_scroll + term->disptop, term->rows);
        }
        do_paint(term);
        sys_cursor(NULL,  term->curs.x, term->curs.y - term->disptop);
        paint_finish();
    }
}

void do_paint(Terminal * term){
    wchar_t * p_chlist;
    int tn_chlen;
    termchar * p_newline;
    int tn_cursor_attr;
    int tn_curs_y;
    int tn_curs_x;
    termline * p_line;
    termchar * p_chars;
    termchar * p_curschar;
    int i;
    pos srcpos;
    int j;
    int tn_char;
    int tn_attr;
    int tb_selected;
    int tb_rv;
    int tn_laststart;
    int tb_dirtyrect;
    int k;
    int tb_dirty_line;
    unsigned long tn_attr_temp;
    unsigned long tn_cset_temp;
    int tn_start;
    int tn_count;
    int tb_dirty_run;
    int tb_last_run_dirty;
    int tb_break_run;
    termchar * p_char_cc;
    unsigned long tn_char_cc;

    tn_chlen=1024;
    p_chlist=snewn(tn_chlen, wchar_t);
    p_newline=snewn(term->cols, termchar);
    tn_cursor_attr=0;
    if(term->cursor_on){
        if(b_has_focus && (term->cblinker || !cfg.blink_cur)){
            tn_cursor_attr=TATTR_ACTCURS;
        }
        else{
            tn_cursor_attr=TATTR_PASCURS;
        }
        if(term->wrapnext){
            tn_cursor_attr |= TATTR_RIGHTCURS;
        }
    }
    tn_curs_y=term->curs.y - term->disptop;
    tn_curs_x=term->curs.x;
    p_line=lineptr(term, term->curs.y);
    p_chars=p_line->chars;
    if(tn_curs_x > 0 && p_chars[tn_curs_x].chr == UCSWIDE){
        tn_curs_x--;
    }
    unlineptr(p_line);
    if(term->dispcursy >= 0 && (term->curstype != tn_cursor_attr || term->dispcursy != tn_curs_y || term->dispcursx != tn_curs_x)){
        p_curschar=term->disptext[term->dispcursy]->chars + term->dispcursx;
        if(term->dispcursx > 0 && p_curschar->chr == UCSWIDE){
            p_curschar[-1].attr |= ATTR_INVALID;
        }
        if(term->dispcursx < term->cols-1 && p_curschar[1].chr == UCSWIDE){
            p_curschar[1].attr |= ATTR_INVALID;
        }
        p_curschar->attr |= ATTR_INVALID;
        term->curstype=0;
    }
    term->dispcursx=term->dispcursy = -1;
    for(i=0;i<term->rows;i++){
        srcpos.y=i + term->disptop;
        p_line=lineptr(term, srcpos.y);
        p_chars=p_line->chars;
        for(j=0;j<term->cols;j++){
            srcpos.x=j;
            tn_char=p_chars[j].chr;
            tn_attr=p_chars[j].attr;
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
            if(j < term->cols-1 && p_chars[j+1].chr == UCSWIDE){
                tn_attr |= ATTR_WIDE;
            }
            tb_selected=FALSE;
            tb_rv=(!term->rvideo ^ !term->in_vbell ? ATTR_REVERSE : 0);
            tn_attr=tn_attr ^ tb_rv ^ (tb_selected ? ATTR_REVERSE : 0);
            if(term->blink_is_real && (tn_attr & ATTR_BLINK)){
                if(b_has_focus && term->tblinker){
                    tn_char=unitab_line[' '];
                }
                tn_attr &= ~ATTR_BLINK;
            }
            if(tn_char != term->disptext[i]->chars[j].chr || tn_attr != (term->disptext[i]->chars[j].attr &~(ATTR_NARROW | DATTR_MASK))){
                if((tn_attr & ATTR_WIDE) == 0 && char_width(tn_char) == 2){
                    tn_attr |= ATTR_NARROW;
                }
            }
            else if(term->disptext[i]->chars[j].attr & ATTR_NARROW){
                tn_attr |= ATTR_NARROW;
            }
            if(i == tn_curs_y && j == tn_curs_x){
                tn_attr |= tn_cursor_attr;
                term->curstype=tn_cursor_attr;
                term->dispcursx=j;
                term->dispcursy=i;
            }
            p_newline[j].attr=tn_attr;
            p_newline[j].chr=tn_char;
            p_newline[j].cc_next=0;
        }
        tn_laststart=0;
        tb_dirtyrect=FALSE;
        for(j=0;j<term->cols;j++){
            if(term->disptext[i]->chars[j].attr & DATTR_STARTRUN){
                tn_laststart=j;
                tb_dirtyrect=FALSE;
            }
            if(term->disptext[i]->chars[j].chr != p_newline[j].chr || (term->disptext[i]->chars[j].attr &~ DATTR_MASK) != p_newline[j].attr){
                if(!tb_dirtyrect){
                    for(k=tn_laststart;k<j;k++){
                        term->disptext[i]->chars[k].attr |= ATTR_INVALID;
                    }
                    tb_dirtyrect=TRUE;
                }
            }
            if(tb_dirtyrect){
                term->disptext[i]->chars[j].attr |= ATTR_INVALID;
            }
        }
        tb_dirty_line=(p_line->lattr != term->disptext[i]->lattr);
        term->disptext[i]->lattr=p_line->lattr;
        tn_start=0;
        tn_count=0;
        tn_attr_temp=tn_attr;
        tn_cset_temp=CSET_OF(tn_char);
        tb_dirty_run=tb_dirty_line;
        tb_last_run_dirty=0;
        for(j=0;j<term->cols;j++){
            tn_attr=p_newline[j].attr;
            tn_char=p_newline[j].chr;
            if((term->disptext[i]->chars[j].attr ^ tn_attr) & ATTR_WIDE){
                tb_dirty_line=TRUE;
            }
            tb_break_run=(tn_attr ^ tn_attr_temp) != 0;
            if((tn_char >= 0x23BA && tn_char <= 0x23BD) || (j > 0 && (p_newline[j-1].chr >= 0x23BA && p_newline[j-1].chr <= 0x23BD))){
                tb_break_run=TRUE;
            }
            if(CSET_OF(tn_char) != tn_cset_temp){
                tb_break_run=TRUE;
            }
            if(p_chars[j].cc_next != 0 || (j > 0 && p_chars[j-1].cc_next != 0)){
                tb_break_run=TRUE;
            }
            if(!font_isdbcs && !tb_dirty_line){
                if(term->disptext[i]->chars[j].chr == tn_char && (term->disptext[i]->chars[j].attr &~ DATTR_MASK) == tn_attr){
                    tb_break_run=TRUE;
                }
                else if(!tb_dirty_run && tn_count == 1){
                    tb_break_run=TRUE;
                }
            }
            if(tb_break_run){
                if((tb_dirty_run || tb_last_run_dirty) && tn_count > 0){
                    do_text(tn_start, i, p_chlist, tn_count, tn_attr_temp, p_line->lattr&LATTR_MODE);
                    if(tn_attr_temp & (TATTR_ACTCURS | TATTR_PASCURS)){
                        do_cursor(tn_start, i, p_chlist, tn_count, tn_attr_temp, p_line->lattr&LATTR_MODE);
                    }
                }
                tn_start=j;
                tn_count=0;
                tn_attr_temp=tn_attr;
                tn_cset_temp=CSET_OF(tn_char);
                tb_dirty_run=tb_dirty_line;
                if(font_isdbcs){
                    tb_last_run_dirty=tb_dirty_run;
                }
            }
            if(tn_count >= tn_chlen){
                tn_chlen=tn_count + 256;
                p_chlist=sresize(p_chlist, tn_chlen, wchar_t);
            }
            p_chlist[tn_count++] = (wchar_t) tn_char;
            if(p_chars[j].cc_next){
                p_char_cc=p_chars+j;
                while(p_char_cc->cc_next){
                    p_char_cc += p_char_cc->cc_next;
                    tn_char_cc=p_char_cc->chr;
                    switch(tn_char_cc & CSET_MASK){
                        case CSET_ASCII:
                            tn_char_cc=unitab_line[tn_char_cc & 0xFF];
                            break;
                        case CSET_LINEDRW:
                            tn_char_cc=unitab_draw[tn_char_cc & 0xFF];
                            break;
                        case CSET_SCOACS:
                            tn_char_cc=unitab_scoacs[tn_char_cc & 0xFF];
                            break;
                    }
                    if(tn_count >= tn_chlen){
                        tn_chlen=tn_count + 256;
                        p_chlist=sresize(p_chlist, tn_chlen, wchar_t);
                    }
                    p_chlist[tn_count++] = (wchar_t) tn_char_cc;
                }
                tn_attr_temp |= TATTR_COMBINING;
            }
            if(!termchars_equal_override(&term->disptext[i]->chars[j], &p_chars[j], tn_char, tn_attr)){
                tb_dirty_run=TRUE;
                copy_termchar(term->disptext[i], j, &p_chars[j]);
                term->disptext[i]->chars[j].chr=tn_char;
                term->disptext[i]->chars[j].attr=tn_attr;
                if(tn_start == j){
                    term->disptext[i]->chars[j].attr |= DATTR_STARTRUN;
                }
            }
            if(tn_attr & ATTR_WIDE){
                j++;
                if(j < term->cols){
                    assert(!(i == tn_curs_y && j == tn_curs_x));
                    if(!termchars_equal(&term->disptext[i]->chars[j], &p_chars[j])){
                        tb_dirty_run=TRUE;
                    }
                    copy_termchar(term->disptext[i], j, &p_chars[j]);
                }
            }
        }
        if(tb_dirty_run && tn_count > 0){
            do_text(tn_start, i, p_chlist, tn_count, tn_attr_temp, p_line->lattr&LATTR_MODE);
            if(tn_attr_temp & (TATTR_ACTCURS | TATTR_PASCURS)){
                do_cursor(tn_start, i, p_chlist, tn_count, tn_attr_temp, p_line->lattr&LATTR_MODE);
            }
        }
        unlineptr(p_line);
    }
    sfree(p_newline);
    sfree(p_chlist);
}

void * compressline(termline * p_line){
    termchar * p_chars;
    int tn_free;
    int tn_buf_pos;
    int tn_buf_size;
    unsigned char* pc_buf_data;
    int n;
    int tn_rle_head;
    int tn_rle_state;
    int tn_prev;
    int j;
    int i;
    unsigned char* pc_temp;
    unsigned char* pc_line;

    p_chars=p_line->chars;
    if(p_line->cc_free){
        tn_free=p_line->cc_free;
        while(tn_free){
            p_chars[tn_free].chr=0;
            p_chars[tn_free].attr=0;
            if(p_chars[tn_free].cc_next){
                tn_free+=p_chars[tn_free].cc_next;
            }
            else{
                break;
            }
        }
    }
    tn_buf_pos=0;
    tn_buf_size=0;
    pc_buf_data=NULL;
    n=p_line->cols;
    while(n >= 128){
        if(tn_buf_pos >= tn_buf_size){
            tn_buf_size=(tn_buf_pos*3/2)+512;
            pc_buf_data=realloc(pc_buf_data, tn_buf_size);
        }
        pc_buf_data[tn_buf_pos++]=(unsigned char)((n & 0x7F)|0x80);
        n >>= 7;
    }
    if(tn_buf_pos >= tn_buf_size){
        tn_buf_size=(tn_buf_pos*3/2)+512;
        pc_buf_data=realloc(pc_buf_data, tn_buf_size);
    }
    pc_buf_data[tn_buf_pos++]=(unsigned char)(n);
    n=p_line->lattr;
    while(n >= 128){
        if(tn_buf_pos >= tn_buf_size){
            tn_buf_size=(tn_buf_pos*3/2)+512;
            pc_buf_data=realloc(pc_buf_data, tn_buf_size);
        }
        pc_buf_data[tn_buf_pos++]=(unsigned char)((n & 0x7F)|0x80);
        n >>= 7;
    }
    if(tn_buf_pos >= tn_buf_size){
        tn_buf_size=(tn_buf_pos*3/2)+512;
        pc_buf_data=realloc(pc_buf_data, tn_buf_size);
    }
    pc_buf_data[tn_buf_pos++]=(unsigned char)(n);
    n=p_line->size;
    while(n >= 128){
        if(tn_buf_pos >= tn_buf_size){
            tn_buf_size=(tn_buf_pos*3/2)+512;
            pc_buf_data=realloc(pc_buf_data, tn_buf_size);
        }
        pc_buf_data[tn_buf_pos++]=(unsigned char)((n & 0x7F)|0x80);
        n >>= 7;
    }
    if(tn_buf_pos >= tn_buf_size){
        tn_buf_size=(tn_buf_pos*3/2)+512;
        pc_buf_data=realloc(pc_buf_data, tn_buf_size);
    }
    pc_buf_data[tn_buf_pos++]=(unsigned char)(n);
    n=p_line->cc_free;
    while(n >= 128){
        if(tn_buf_pos >= tn_buf_size){
            tn_buf_size=(tn_buf_pos*3/2)+512;
            pc_buf_data=realloc(pc_buf_data, tn_buf_size);
        }
        pc_buf_data[tn_buf_pos++]=(unsigned char)((n & 0x7F)|0x80);
        n >>= 7;
    }
    if(tn_buf_pos >= tn_buf_size){
        tn_buf_size=(tn_buf_pos*3/2)+512;
        pc_buf_data=realloc(pc_buf_data, tn_buf_size);
    }
    pc_buf_data[tn_buf_pos++]=(unsigned char)(n);
    tn_rle_head=tn_buf_pos;
    if(tn_buf_pos >= tn_buf_size){
        tn_buf_size=(tn_buf_pos*3/2)+512;
        pc_buf_data=realloc(pc_buf_data, tn_buf_size);
    }
    pc_buf_data[tn_buf_pos++]=(unsigned char)(0);
    tn_rle_state=RLE_NoMatch;
    tn_prev=0x1ff;
    for(j=0;j<sizeof(termchar);j++){
        for(i=0;i<p_line->size;i++){
            pc_temp=(unsigned char *)(p_line->chars+i);
            if(pc_temp[j]==tn_prev){
                if(tn_rle_state==RLE_NoMatch){
                    if(pc_buf_data[tn_rle_head]<127){
                        pc_buf_data[tn_rle_head]++;
                        if(tn_buf_pos >= tn_buf_size){
                            tn_buf_size=(tn_buf_pos*3/2)+512;
                            pc_buf_data=realloc(pc_buf_data, tn_buf_size);
                        }
                        pc_buf_data[tn_buf_pos++]=(unsigned char)(pc_temp[j]);
                        tn_prev=pc_temp[j];
                    }
                    else{
                        tn_prev=pc_temp[j];
                        tn_rle_head=tn_buf_pos;
                        if(tn_buf_pos >= tn_buf_size){
                            tn_buf_size=(tn_buf_pos*3/2)+512;
                            pc_buf_data=realloc(pc_buf_data, tn_buf_size);
                        }
                        pc_buf_data[tn_buf_pos++]=(unsigned char)(1);
                        if(tn_buf_pos >= tn_buf_size){
                            tn_buf_size=(tn_buf_pos*3/2)+512;
                            pc_buf_data=realloc(pc_buf_data, tn_buf_size);
                        }
                        pc_buf_data[tn_buf_pos++]=(unsigned char)(tn_prev);
                        tn_rle_state=RLE_NoMatch;
                    }
                    tn_rle_state=RLE_Match;
                }
                else if(tn_rle_state==RLE_Match){
                    pc_buf_data[tn_rle_head]-=2;
                    if(pc_buf_data[tn_rle_head]>0){
                        tn_rle_head=tn_buf_pos-2;
                    }
                    else{
                        tn_rle_head=tn_buf_pos-3;
                        tn_buf_pos--;
                    }
                    pc_buf_data[tn_rle_head]=3|0x80;
                    tn_rle_state=RLE_Run;
                }
                else{
                    if(pc_buf_data[tn_rle_head]<255){
                        pc_buf_data[tn_rle_head]++;
                    }
                    else{
                        tn_prev=pc_buf_data[tn_rle_head+1];
                        tn_rle_head=tn_buf_pos;
                        if(tn_buf_pos >= tn_buf_size){
                            tn_buf_size=(tn_buf_pos*3/2)+512;
                            pc_buf_data=realloc(pc_buf_data, tn_buf_size);
                        }
                        pc_buf_data[tn_buf_pos++]=(unsigned char)(1);
                        if(tn_buf_pos >= tn_buf_size){
                            tn_buf_size=(tn_buf_pos*3/2)+512;
                            pc_buf_data=realloc(pc_buf_data, tn_buf_size);
                        }
                        pc_buf_data[tn_buf_pos++]=(unsigned char)(tn_prev);
                        tn_rle_state=RLE_NoMatch;
                    }
                }
            }
            else{
                if(tn_rle_state==RLE_NoMatch){
                    if(pc_buf_data[tn_rle_head]<127){
                        pc_buf_data[tn_rle_head]++;
                        if(tn_buf_pos >= tn_buf_size){
                            tn_buf_size=(tn_buf_pos*3/2)+512;
                            pc_buf_data=realloc(pc_buf_data, tn_buf_size);
                        }
                        pc_buf_data[tn_buf_pos++]=(unsigned char)(pc_temp[j]);
                        tn_prev=pc_temp[j];
                    }
                    else{
                        tn_prev=pc_temp[j];
                        tn_rle_head=tn_buf_pos;
                        if(tn_buf_pos >= tn_buf_size){
                            tn_buf_size=(tn_buf_pos*3/2)+512;
                            pc_buf_data=realloc(pc_buf_data, tn_buf_size);
                        }
                        pc_buf_data[tn_buf_pos++]=(unsigned char)(1);
                        if(tn_buf_pos >= tn_buf_size){
                            tn_buf_size=(tn_buf_pos*3/2)+512;
                            pc_buf_data=realloc(pc_buf_data, tn_buf_size);
                        }
                        pc_buf_data[tn_buf_pos++]=(unsigned char)(tn_prev);
                        tn_rle_state=RLE_NoMatch;
                    }
                }
                else if(tn_rle_state==RLE_Match){
                    tn_rle_state=RLE_NoMatch;
                    if(pc_buf_data[tn_rle_head]<127){
                        pc_buf_data[tn_rle_head]++;
                        if(tn_buf_pos >= tn_buf_size){
                            tn_buf_size=(tn_buf_pos*3/2)+512;
                            pc_buf_data=realloc(pc_buf_data, tn_buf_size);
                        }
                        pc_buf_data[tn_buf_pos++]=(unsigned char)(pc_temp[j]);
                        tn_prev=pc_temp[j];
                    }
                    else{
                        tn_prev=pc_temp[j];
                        tn_rle_head=tn_buf_pos;
                        if(tn_buf_pos >= tn_buf_size){
                            tn_buf_size=(tn_buf_pos*3/2)+512;
                            pc_buf_data=realloc(pc_buf_data, tn_buf_size);
                        }
                        pc_buf_data[tn_buf_pos++]=(unsigned char)(1);
                        if(tn_buf_pos >= tn_buf_size){
                            tn_buf_size=(tn_buf_pos*3/2)+512;
                            pc_buf_data=realloc(pc_buf_data, tn_buf_size);
                        }
                        pc_buf_data[tn_buf_pos++]=(unsigned char)(tn_prev);
                        tn_rle_state=RLE_NoMatch;
                    }
                }
                else{
                    tn_prev=pc_temp[j];
                    tn_rle_head=tn_buf_pos;
                    if(tn_buf_pos >= tn_buf_size){
                        tn_buf_size=(tn_buf_pos*3/2)+512;
                        pc_buf_data=realloc(pc_buf_data, tn_buf_size);
                    }
                    pc_buf_data[tn_buf_pos++]=(unsigned char)(1);
                    if(tn_buf_pos >= tn_buf_size){
                        tn_buf_size=(tn_buf_pos*3/2)+512;
                        pc_buf_data=realloc(pc_buf_data, tn_buf_size);
                    }
                    pc_buf_data[tn_buf_pos++]=(unsigned char)(tn_prev);
                    tn_rle_state=RLE_NoMatch;
                }
            }
        }
    }
    pc_line=realloc(pc_buf_data, tn_buf_pos+1);
    return pc_line;
}

termline * decompressline(void * buf){
    unsigned char* pc_line;
    int tn_buf_pos;
    unsigned char* pc_buf_data;
    termline * p_line;
    int n;
    int tn_c;
    termchar * p_chars;
    int tn_rle_count;
    int j;
    int i;
    unsigned char* pc_temp;

    pc_line=buf;
    tn_buf_pos=0;
    pc_buf_data=buf;
    p_line=snew(termline);
    n=0;
    tn_c=pc_buf_data[tn_buf_pos++];
    while(tn_c & 0x80){
        n+=tn_c&0x7f;
        n<<=7;
        tn_c=pc_buf_data[tn_buf_pos++];
    }
    p_line->cols=n+(tn_c&0x7f);
    n=0;
    tn_c=pc_buf_data[tn_buf_pos++];
    while(tn_c & 0x80){
        n+=tn_c&0x7f;
        n<<=7;
        tn_c=pc_buf_data[tn_buf_pos++];
    }
    p_line->lattr=n+(tn_c&0x7f);
    n=0;
    tn_c=pc_buf_data[tn_buf_pos++];
    while(tn_c & 0x80){
        n+=tn_c&0x7f;
        n<<=7;
        tn_c=pc_buf_data[tn_buf_pos++];
    }
    p_line->size=n+(tn_c&0x7f);
    n=0;
    tn_c=pc_buf_data[tn_buf_pos++];
    while(tn_c & 0x80){
        n+=tn_c&0x7f;
        n<<=7;
        tn_c=pc_buf_data[tn_buf_pos++];
    }
    p_line->cc_free=n+(tn_c&0x7f);
    p_line->chars=snewn(p_line->size, termchar);
    p_chars=p_line->chars;
    tn_rle_count=pc_buf_data[tn_buf_pos];
    tn_buf_pos++;
    for(j=0;j<sizeof(termchar);j++){
        for(i=0;i<p_line->size;i++){
            pc_temp=(unsigned char *)(p_line->chars+i);
            if((tn_rle_count & 0x7f)>0){
                if(tn_rle_count & 0x80){
                    pc_temp[j]=pc_buf_data[tn_buf_pos];
                }
                else{
                    pc_temp[j]=pc_buf_data[tn_buf_pos];
                    tn_buf_pos++;
                }
                tn_rle_count--;
            }
            if((tn_rle_count & 0x7f)==0){
                if(tn_rle_count & 0x80){
                    tn_buf_pos++;
                }
                tn_rle_count=pc_buf_data[tn_buf_pos];
                tn_buf_pos++;
            }
        }
    }
    return p_line;
}

void term_debug_screen(Terminal * term){
    termline * p_line;
    termchar * p_chars;
    int i;

    p_line=lineptr(term, 0);
    p_chars=p_line->chars;
    fprintf(stdout, "    :[debug] p_line->cols=%d, p_line->size=%d\n", p_line->cols, p_line->size);
    for(i=0;i<p_line->cols;i++){
        printf("%d - %08x - %08x - %d ", i, p_chars[i].chr, p_chars[i].attr, p_chars[i].cc_next);
        if(i % 2 ==1){
            printf("\n");
        }
    }
}

int term_app_cursor_keys(Terminal * term){
    return term->b_app_cursor_keys;
}

void term_provide_logctx(Terminal * term, void * logctx){
    term->logctx=logctx;
}

