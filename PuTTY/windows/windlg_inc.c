#include <winsock2.h>
#include <windows.h>
#include <string.h>
#include "windlg_inc-res.h"
#include <stdlib.h>

void showabout(HWND hwnd);
void modal_about_box(HWND hwnd);
LRESULT CALLBACK WndProc_about(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
LRESULT CALLBACK WndProc_config(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
LRESULT CALLBACK WndProc_logbox(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
LRESULT CALLBACK WndProc_license(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

char * s_version="PuTTY 0.62 - MyDef Build";
extern HINSTANCE cur_instance;
char * * events=NULL;
int n_da_len_events=0;
int n_da_size_events=0;
HWND hwnd_log=NULL;
unsigned char sel_nl[]={13,10};

LRESULT CALLBACK WndProc_about(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam){
    switch(msg){
        case WM_INITDIALOG:
            SetDlgItemText(hwnd, text_Version, s_version);
            return 1;
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case button_Close:
                    EndDialog(hwnd, 1);
                    return 0;
                case button_License:
                    EnableWindow(hwnd, 0);
                    DialogBox(cur_instance, MAKEINTRESOURCE(dialog_license), hwnd, WndProc_license);
                    EnableWindow(hwnd, 1);
                    SetActiveWindow(hwnd);
                    return 0;
                case button_Web:
                    ShellExecute(hwnd, "open", "http://puttytray.goeswhere.com/", 0, 0, SW_SHOWDEFAULT);
                    return 0;
            }
            break;
        case WM_CLOSE:
            EndDialog(hwnd, 1);
            return 0;
        default:
            break;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

LRESULT CALLBACK WndProc_config(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam){
    switch(msg){
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
            }
            break;
        case WM_CLOSE:
            EndDialog(hwnd, 1);
            return 0;
        default:
            break;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

LRESULT CALLBACK WndProc_logbox(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam){
    static int pn_logtabs[4]={78,108};
    int i;
    int tn_count;
    int* pn_selitems;
    int tn_size;
    char * ts_clipdata;
    char * ts_p;
    char * ts_q;
    int tn_len;

    switch(msg){
        case WM_INITDIALOG:
            SendDlgItemMessage(hwnd, list_Log, LB_SETTABSTOPS, 2,(LPARAM)pn_logtabs);
            for(i=0;i<n_da_len_events;i++){
                SendDlgItemMessage(hwnd, list_Log, LB_ADDSTRING, 0, (LPARAM) events[i]);
            }
            return 1;
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case button_Close:
                    hwnd_log=NULL;
                    SetActiveWindow(GetParent(hwnd));
                    DestroyWindow(hwnd);
                    return 0;
                case button_Copy:
                    if(HIWORD(wParam) == BN_CLICKED || HIWORD(wParam) == BN_DOUBLECLICKED){
                        int *selitems;
                        tn_count=SendDlgItemMessage(hwnd, list_Log, LB_GETSELCOUNT, 0, 0);
                        if(tn_count>0){
                            pn_selitems=(int*)malloc(tn_count*sizeof(int));
                            assert(pn_selitems);
                            tn_count=SendDlgItemMessage(hwnd, list_Log, LB_GETSELITEMS, tn_count, (LPARAM) pn_selitems);
                            tn_size=0;
                            for(i=0;i<tn_count;i++){
                                tn_size+=strlen(events[pn_selitems[i]]) + sizeof(sel_nl);
                            }
                            ts_clipdata=(char*)malloc(tn_size*sizeof(char));
                            assert(ts_clipdata);
                            ts_p=ts_clipdata;
                            for(i=0;i<tn_count;i++){
                                ts_q=events[pn_selitems[i]];
                                tn_len=strlen(ts_q);
                                memcpy(ts_p, ts_q, tn_len);
                                ts_p += tn_len;
                                memcpy(ts_p, sel_nl, sizeof(sel_nl));
                                ts_p += sizeof(sel_nl);
                            }
                            write_aclip(NULL, ts_clipdata, tn_size, TRUE);
                            for(i=0;i<n_da_len_events;i++){
                                SendDlgItemMessage(hwnd, list_Log, LB_SETSEL, FALSE, i);
                            }
                            free(pn_selitems);
                            free(ts_clipdata);
                        }
                    }
                    return 0;
            }
            break;
        case WM_CLOSE:
            hwnd_log=NULL;
            SetActiveWindow(GetParent(hwnd));
            DestroyWindow(hwnd);
            return 0;
        default:
            break;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

LRESULT CALLBACK WndProc_license(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam){
    switch(msg){
        case WM_INITDIALOG:
            SetWindowText(hwnd, "PuTTY License");
            return 1;
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case button_OK:
                    EndDialog(hwnd, 1);
                    return 0;
            }
            break;
        case WM_CLOSE:
            EndDialog(hwnd, 1);
            return 0;
        default:
            break;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

void showabout(HWND hwnd){
    DialogBox(hinst, MAKEINTRESOURCE(dialog_about), hwnd, WndProc_about);
}

void modal_about_box(HWND hwnd){
    EnableWindow(hwnd, 0);
    DialogBox(cur_instance, MAKEINTRESOURCE(dialog_about), hwnd, WndProc_about);
    EnableWindow(hwnd, 1);
    SetActiveWindow(hwnd);
}

