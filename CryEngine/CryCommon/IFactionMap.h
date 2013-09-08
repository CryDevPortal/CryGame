#ifndef __IFactionMap_h__
#define __IFactionMap_h__

#pragma once


struct IFactionMap
{
	virtual ~IFactionMap(){}
	
	enum EReactionType
	{
		eRT_Hostile = 0, // intentionally from most-hostile to most-friendly
		eRT_Neutral,
		eRT_Friendly,
	};

	enum
	{
		InvalidFactionID = 0xff,
	};

	virtual uint32 GetFactionCount() const = 0;
	virtual const char* GetFactionName(uint8 fraction) const = 0;
	virtual uint8 GetFactionID(const char* name) const = 0;

	virtual void SetReaction(uint8 factionOne, uint8 factionTwo, IFactionMap::EReactionType reaction) = 0;
	virtual IFactionMap::EReactionType GetReaction(uint8 factionOne, uint8 factionTwo) const = 0;
};

#endif