//doomgeneric for cross-platform development library 'Simple DirectMedia Layer'
// ============================================================================
// NÂNG CẤP LÊN NẤC 1A (v2.1.0) - EDU DOOM ASS
// Ghi chú các thay đổi so với v2.0.3:
//   1. Chuyển toàn bộ Renderer từ SDL (2D) sang OpenGL (Hardware).
//   2. Bật VSync (chống xé hình).
//   3. Thêm phím F5 để đổi bộ lọc (Pixel sắc cạnh <-> Làm mịn).
//   4. Bắt sự kiện Resize cửa sổ để cập nhật glViewport.
// ============================================================================
// Upgraded for EduDoom-Ass (v2.0.3):
//   0. Auto-detects wads/DOOM1.WAD (and <exe_dir>/wads/DOOM1.WAD) in main() so launching from Geany or Explorer always finds DOOM1.WAD
//   1. Full WASD Movement (W=Forward, S=Backward, A=Strafe Left, D=Strafe Right)
//      - Simultaneous diagonal movement (W+A, W+D, S+A, S+D) with OS key-repeat filtering
//      - 100% independent of Mouse X camera turning (angleturn vs sidemove)
//      - Smart "id..." Cheat Prefix Detector so all DOOM cheats (iddqd, idkfa, idspispopd,
//        idclip, idbehold, idclev, idchoppers, idmypos, iddt) work without conflict!
//   2. Mouse Wheel & Mouse Clicks in Gameplay (!menuactive):
//      - Scroll Wheel Up / Down : Cycle Next / Previous owned weapon (G_NextWeapon)
//      - Left Click             : Fire weapon (BT_ATTACK)
//      - Right / Middle Click   : Use / Open Door / Activate Switch (KEY_USE)
//   3. Mouse Wheel & Mouse Clicks in Menu (menuactive):
//      - Scroll Wheel Up / Down : Scroll Menu Skull Up / Down (KEY_UPARROW / KEY_DOWNARROW)
//      - Left Click on Normal   : Select / Enter menu item (KEY_ENTER, or 'y' on Yes/No prompt)
//      - Left Click on Slider   : Toggle Slider Adjust Mode (Scroll Wheel adjusts horizontal
//                                 slider Left/Right; Left Click again locks the value!)
//      - Right Click in Menu    : Back to previous menu (KEY_BACKSPACE), or exit Main Menu
//                                 straight back to game (KEY_ESCAPE, or 'n' on Yes/No prompt)

#include "doomkeys.h"
#include "m_argv.h"
#include "doomgeneric.h"
#include "d_event.h"
#include "doomtype.h"
#include "m_controls.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <SDL.h>
#include <SDL_opengl.h> // [THAY ĐỔI] Thêm thư viện OpenGL

SDL_Window* window = NULL;
// [THAY ĐỔI] Thay thế SDL_Renderer và SDL_Texture bằng OpenGL Context và Texture
SDL_GLContext glContext = NULL;
GLuint glTexture = 0;
int filter_mode = 1; // [THAY ĐỔI] 0 = Nearest (Sắc cạnh), 1 = Linear (Làm mịn)

// --- External DOOM Engine Menu & Game State (from m_menu.c / doomstat.h) ---
typedef struct
{
  short status; // 1 = normal item, 2 = horizontal thermometer slider
  char name[10];
  void (*routine)(int choice);
  char alphaKey;
} dg_menuitem_t;

typedef struct dg_menu_s
{
  short numitems;
  struct dg_menu_s *prevMenu;
  dg_menuitem_t *menuitems;
  void (*routine)(void);
  short x;
  short y;
  short lastOn;
} dg_menu_t;

extern boolean menuactive;
extern boolean inhelpscreens;
extern short itemOn;
extern dg_menu_t *currentMenu;
extern int messageToPrint;
extern boolean messageNeedsInput;
extern int saveStringEnter;

#define KEYQUEUE_SIZE 128

// Custom internal keycodes bound to DOOM's native key_prevweapon / key_nextweapon
#define DG_KEY_PREVWEAPON 0xf0
#define DG_KEY_NEXTWEAPON 0xf1

static unsigned short s_KeyQueue[KEYQUEUE_SIZE];
static unsigned int s_KeyQueueWriteIndex = 0;
static unsigned int s_KeyQueueReadIndex = 0;

