#include <winsock2.h>
#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define lenof(x) ((sizeof((x)))/(sizeof(*(x))))
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

const char * cp_enumerate(int n_index);
const char * cp_name(int n_codepage);
int decode_codepage(char * s_codepage);
int wc_to_mb(int n_codepage, wchar_t * wbuf, int wlen, char * sbuf, int slen);
int mb_to_wc(int n_codepage, int n_flags, char * sbuf, int slen, wchar_t * wbuf, int wlen);
int is_dbcs_leadbyte(int n_codepage, char byte);
void get_unitab(int n_codepage, wchar_t * unitab, int n_type);
void link_font(wchar_t * tbl_line, wchar_t * tbl_font, wchar_t n_attr);
void update_ucs_charset(DWORD n_cs);
void update_ucs_line_codepage(char * s_codepage);
void update_ucs_linedraw(int n_vtmode);

int font_isdbcs;
int font_codepage;
int line_codepage;
wchar_t unitab_scoacs[256];
wchar_t unitab_line[256];
wchar_t unitab_font[256];
wchar_t unitab_draw[256];
wchar_t unitab_oemcp[256];
unsigned char unitab_ctrl[256];
char ** uni_tbl;

const char * cp_enumerate(int n_index){
    if(n_index<0 || n_index >= lenof(cp_list)){
        return NULL;
    }
    else{
        return cp_list[n_index].name;
    }
}

const char * cp_name(int n_codepage){
    const struct cp_list_item * cpi;
    const struct cp_list_item * cpno;
    static  char ts_buf[32];

    if(n_codepage == -1){
        sprintf(ts_buf, "Use font encoding");
        return ts_buf;
    }
    if(n_codepage > 0 && n_codepage < 65536){
        sprintf(ts_buf, "CP%03d", n_codepage);
    }
    else{
        *ts_buf=0;
    }
    if(n_codepage >= 65536){
        if((n_codepage-65536)<lenof(cp_list)){
            cpno=cp_list+(n_codepage-65536);
            for(cpi = cp_list; cpi->name; cpi++){
                if(cpno->cp_table == cpi->cp_table){
                    return cpi->name;
                }
            }
        }
        else{
            cpno=0;
        }
    }
    else{
        for(cpi = cp_list; cpi->name; cpi++){
            if(n_codepage == cpi->codepage){
                return cpi->name;
            }
        }
    }
    return ts_buf;
}

int decode_codepage(char * s_codepage){
    int tn_codepage=-1;
    CPINFO cpinfo;
    int tn_cp;
    const struct cp_list_item * cpi;
    char * ts_s1;
    char * ts_s2;

    if(!*s_codepage){
        tn_cp=GetACP();
        switch(tn_cp){
            s_codepage="ISO-8859-2"; break;
            s_codepage="KOI8-U"; break;
            s_codepage="ISO-8859-1"; break;
            s_codepage="ISO-8859-7"; break;
            s_codepage="ISO-8859-9"; break;
            s_codepage="ISO-8859-8"; break;
            s_codepage="ISO-8859-6"; break;
            s_codepage="ISO-8859-13"; break;
        }
    }
    if(s_codepage && *s_codepage){
        for(cpi = cp_list; cpi->name; cpi++){
            ts_s1=s_codepage;
            ts_s2=cpi->name;
            while(1){
                while(*ts_s1 && !isalnum(*ts_s1) && *ts_s1 != ':'){
                    ts_s1++;
                }
                while(*ts_s2 && !isalnum(*ts_s2) && *ts_s2 != ':'){
                    ts_s2++;
                }
                if(*ts_s1 == 0){
                    tn_codepage=cpi->codepage;
                    if(tn_codepage == CP_UTF8){
                        goto return_codepage;
                    }
                    if(tn_codepage == -1){
                        return -1;
                    }
                    if(tn_codepage == 0){
                        tn_codepage=65536 + (cpi - cp_list);
                        goto return_codepage;
                    }
                    if(GetCPInfo(tn_codepage, &cpinfo) != 0){
                        goto return_codepage;
                    }
                }
                if(tolower(*ts_s1++) != tolower(*ts_s2++)){
                    break;
                }
            }
        }
    }
    if(s_codepage && *s_codepage){
        ts_s1=s_codepage;
        if(tolower(ts_s1[0]) == 'c' && tolower(ts_s1[1]) == 'p'){
            ts_s1 += 2;
        }
        if(tolower(ts_s1[0]) == 'i' && tolower(ts_s1[1]) == 'b' && tolower(ts_s1[2]) == 'm'){
            ts_s1 += 3;
        }
        for(ts_s2=ts_s1; *ts_s2 >= '0' && *ts_s2 <= '9'; ts_s2++);
        if(*ts_s2 == 0 && ts_s2 != ts_s1){
            tn_codepage=atoi(ts_s1);
        }
        if(tn_codepage == 0){
            tn_codepage=GetACP();
        }
        if(tn_codepage == 1){
            tn_codepage=GetOEMCP();
        }
        if(tn_codepage > 65535){
            tn_codepage=-2;
        }
    }
    return_codepage:
    if(tn_codepage != -1){
        if(tn_codepage != CP_UTF8 && tn_codepage < 65536){
            if(GetCPInfo(tn_codepage, &cpinfo) == 0){
                tn_codepage=-2;
            }
            else if(cpinfo.MaxCharSize > 1){
                tn_codepage=-3;
            }
        }
    }
    if(tn_codepage == -1 && *s_codepage){
        tn_codepage=-2;
    }
    return tn_codepage;
}

