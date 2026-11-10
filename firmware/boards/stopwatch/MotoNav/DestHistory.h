#pragma once
#include <Arduino.h>
#include <Preferences.h>

#define MAX_DEST_ITEMS 8

struct DestItem {
  uint8_t type;  // 0 = 常用收藏 (Star), 1 = 导航历史 (Pin)
  char name[32]; // 名称 (如 "Home", "Work", "Mall")
  char addr[32]; // 简短地址
  bool valid;
};

extern DestItem destItems[MAX_DEST_ITEMS];
extern int destItemCount;
extern bool destSheetOpen;
extern int destScrollOffset;

inline void destSaveToNvs() {
  Preferences p;
  p.begin("motonav_dest", false);
  p.putInt("cnt", destItemCount);
  for (int i = 0; i < destItemCount; i++) {
    char keyN[16], keyA[16], keyT[16];
    snprintf(keyN, sizeof(keyN), "n_%d", i);
    snprintf(keyA, sizeof(keyA), "a_%d", i);
    snprintf(keyT, sizeof(keyT), "t_%d", i);
    p.putString(keyN, destItems[i].name);
    p.putString(keyA, destItems[i].addr);
    p.putUChar(keyT, destItems[i].type);
  }
  p.end();
}

inline void destLoadFromNvs() {
  Preferences p;
  p.begin("motonav_dest", true);
  int cnt = p.getInt("cnt", 0);
  if (cnt > 0 && cnt <= MAX_DEST_ITEMS) {
    destItemCount = cnt;
    for (int i = 0; i < cnt; i++) {
      char keyN[16], keyA[16], keyT[16];
      snprintf(keyN, sizeof(keyN), "n_%d", i);
      snprintf(keyA, sizeof(keyA), "a_%d", i);
      snprintf(keyT, sizeof(keyT), "t_%d", i);
      String n = p.getString(keyN, "");
      String a = p.getString(keyA, "");
      destItems[i].type = p.getUChar(keyT, 0);
      strncpy(destItems[i].name, n.c_str(), sizeof(destItems[i].name) - 1);
      destItems[i].name[sizeof(destItems[i].name) - 1] = '\0';
      strncpy(destItems[i].addr, a.c_str(), sizeof(destItems[i].addr) - 1);
      destItems[i].addr[sizeof(destItems[i].addr) - 1] = '\0';
      destItems[i].valid = true;
    }
  }
  p.end();
}

inline void destItemsClear() {
  destItemCount = 0;
  for (int i = 0; i < MAX_DEST_ITEMS; i++) {
    destItems[i].valid = false;
    destItems[i].name[0] = '\0';
    destItems[i].addr[0] = '\0';
  }
  Preferences p;
  p.begin("motonav_dest", false);
  p.putInt("cnt", 0);
  p.end();
}

inline void destItemAdd(uint8_t type, const char* name, const char* addr) {
  if (destItemCount >= MAX_DEST_ITEMS) return;
  int idx = destItemCount++;
  destItems[idx].type = type;
  strncpy(destItems[idx].name, name ? name : "", sizeof(destItems[idx].name) - 1);
  destItems[idx].name[sizeof(destItems[idx].name) - 1] = '\0';
  strncpy(destItems[idx].addr, addr ? addr : "", sizeof(destItems[idx].addr) - 1);
  destItems[idx].addr[sizeof(destItems[idx].addr) - 1] = '\0';
  destItems[idx].valid = true;
}

// 默认填充两项常用地点 (开机先读取 NVS，若无则提示未同步)
inline void destInitDefaults() {
  if (destItemCount > 0) return;
  destLoadFromNvs();
  if (destItemCount > 0) return;
  destItemAdd(0, "NO FAVORITES", "Sync from phone App");
}