// --- Mouse State (Connected to DOOM Core Engine via D_PostEvent) ---
static int s_MouseGrabbed = 1;       // 1 = Relative Mouse Mode active (FPS lock)
static int s_NoVert = 1;             // 1 = Ignore vertical mouse motion (modern FPS style)
static int s_MouseSensScale = 4;     // Multiplier for SDL xrel/yrel -> DOOM ev_mouse
static int s_MouseButtons = 0;       // Bit 0: Left (Fire)
static int s_PrevMouseButtons = 0;
static int s_AccumMouseDX = 0;
static int s_AccumMouseDY = 0;
static int s_LastMouseX = 0;
static int s_LastMouseY = 0;

// --- Menu Slider Edit State ---
static int s_MenuSliderEdit = 0;     // 0 = Wheel scrolls menu Up/Down; 1 = Wheel adjusts slider Left/Right
static short s_SliderMenuItemOn = -1;
static dg_menu_t *s_SliderMenuPtr = NULL;
static int s_PrevMenuActive = 0;

// --- WASD & "id..." Cheat Prefix State Machine ---
// Tracks exact DOOM key sent on KEYDOWN for [0]=W, [1]=A, [2]=S, [3]=D so KEYUP always matches
static unsigned char s_WasdActiveKey[4] = { 0, 0, 0, 0 };
// 0 = Normal WASD movement; 1 = Just pressed 'i'; 2 = Typing "id..." cheat code
static int s_CheatState = 0;
static uint32_t s_CheatExpireTick = 0;
static char s_CheatBuf[16];
static int s_CheatLen = 0;

static void setMouseGrab(int grab)
{
  s_MouseGrabbed = grab ? 1 : 0;
  SDL_SetRelativeMouseMode(s_MouseGrabbed ? SDL_TRUE : SDL_FALSE);
}

static int isMenuOverlayActive(void)
{
  return (menuactive || inhelpscreens || messageToPrint != 0) ? 1 : 0;
}

static int isCurrentMenuItemSlider(void)
{
  if (!menuactive || inhelpscreens || messageToPrint != 0 || saveStringEnter != 0)
    return 0;
  if (currentMenu == NULL || currentMenu->menuitems == NULL)
    return 0;
  if (itemOn < 0 || itemOn >= currentMenu->numitems)
    return 0;
  return (currentMenu->menuitems[itemOn].status == 2) ? 1 : 0;
}

static void enqueueDoomKey(int pressed, unsigned char doomKey)
{
  unsigned short keyData = ((pressed ? 1 : 0) << 8) | (doomKey & 0xFF);
  unsigned int nextWrite = (s_KeyQueueWriteIndex + 1) % KEYQUEUE_SIZE;
  if (nextWrite == s_KeyQueueReadIndex) {
    // Prevent queue overflow from dropping events
    return;
  }
  s_KeyQueue[s_KeyQueueWriteIndex] = keyData;
  s_KeyQueueWriteIndex = nextWrite;
}

static void tapDoomKey(unsigned char doomKey)
{
  enqueueDoomKey(1, doomKey);
  enqueueDoomKey(0, doomKey);
}

static void releaseActiveWasdKeys(void)
{
  for (int i = 0; i < 4; ++i) {
    if (s_WasdActiveKey[i] != 0) {
      enqueueDoomKey(0, s_WasdActiveKey[i]);
      s_WasdActiveKey[i] = 0;
    }
  }
}

static int wasdIndexFromSym(unsigned int sym)
{
  switch (sym) {
    case SDLK_w: return 0;
    case SDLK_a: return 1;
    case SDLK_s: return 2;
    case SDLK_d: return 3;
    default:     return -1;
  }
}

static unsigned char wasdMovementKey(int idx)
{
  switch (idx) {
    case 0: return (unsigned char)(key_up ? key_up : KEY_UPARROW);
    case 1: return (unsigned char)(key_strafeleft ? key_strafeleft : ',');
    case 2: return (unsigned char)(key_down ? key_down : KEY_DOWNARROW);
    case 3: return (unsigned char)(key_straferight ? key_straferight : '.');
    default: return 0;
  }
}