int wc_to_mb(int n_codepage, wchar_t * wbuf, int wlen, char * sbuf, int slen){
    char * p;
    int i;
    wchar_t ch;
    char * p1;

    if(n_codepage == line_codepage && uni_tbl){
        if(wlen < 0){
            for(wlen=0; wbuf[wlen++] ;);
        }
        p=sbuf;
        for(i=0;i<wlen;i++){
            ch=wbuf[i];
            p1=uni_tbl[(ch >> 8) & 0xFF];
            if(p1 && p1[ch & 0xFF]){
                *p++ = p1[ch&0xFF];
            }
            else if(ch < 0x80){
                *p++ = (char) ch;
            }
            else{
                *p++ = '.';
            }
            assert(p - sbuf < slen);
        }
        return p - sbuf;
    }
    return WideCharToMultiByte(n_codepage, 0, wbuf, wlen, sbuf, slen, NULL, NULL);
}

int mb_to_wc(int n_codepage, int n_flags, char * sbuf, int slen, wchar_t * wbuf, int wlen){
    return MultiByteToWideChar(n_codepage, n_flags, sbuf, slen, wbuf, wlen);
}

int is_dbcs_leadbyte(int n_codepage, char byte){
    return IsDBCSLeadByteEx(n_codepage, byte);
}

void get_unitab(int n_codepage, wchar_t * unitab, int n_type){
    int tn_max;
    int i;
    int tn_flg;
    char tbuf[4];
    int j;

    tn_max=256;
    if(n_type == 2){
        tn_max=128;
    }
    if(n_codepage == CP_UTF8){
        for(i=0;i<tn_max;i++){
            unitab[i] = i;
        }
        return;
    }
    if(n_codepage == CP_ACP){
        n_codepage=GetACP();
    }
    else if(n_codepage == CP_OEMCP){
        n_codepage=GetOEMCP();
    }
    if(n_codepage > 0 && n_codepage < 65536){
        tn_flg=MB_ERR_INVALID_CHARS;
        if(n_type){
            tn_flg |= MB_USEGLYPHCHARS;
        }
        for(i=0;i<tn_max;i++){
            tbuf[0] = i;
            if(mb_to_wc(n_codepage, tn_flg, tbuf, 1, unitab + i, 1) != 1){
                unitab[i] = 0xFFFD;
            }
        }
    }
    else{
        j=256 - cp_list[n_codepage & 0xFFFF].cp_size;
        for(i=0;i<j;i++){
            unitab[i] = i;
        }
        for(i=j;i<tn_max;i++){
            unitab[i] = cp_list[n_codepage & 0xFFFF].cp_table[i - j];
        }
    }
}

void link_font(wchar_t * tbl_line, wchar_t * tbl_font, wchar_t n_attr){
    int tn_i;
    int i;
    int tn_j;

    for(tn_i=0;tn_i<256;tn_i++){
        if(DIRECT_FONT(tbl_line[tn_i])){
            continue;
        }
        else{
            for(i=0;i<256;i++){
                tn_j=((32 + i) & 0xFF);
                if(tbl_line[tn_i] == tbl_font[tn_j]){
                    tbl_line[tn_i] = (wchar_t) (n_attr + tn_j);
                    break;
                }
            }
        }
    }
}

void update_ucs_charset(DWORD n_cs){
    CHARSETINFO info_charset;
    CPINFO info_cp;

    if(n_cs == OEM_CHARSET){
        font_codepage=GetOEMCP();
    }
    else{
        memset(&info_charset, 0xFF, sizeof(info_charset));
        if(TranslateCharsetInfo ((DWORD *) n_cs, &info_charset, TCI_SRCCHARSET)){
            font_codepage=info_charset.ciACP;
        }
        else{
            font_codepage=-1;
        }
    }
    if(font_codepage <= 0){
        font_codepage=0;
        font_isdbcs=0;
    }
    else{
        GetCPInfo(font_codepage, &info_cp);
        font_isdbcs=(info_cp.MaxCharSize > 1);
    }
}

void update_ucs_line_codepage(char * s_codepage){
    line_codepage=decode_codepage(s_codepage);
}

