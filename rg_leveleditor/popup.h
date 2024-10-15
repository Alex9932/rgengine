/*
 * rgEngine popup.h
 *
 *  Created on: Sep 19, 2024
 *      Author: alex9932
 */

#ifndef _POPUPWINDOW_H
#define _POPUPWINDOW_H

#include <rgtypes.h>

// Popup mode
#define POPUP_MODE_INFO        0x00000000
#define POPUP_MODE_TEXTINPUT   0x00000001
#define POPUP_MODE_PROGRESSBAR 0x00000002

// Popup buttons
#define POPUP_BTN_NOBTN        0x00000000
#define POPUP_BTN_OK_CANCEL    0x00000001

// Popup buttons id
#define POPUP_BTNID_OK         0x00000001
#define POPUP_BTNID_CANCEL     0x00000000
#define POPUP_BTNID_NOBTN      0xFFFFFFFF

void PopupShow(String title, String text, Uint32 mode, Uint32 btns, void* data = NULL);
void PopupDraw();

RG_INLINE void PopupShowInfo(String title, String text, void* data = NULL) { PopupShow(title, text, POPUP_MODE_INFO, POPUP_BTN_OK_CANCEL, data); }
RG_INLINE void PopupShowInput(String title, String text, void* data = NULL) { PopupShow(title, text, POPUP_MODE_TEXTINPUT, POPUP_BTN_OK_CANCEL, data); }
RG_INLINE void PopupShowProgress(String title, String text, void* data = NULL) { PopupShow(title, text, POPUP_MODE_PROGRESSBAR, POPUP_BTN_OK_CANCEL, data); }

Bool PopupShown();
void PopupSetInputBuffer(String str);
void PopupSetProgress(Float32 p);
String PopupGetInputBuffer();
Float32 PopupGetProgress();
Uint32 PopupGetBtnPressed();

#endif