// Checks if s_CheatBuf matches a complete DOOM cheat code so WASD resumes immediately
static int isCompleteDoomCheat(const char *buf, int len)
{
  static const char * const exactCheats[] = {
    "iddqd", "idkfa", "idfa", "idspispopd", "idclip",
    "idchoppers", "idmypos", "iddt"
  };
  for (unsigned int i = 0; i < sizeof(exactCheats) / sizeof(exactCheats[0]); ++i) {
    if (strcmp(buf, exactCheats[i]) == 0)
      return 1;
  }
  // idbehold + 1 letter (v, s, i, r, a, l) -> length 9
  if (len >= 9 && strncmp(buf, "idbehold", 8) == 0)
    return 1;
  // idclev + 2 digits -> length 8
  if (len >= 8 && strncmp(buf, "idclev", 6) == 0)
    return 1;
  // idmus + 2 digits -> length 7
  if (len >= 7 && strncmp(buf, "idmus", 5) == 0)
    return 1;
  return 0;
}

//Validates whether s_CheatBuf is still a valid prefix of at least one DOOM cheat
static int isValidDoomCheatPrefix(const char *buf, int len)
{
  static const char * const cheatPrefixes[] = {
    "iddqd", "idkfa", "idfa", "idspispopd", "idclip",
    "idchoppers", "idmypos", "iddt", "idbehold", "idclev", "idmus"
  };
  for (unsigned int i = 0; i < sizeof(cheatPrefixes) / sizeof(cheatPrefixes[0]); ++i) {
    const char *p = cheatPrefixes[i];
    int plen = (int)strlen(p);
    int cmpLen = (len < plen) ? len : plen;
    if (strncmp(buf, p, cmpLen) == 0)
      return 1;
  }
  return 0;
}

static unsigned char convertToDoomKey(unsigned int key){
  switch (key)
    {
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
      key = KEY_ENTER;
      break;
    case SDLK_ESCAPE:
      key = KEY_ESCAPE;
      break;
    case SDLK_BACKSPACE:
      key = KEY_BACKSPACE;
      break;
    case SDLK_TAB:
      key = KEY_TAB;
      break;
    case SDLK_LEFT:
      key = KEY_LEFTARROW;
      break;
    case SDLK_RIGHT:
      key = KEY_RIGHTARROW;
      break;
    case SDLK_UP:
      key = KEY_UPARROW;
      break;
    case SDLK_DOWN:
      key = KEY_DOWNARROW;
      break;
    case SDLK_LCTRL:
    case SDLK_RCTRL:
      key = KEY_FIRE;
      break;
    case SDLK_SPACE:
    case SDLK_e:
      // 'e' also acts as Use/Open door when not in menu/cheat mode
      if (key == SDLK_e && (isMenuOverlayActive() || s_CheatState == 2))
        key = 'e';
      else
        key = KEY_USE;
      break;
    case SDLK_LSHIFT:
    case SDLK_RSHIFT:
      key = KEY_RSHIFT;
      break;
    case SDLK_LALT:
    case SDLK_RALT:
      key = KEY_LALT;
      break;
    case SDLK_F1:
      key = KEY_F1;
      break;
    case SDLK_F2:
      key = KEY_F2;
      break;
    case SDLK_F3:
      key = KEY_F3;
      break;
    case SDLK_F4:
      key = KEY_F4;
      break;
    // [THAY ĐỔI] Xóa case SDLK_F5 ở đây vì F5 đã được trưng dụng làm phím đổi bộ lọc OpenGL
    case SDLK_F6:
      key = KEY_F6;
      break;
    case SDLK_F7:
      key = KEY_F7;
      break;
    case SDLK_F8:
      key = KEY_F8;
      break;
    case SDLK_F9:
      key = KEY_F9;
      break;
    case SDLK_F10:
      key = KEY_F10;
      break;
    case SDLK_F11:
      key = KEY_F11;
      break;
    case SDLK_EQUALS:
    case SDLK_PLUS:
    case SDLK_KP_PLUS:
      key = KEY_EQUALS;
      break;
    case SDLK_MINUS:
    case SDLK_KP_MINUS:
      key = KEY_MINUS;
      break;
    default:
      if (key < 256)
        key = tolower((int)key);
      else
        key = 0;
      break;
    }

  return (unsigned char)key;
}