void update_ucs_linedraw(int n_vtmode){
    int i;
    int tb_used_dtf;
    int j;
    static  const char poorman_scoacs[]="CueaaaaceeeiiiAAE**ooouuyOUc$YPsaiounNao?++**!<>###||||++||++++++--|-+||++--|-+----++++++++##||#aBTPEsyt******EN=+><++-=... n2* ";
    static  const char poorman_latin1[]=" !cL.Y|S\"Ca<--R~o+23'u|.,1o>///?AAAAAAACEEEEIIIIDNOOOOOxOUUUUYPBaaaaaaaceeeeiiiionooooo/ouuuuypy";
    static  const char poorman_vt100[]= "*#****o~**+++++-----++++|****L.";

    if(n_vtmode == VT_OEMONLY){
        font_codepage=437;
        font_isdbcs=0;
        if(line_codepage <= 0){
            line_codepage=GetACP();
        }
    }
    else if(line_codepage <= 0){
        line_codepage=font_codepage;
    }
    if(font_isdbcs || font_codepage == 0){
        get_unitab(font_codepage, unitab_font, 2);
        for(i=128;i<256;i++){
            unitab_font[i] = (WCHAR) (CSET_ACP + i);
        }
    }
    else{
        get_unitab(font_codepage, unitab_font, 1);
        if(font_codepage == 437){
            unitab_font[0] = unitab_font[255] = 0xFFFF;
        }
    }
    if(n_vtmode == VT_XWINDOWS){
        memcpy(unitab_font + 1, unitab_draw_std, sizeof(unitab_draw_std));
    }
    get_unitab(CP_OEMCP, unitab_oemcp, 1);
    if(n_vtmode == VT_OEMANSI || n_vtmode == VT_XWINDOWS){
        memcpy(unitab_scoacs, unitab_oemcp, sizeof(unitab_scoacs));
    }
    else{
        get_unitab(437, unitab_scoacs, 1);
    }
    if(line_codepage == font_codepage && (font_isdbcs || n_vtmode == VT_POORMAN || font_codepage==0)){
        tb_used_dtf=1;
        for(i=0;i<32;i++){
            unitab_line[i] = (WCHAR) i;
        }
        for(i=32;i<256;i++){
            unitab_line[i] = (WCHAR) (CSET_ACP + i);
        }
        unitab_line[127] = (WCHAR) 127;
    }
    else{
        get_unitab(line_codepage, unitab_line, 0);
    }
    memcpy(unitab_draw, unitab_line, sizeof(unitab_draw));
    memcpy(unitab_draw + '`', unitab_draw_std, sizeof(unitab_draw_std));
    unitab_draw['_'] = ' ';
    if(uni_tbl){
        for(i=0;i<256;i++){
            if(uni_tbl[i]){
                free(uni_tbl[i]);
            }
        }
        free(uni_tbl);
        uni_tbl=0;
    }
    if(!tb_used_dtf){
        for(i=0;i<256;i++){
            if(DIRECT_CHAR(unitab_line[i])){
                continue;
            }
            if(DIRECT_FONT(unitab_line[i])){
                continue;
            }
            if(!uni_tbl){
                uni_tbl=(char **)malloc(256*sizeof(char *));
                memset(uni_tbl, 0, 256 * sizeof(char *));
            }
            j=((unitab_line[i] >> 8) & 0xFF);
            if(!uni_tbl[j]){
                uni_tbl[j]=(char*)malloc(256*sizeof(char));
                memset(uni_tbl[j], 0, 256 * sizeof(char));
            }
            uni_tbl[j][unitab_line[i] & 0xFF] = i;
        }
    }
    for(i=0;i<256;i++){
        if(unitab_line[i] < ' ' || (unitab_line[i] >= 0x7F && unitab_line[i] < 0xA0)){
            unitab_ctrl[i] = i;
        }
        else{
            unitab_ctrl[i] = 0xFF;
        }
    }
    if(n_vtmode == VT_OEMANSI || n_vtmode == VT_XWINDOWS){
        link_font(unitab_scoacs, unitab_oemcp, CSET_OEMCP);
    }
    link_font(unitab_line, unitab_font, CSET_ACP);
    link_font(unitab_scoacs, unitab_font, CSET_ACP);
    link_font(unitab_draw, unitab_font, CSET_ACP);
    if(n_vtmode == VT_OEMANSI || n_vtmode == VT_XWINDOWS){
        link_font(unitab_line, unitab_oemcp, CSET_OEMCP);
        link_font(unitab_draw, unitab_oemcp, CSET_OEMCP);
    }
    if(font_isdbcs && font_codepage != line_codepage){
        unitab_line['\\'] = CSET_OEMCP + '\\';
    }
    if(n_vtmode != VT_UNICODE){
        for(i=160;i<256;i++){
            if(!DIRECT_FONT(unitab_line[i]) && unitab_line[i] >= 160 && unitab_line[i] < 256){
                unitab_line[i] = (wchar_t) (CSET_ACP + poorman_latin1[unitab_line[i] - 160]);
            }
        }
        for(i=96;i<127;i++){
            if(!DIRECT_FONT(unitab_draw[i])){
                unitab_draw[i] =(wchar_t) (CSET_ACP + poorman_vt100[i - 96]);
            }
        }
        for(i=128;i<256;i++){
            if(!DIRECT_FONT(unitab_scoacs[i])){
                unitab_scoacs[i] = (wchar_t) (CSET_ACP + poorman_scoacs[i - 128]);
            }
        }
    }
}

