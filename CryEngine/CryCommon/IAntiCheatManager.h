#ifndef IAntiCheatManager_h
#define IAntiCheatManager_h

#if _MSC_VER > 1000
#pragma once
#endif

typedef int32 TCheatType;
typedef int32 TCheatAssetGroup;

UNIQUE_IFACE struct IAntiCheatManager
{
  virtual ~IAntiCheatManager(){}
  virtual int RetrieveHashMethod() = 0;
  virtual int GetAssetGroupCount() = 0;
  virtual void FlagActivity(TCheatType type, uint16 channelId, const char * message) = 0;
  virtual void FlagActivity(TCheatType type, EntityId playerId) = 0;
  virtual void FlagActivity(TCheatType type, EntityId playerId, float param1) = 0;
  virtual void FlagActivity(TCheatType type, EntityId playerId, float param1, float param2) = 0;
  virtual TCheatType FindCheatType(const char* name) = 0;
  virtual TCheatAssetGroup FindAssetTypeByExtension(const char * ext) = 0;
  virtual TCheatAssetGroup FindAssetTypeByWeight() = 0;
  virtual void OnSessionEnd() = 0;
};

#endif