static void handleKeyEvent(int pressed, unsigned int sym, uint8_t repeat)
{
  uint32_t now = SDL_GetTicks();

  // Expire cheat state if user paused typing
  if (s_CheatState != 0 && now > s_CheatExpireTick) {
    s_CheatState = 0;
    s_CheatLen = 0;
  }

  int inMenu = isMenuOverlayActive();

  // During active gameplay (!inMenu), filter out OS auto-repeat events so holding W+A / W+D
  // never floods the key queue or interrupts simultaneous multi-key state!
  if (!inMenu && pressed && repeat != 0) {
    return;
  }

  int wasdIdx = wasdIndexFromSym(sym);

  if (!pressed) {
    // Key release: if this was W, A, S, or D, release the exact DOOM key sent when it was pressed
    if (wasdIdx >= 0) {
      unsigned char activeKey = s_WasdActiveKey[wasdIdx];
      s_WasdActiveKey[wasdIdx] = 0;
      if (activeKey != 0) {
        enqueueDoomKey(0, activeKey);
      }
      return;
    }
    unsigned char dk = convertToDoomKey(sym);
    if (dk != 0) enqueueDoomKey(0, dk);
    return;
  }

  // Key press (pressed == 1):
  if (!inMenu) {
    // Check "id..." cheat state machine
    int asciiCh = (sym < 128) ? tolower((int)sym) : 0;

    if (s_CheatState == 0) {
      if (asciiCh == 'i') {
        s_CheatState = 1;
        s_CheatExpireTick = now + 1500;
        s_CheatBuf[0] = 'i';
        s_CheatBuf[1] = '\0';
        s_CheatLen = 1;
        enqueueDoomKey(1, 'i');
        return;
      }
    } else if (s_CheatState == 1) {
      if (asciiCh == 'd') {
        // Confirmed "id" cheat prefix! Release any held WASD movement keys and enter cheat mode
        releaseActiveWasdKeys();
        s_CheatState = 2;
        s_CheatExpireTick = now + 2500;
        s_CheatBuf[1] = 'd';
        s_CheatBuf[2] = '\0';
        s_CheatLen = 2;
        enqueueDoomKey(1, 'd');
        return;
      } else if (asciiCh == 'i') {
        s_CheatExpireTick = now + 1500;
        enqueueDoomKey(1, 'i');
        return;
      } else {
        // Not followed by 'd' -> cancel cheat prefix and treat as normal gameplay key
        s_CheatState = 0;
        s_CheatLen = 0;
      }
    } else if (s_CheatState == 2) {
      if ((asciiCh >= 'a' && asciiCh <= 'z') || (asciiCh >= '0' && asciiCh <= '9')) {
        if (s_CheatLen < (int)sizeof(s_CheatBuf) - 1) {
          s_CheatBuf[s_CheatLen++] = (char)asciiCh;
          s_CheatBuf[s_CheatLen] = '\0';
        }
        s_CheatExpireTick = now + 2500;
        if (wasdIdx >= 0) {
          s_WasdActiveKey[wasdIdx] = (unsigned char)asciiCh;
        }
        enqueueDoomKey(1, (unsigned char)asciiCh);

        // If cheat completed or is no longer a valid DOOM cheat prefix, return to WASD mode immediately
        if (isCompleteDoomCheat(s_CheatBuf, s_CheatLen) || !isValidDoomCheatPrefix(s_CheatBuf, s_CheatLen)) {
          s_CheatState = 0;
          s_CheatLen = 0;
        }
        return;
      } else {
        s_CheatState = 0;
        s_CheatLen = 0;
      }
    }

    // Normal gameplay WASD movement mapping (100% independent of mouse angleturn!)
    if (wasdIdx >= 0) {
      unsigned char moveKey = wasdMovementKey(wasdIdx);
      s_WasdActiveKey[wasdIdx] = moveKey;
      enqueueDoomKey(1, moveKey);
      return;
    }
  } else {
    // In menu: if user is typing a savegame name, W/A/S/D type normal letters;
    // if browsing menu (not typing savegame name), W/S also move up/down and A/D move sliders!
    if (wasdIdx >= 0 && saveStringEnter == 0 && messageToPrint == 0) {
      unsigned char menuNavKey = 0;
      if (wasdIdx == 0) menuNavKey = KEY_UPARROW;
      else if (wasdIdx == 2) menuNavKey = KEY_DOWNARROW;
      else if (wasdIdx == 1) menuNavKey = KEY_LEFTARROW;
      else if (wasdIdx == 3) menuNavKey = KEY_RIGHTARROW;
      s_WasdActiveKey[wasdIdx] = menuNavKey;
      enqueueDoomKey(1, menuNavKey);
      return;
    } else if (wasdIdx >= 0) {
      unsigned char ch = (unsigned char)tolower((int)sym);
      s_WasdActiveKey[wasdIdx] = ch;
      enqueueDoomKey(1, ch);
      return;
    }
  }

  unsigned char dk = convertToDoomKey(sym);
  if (dk != 0) {
    enqueueDoomKey(1, dk);
  }
}

static void pumpSdlEvents(void)
{
  SDL_Event e;
  int inMenu = isMenuOverlayActive();

  // Detect menu closing -> automatically re-grab mouse for FPS gameplay
  if (s_PrevMenuActive && !inMenu) {
    s_MenuSliderEdit = 0;
    if (!s_MouseGrabbed) {
      setMouseGrab(1);
    }
  }
  s_PrevMenuActive = inMenu;

  // If menu item changed away from the slider item, auto-exit slider edit mode
  if (s_MenuSliderEdit) {
    if (!isCurrentMenuItemSlider() || currentMenu != s_SliderMenuPtr || itemOn != s_SliderMenuItemOn) {
      s_MenuSliderEdit = 0;
    }
  }

  while (SDL_PollEvent(&e)){
    if (e.type == SDL_QUIT){
      puts("Quit requested");
      atexit(SDL_Quit);
      exit(1);
    }

    if (e.type == SDL_WINDOWEVENT) {
      if (e.window.event == SDL_WINDOWEVENT_FOCUS_LOST ||
          e.window.event == SDL_WINDOWEVENT_MINIMIZED) {
        if (s_MouseGrabbed) {
          setMouseGrab(0);
        }
        s_MouseButtons = 0;
        releaseActiveWasdKeys();
      }
      // [THAY ĐỔI] Bắt sự kiện Resize cửa sổ để cập nhật glViewport
      else if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
        glViewport(0, 0, e.window.data1, e.window.data2);
      }
    }

    if (e.type == SDL_KEYDOWN) {
      // [THAY ĐỔI] F5: Đổi bộ lọc (Nearest / Linear)
      if (e.key.keysym.sym == SDLK_F5) {
        filter_mode = (filter_mode == 0) ? 1 : 0;
        continue;
      }
      // F12: Toggle mouse grab (lock/unlock cursor)
      if (e.key.keysym.sym == SDLK_F12) {
        setMouseGrab(!s_MouseGrabbed);
        continue;
      }
      // CAPSLOCK: Toggle vertical mouse movement (novert)
      if (e.key.keysym.sym == SDLK_CAPSLOCK) {
        s_NoVert = !s_NoVert;
        continue;
      }
      // [ and ]: Adjust mouse sensitivity scale on the fly
      if (e.key.keysym.sym == SDLK_LEFTBRACKET) {
        if (s_MouseSensScale > 1) s_MouseSensScale--;
        continue;
      }
      if (e.key.keysym.sym == SDLK_RIGHTBRACKET) {
        if (s_MouseSensScale < 16) s_MouseSensScale++;
        continue;
      }
      handleKeyEvent(1, e.key.keysym.sym, e.key.repeat);
    } else if (e.type == SDL_KEYUP) {
      // [THAY ĐỔI] Thêm F5 vào danh sách bỏ qua khi nhả phím
      if (e.key.keysym.sym == SDLK_F5 ||
          e.key.keysym.sym == SDLK_F12 ||
          e.key.keysym.sym == SDLK_CAPSLOCK ||
          e.key.keysym.sym == SDLK_LEFTBRACKET ||
          e.key.keysym.sym == SDLK_RIGHTBRACKET) {
        continue;
      }
      handleKeyEvent(0, e.key.keysym.sym, 0);
    } else if (e.type == SDL_MOUSEMOTION) {
      // Only accumulate camera rotation when NOT inside the menu overlay
      if (s_MouseGrabbed && !isMenuOverlayActive()) {
        s_AccumMouseDX += e.motion.xrel;
        s_AccumMouseDY += e.motion.yrel;
      }
    } else if (e.type == SDL_MOUSEWHEEL) {
      if (e.wheel.y == 0) continue;
      int steps = abs(e.wheel.y);
      if (steps > 4) steps = 4;

      if (isMenuOverlayActive()) {
        // In Menu:
        if (messageToPrint != 0 || saveStringEnter != 0) {
          continue;
        }
        if (s_MenuSliderEdit && isCurrentMenuItemSlider()) {
          // Slider Adjust Mode: Wheel Up = Right Arrow (Increase), Wheel Down = Left Arrow (Decrease)
          unsigned char arrowKey = (e.wheel.y > 0) ? KEY_RIGHTARROW : KEY_LEFTARROW;
          for (int i = 0; i < steps; ++i) {
            tapDoomKey(arrowKey);
          }
        } else {
          // Normal Menu Browse Mode: Wheel Up = Up Arrow, Wheel Down = Down Arrow
          s_MenuSliderEdit = 0;
          unsigned char navKey = (e.wheel.y > 0) ? KEY_UPARROW : KEY_DOWNARROW;
          for (int i = 0; i < steps; ++i) {
            tapDoomKey(navKey);
          }
        }
      } else {
        // In Gameplay: Wheel Up = Next Weapon, Wheel Down = Previous Weapon
        unsigned char weapKey = (e.wheel.y > 0) ? DG_KEY_NEXTWEAPON : DG_KEY_PREVWEAPON;
        tapDoomKey(weapKey);
      }
    } else if (e.type == SDL_MOUSEBUTTONDOWN) {
      if (!s_MouseGrabbed && e.button.button == SDL_BUTTON_LEFT) {
        setMouseGrab(1);
      }

      if (isMenuOverlayActive()) {
        // --- MENU MOUSE CLICK HANDLING ---
        if (e.button.button == SDL_BUTTON_LEFT) {
          if (messageToPrint != 0 && messageNeedsInput) {
            // Confirm Yes ('y') on Yes/No prompts (e.g., Quit Game / End Game)
            tapDoomKey('y');
          } else if (saveStringEnter != 0) {
            tapDoomKey(KEY_ENTER);
          } else if (inhelpscreens) {
            tapDoomKey(KEY_BACKSPACE);
          } else if (isCurrentMenuItemSlider()) {
            // Toggle Slider Edit Mode on Thermometer Sliders (Screen Size, Sensitivity, Volume)
            s_MenuSliderEdit = !s_MenuSliderEdit;
            s_SliderMenuItemOn = itemOn;
            s_SliderMenuPtr = currentMenu;
          } else {
            // Normal Menu Item -> Select / Enter
            tapDoomKey(KEY_ENTER);
          }
        } else if (e.button.button == SDL_BUTTON_RIGHT) {
          if (messageToPrint != 0 && messageNeedsInput) {
            // Cancel ('n') on Yes/No prompts
            tapDoomKey('n');
          } else if (saveStringEnter != 0) {
            tapDoomKey(KEY_ESCAPE);
          } else if (s_MenuSliderEdit) {
            // Exit Slider Edit Mode first, or go back
            s_MenuSliderEdit = 0;
          } else if (inhelpscreens || (currentMenu != NULL && currentMenu->prevMenu != NULL)) {
            // In a Submenu -> Go back to parent menu (KEY_BACKSPACE)
            tapDoomKey(KEY_BACKSPACE);
          } else {
            // At top-level Main Menu -> Close menu and return to gameplay (KEY_ESCAPE)
            tapDoomKey(KEY_ESCAPE);
          }
        } else if (e.button.button == SDL_BUTTON_MIDDLE) {
          tapDoomKey(KEY_ENTER);
        }
      } else {
        // --- IN-GAME COMBAT MOUSE CLICK HANDLING ---
        // Note: We NEVER set mousebstrafe (bit 1) here so mouse horizontal movement
        // ALWAYS turns the camera smoothly without ever locking/stuttering!
        if (e.button.button == SDL_BUTTON_LEFT) {
          s_MouseButtons |= 1; // mousebfire (Fire weapon)
        } else if (e.button.button == SDL_BUTTON_RIGHT ||
                   e.button.button == SDL_BUTTON_MIDDLE ||
                   e.button.button == SDL_BUTTON_X1) {
          enqueueDoomKey(1, KEY_USE); // Right/Middle/Side1 click = Open doors / Activate switches
        } else if (e.button.button == SDL_BUTTON_X2) {
          enqueueDoomKey(1, KEY_RSHIFT); // Side button 2 = Run
        }
      }
    } else if (e.type == SDL_MOUSEBUTTONUP) {
      if (e.button.button == SDL_BUTTON_LEFT) {
        s_MouseButtons &= ~1;
      } else if (e.button.button == SDL_BUTTON_RIGHT ||
                 e.button.button == SDL_BUTTON_MIDDLE ||
                 e.button.button == SDL_BUTTON_X1) {
        if (!isMenuOverlayActive()) {
          enqueueDoomKey(0, KEY_USE);
        }
      } else if (e.button.button == SDL_BUTTON_X2) {
        if (!isMenuOverlayActive()) {
          enqueueDoomKey(0, KEY_RSHIFT);
        }
      }
    }
  }
}

static void flushMouseEventToDoom(void)
{
  if (isMenuOverlayActive()) {
    s_AccumMouseDX = 0;
    s_AccumMouseDY = 0;
    if (s_MouseButtons != 0 || s_PrevMouseButtons != 0) {
      s_MouseButtons = 0;
      s_PrevMouseButtons = 0;
      event_t ev;
      memset(&ev, 0, sizeof(ev));
      ev.type = ev_mouse;
      D_PostEvent(&ev);
    }
    return;
  }

  // Only post ONE consolidated ev_mouse event when actual motion or button state change occurred.
  // Never post dummy dx=0 events when holding a button, preventing G_Responder from overwriting mousex!
  if (s_AccumMouseDX != 0 || s_AccumMouseDY != 0 || s_MouseButtons != s_PrevMouseButtons) {
    event_t ev;
    ev.type = ev_mouse;
    ev.data1 = s_MouseButtons;
    ev.data2 = s_AccumMouseDX * s_MouseSensScale;
    ev.data3 = s_NoVert ? 0 : (-s_AccumMouseDY * s_MouseSensScale);
    ev.data4 = 0;
    D_PostEvent(&ev);

    s_LastMouseX = s_AccumMouseDX;
    s_LastMouseY = s_AccumMouseDY;
    s_AccumMouseDX = 0;
    s_AccumMouseDY = 0;
    s_PrevMouseButtons = s_MouseButtons;
  }
}

// [THAY ĐỔI] Thêm hàm vẽ Quad bằng OpenGL thay vì SDL_RenderCopy
void drawOpenGLQuad(void) {
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, glTexture);
  
  // Áp dụng bộ lọc (Nearest hoặc Linear)
  if (filter_mode == 0) {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  } else {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  }

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // Vẽ hình chữ nhật phủ kín cửa sổ
  // [ĐÃ SỬA] Đảo ngược tọa độ Texture trục Y để hình không bị lật
  glBegin(GL_QUADS);
      glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, -1.0f); // Bottom-Left
      glTexCoord2f(1.0f, 1.0f); glVertex2f( 1.0f, -1.0f); // Bottom-Right
      glTexCoord2f(1.0f, 0.0f); glVertex2f( 1.0f,  1.0f); // Top-Right
      glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f,  1.0f); // Top-Left
  glEnd();
}

void DG_Init(){
  // [THAY ĐỔI] Thêm cờ SDL_WINDOW_OPENGL
  window = SDL_CreateWindow("DOOM (Level 1A)",
                            SDL_WINDOWPOS_UNDEFINED,
                            SDL_WINDOWPOS_UNDEFINED,
                            DOOMGENERIC_RESX,
                            DOOMGENERIC_RESY,
                            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL
                            );

  // [THAY ĐỔI] Khởi tạo OpenGL Context thay cho SDL_Renderer
  glContext = SDL_GL_CreateContext(window);
  if (!glContext) {
      printf("Lỗi tạo OpenGL Context: %s\n", SDL_GetError());
      exit(1);
  }
  SDL_GL_SetSwapInterval(1); // Bật VSync

  // [THAY ĐỔI] Khởi tạo OpenGL Texture
  glGenTextures(1, &glTexture);
  glBindTexture(GL_TEXTURE_2D, glTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, DOOMGENERIC_RESX, DOOMGENERIC_RESY, 0, GL_BGRA, GL_UNSIGNED_BYTE, DG_ScreenBuffer);

  // Bind DOOM's native weapon cycling keys to our virtual wheel keycodes
  key_prevweapon = DG_KEY_PREVWEAPON;
  key_nextweapon = DG_KEY_NEXTWEAPON;

  // Enable relative mouse mode by default for FPS aiming
  setMouseGrab(1);
}

void DG_DrawFrame()
{
  // [THAY ĐỔI] Chuyển sang dùng lệnh vẽ của OpenGL
  glBindTexture(GL_TEXTURE_2D, glTexture);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, DOOMGENERIC_RESX, DOOMGENERIC_RESY, GL_BGRA, GL_UNSIGNED_BYTE, DG_ScreenBuffer);
  
  drawOpenGLQuad();
  SDL_GL_SwapWindow(window);

  pumpSdlEvents();
}

void DG_SleepMs(uint32_t ms)
{
  SDL_Delay(ms);
}

uint32_t DG_GetTicksMs()
{
  return SDL_GetTicks();
}

int DG_GetKey(int* pressed, unsigned char* doomKey)
{
  // Pump SDL events right at the start of I_StartTic() so input has zero frame lag
  pumpSdlEvents();

  if (s_KeyQueueReadIndex == s_KeyQueueWriteIndex){
    // Key queue is empty -> flush consolidated mouse motion once for this tic
    flushMouseEventToDoom();
    return 0;
  }else{
    unsigned short keyData = s_KeyQueue[s_KeyQueueReadIndex];
    s_KeyQueueReadIndex++;
    s_KeyQueueReadIndex %= KEYQUEUE_SIZE;

    *pressed = keyData >> 8;
    *doomKey = keyData & 0xFF;

    if (s_KeyQueueReadIndex == s_KeyQueueWriteIndex) {
      flushMouseEventToDoom();
    }

    return 1;
  }
}

void DG_SetWindowTitle(const char * title)
{
  if (window != NULL){
    SDL_SetWindowTitle(window, title);
  }
}

int main(int argc, char **argv)
{
    int has_iwad = 0;
    for (int i = 1; i < argc; ++i)
    {
        if (argv[i] != NULL && strcmp(argv[i], "-iwad") == 0)
        {
            has_iwad = 1;
            break;
        }
    }

    if (!has_iwad)
    {
        static char s_waddir_env[] = "DOOMWADDIR=wads";
        putenv(s_waddir_env);

        const char *wad_candidates[4];
        int num_candidates = 0;
        wad_candidates[num_candidates++] = "wads/DOOM1.WAD";
        wad_candidates[num_candidates++] = "wads\\DOOM1.WAD";
        wad_candidates[num_candidates++] = "DOOM1.WAD";

        static char s_exe_wad_path[1024];
        if (argc > 0 && argv[0] != NULL)
        {
            const char *last_slash = strrchr(argv[0], '\\');
            const char *last_fslash = strrchr(argv[0], '/');
            if (last_fslash != NULL && (last_slash == NULL || last_fslash > last_slash))
            {
                last_slash = last_fslash;
            }
            if (last_slash != NULL)
            {
                size_t dir_len = (size_t)(last_slash - argv[0]);
                if (dir_len + 20 < sizeof(s_exe_wad_path))
                {
                    memcpy(s_exe_wad_path, argv[0], dir_len);
                    strcpy(s_exe_wad_path + dir_len, "\\wads\\DOOM1.WAD");
                    wad_candidates[num_candidates++] = s_exe_wad_path;
                }
            }
        }

        const char *found_wad = NULL;
        for (int c = 0; c < num_candidates; ++c)
        {
            FILE *fp = fopen(wad_candidates[c], "rb");
            if (fp != NULL)
            {
                fclose(fp);
                found_wad = wad_candidates[c];
                break;
            }
        }

        if (found_wad != NULL)
        {
            char **new_argv = (char **)malloc((size_t)(argc + 3) * sizeof(char *));
            if (new_argv != NULL)
            {
                for (int i = 0; i < argc; ++i)
                {
                    new_argv[i] = argv[i];
                }
                new_argv[argc]     = "-iwad";
                new_argv[argc + 1] = (char *)found_wad;
                new_argv[argc + 2] = NULL;
                argc += 2;
                argv = new_argv;
            }
        }
    }

    doomgeneric_Create(argc, argv);

    for (int i = 0; ; i++)
    {
        doomgeneric_Tick();
    }

    return 0;
}

int drone = 0;
int net_client_connected = 0;

int DG_GetMouse(int *x, int *y, int *buttons) {
    if (x) *x = s_LastMouseX;
    if (y) *y = s_LastMouseY;
    if (buttons) *buttons = s_MouseButtons;
    return (s_LastMouseX != 0 || s_LastMouseY != 0 || s_MouseButtons != 0) ? 1 : 0;
}